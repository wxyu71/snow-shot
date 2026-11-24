#!/usr/bin/env python3
"""Community upstream integration: one rolling PR, never a direct main push.

The bot preserves all existing PR commits.  A conflicting first merge starts
from the *real upstream commit*, not a misleading report-only commit.  GitHub
cannot store an unresolved index; a human must merge Community main into that
branch and resolve conflicts before the PR can pass the ancestry gate.
"""
from __future__ import annotations

import argparse
import fnmatch
import json
import os
import re
import subprocess
import sys
import tempfile
import tomllib
from pathlib import Path
from typing import Any

ROOT = Path(__file__).resolve().parents[1]
POLICY = ROOT / ".github" / "upstream-protection.json"
COMMUNITY_UPDATE_KEYS = ROOT / "snow_shot" / "resources" / "update-trusted-keys.json"
BOT_START = "<!-- community-upstream-sync:begin -->"
BOT_END = "<!-- community-upstream-sync:end -->"
BRANCH_PREFIX = "automation/upstream-sync-"
SHA = re.compile(r"^[a-f0-9]{40}$")
# Login items are OS auto-start facilities, not commercial account sign-in.
COMMERCIAL_PATH = re.compile(
    r"(?:^|[/_-])(?:account|membership|membercenter|billing|payment|purchase|topup|"
    r"recharge|credits?|points?|paywall|subscription|signin|signup|register|checkout)"
    r"(?:[/_.-]|$)|(?:^|[/_-])login(?:[/.-]|$)", re.I
)
COMMERCIAL_CONTENT = re.compile(
    r"AccountSessionService|AccountPageWidget|account_secret|insufficient_credits|"
    r"account_required|purchase[._-]?credits|membership[._-]?payment|"
    r"top[._-]?up|recharge|会员|充值|支付|积分", re.I
)


# Current-tree safety policy. These are runtime/product identifiers, not the upstream
# metadata in .github/upstream-protection.json that tells the sync tool where to fetch from.
FORBIDDEN_COMMUNITY_CONTENT = re.compile(
    r"AccountSessionService|AccountPageWidget|AccountSecretStore|"
    r"accountAccessToken|setAccountAccessToken|accountIdentityChanged|officialCallSettled|"
    r"account_required|invalid_session|insufficient_credits|outstanding_balance|"
    r"funds_temporarily_reserved|purchase\.credits|membership\.payment|"
    r"account-session|Snow Shot account|Purchase credits|available credits|"
    r"official account authorization|"
    r"\btelemetry\b|\b(?:sentry|posthog|mixpanel|firebase_analytics)\b|segment\.io|"
    r"\bmg-chao/(?:snow-shot|snow-apps)\b|\bmg-chao\.snow-shot\b|"
    r"\bsnowshot\.top\b|\b120\.79\.232\.67\b",
    re.I,
)
COMMUNITY_SCAN_ROOTS = (
    ".github/workflows",
    "cmake",
    "scripts",
    "snow-crates",
    "snow-draw-engine-gpui",
    "snow-image-rs",
    "snow_draw_engine_qt",
    "snow_image",
    "snow_image_viewer",
    "snow_memory",
    "snow_rust_ffi",
    "snow_shot/include",
    "snow_shot/src",
    "snow_shot/rust",
    "snow_shot/tests",
)
COMMUNITY_SCAN_SUFFIXES = {
    ".c", ".cc", ".cpp", ".h", ".hpp", ".mm", ".m",
    ".py", ".js", ".ts", ".rs", ".cmake", ".txt", ".json", ".yml", ".yaml", ".sh", ".ps1",
}
COMMUNITY_SCAN_EXCLUSIONS = {
    "scripts/upstream_sync.py",
    "scripts/test-upstream-sync.py",
}


def run(*args: str, cwd: Path | None = None, check: bool = True) -> subprocess.CompletedProcess[str]:
    proc = subprocess.run(args, cwd=cwd if cwd is not None else ROOT, text=True, stdout=subprocess.PIPE,
                          stderr=subprocess.PIPE, check=False)
    if check and proc.returncode:
        raise RuntimeError(f"{' '.join(args)} failed ({proc.returncode}): {proc.stderr.strip()}")
    return proc


def git(*args: str, check: bool = True) -> str:
    return run("git", *args, check=check).stdout.strip()


def gh(*args: str) -> Any:
    result = run("gh", *args).stdout
    return json.loads(result) if result.strip().startswith(("[", "{")) else result.strip()


def ancestor(a: str, b: str) -> bool:
    return run("git", "merge-base", "--is-ancestor", a, b, check=False).returncode == 0


def changed_paths(base: str, head: str) -> set[str]:
    # Keep old/new names visible separately, including deletions.
    output = subprocess.check_output(
        ["git", "diff", "--no-renames", "--name-only", "-z", base, head], cwd=ROOT
    )
    return {s.decode("utf-8", errors="replace") for s in output.split(b"\0") if s}


def is_protected(path: str, patterns: list[str]) -> bool:
    return any(fnmatch.fnmatchcase(path, pattern) for pattern in patterns)


def decide(incoming: set[str], fork_changes: set[str], policy: dict,
           suspicious_patch: bool = False) -> dict:
    if not incoming:
        return {"mode": "noop", "reasons": [], "upstream_paths": [],
                "overlap": [], "protected": [], "commercial": []}
    overlap = sorted(incoming & fork_changes)
    protected = sorted(p for p in incoming if p in policy["deleted_files"] or
                       is_protected(p, policy["protected_globs"]))
    commercial = sorted(p for p in incoming if COMMERCIAL_PATH.search(p))
    reasons = []
    if overlap:
        reasons.append("Both sides changed the same paths (not necessarily a Git conflict): " +
                       ", ".join(overlap[:20]))
    if protected:
        reasons.append("Community protected or intentionally deleted paths: " +
                       ", ".join(protected[:20]))
    if commercial or suspicious_patch:
        reasons.append("Possible account/commercial code: " +
                       (", ".join(commercial[:20]) or "new source content"))
    return {"mode": "review" if reasons else "auto", "reasons": reasons,
            "upstream_paths": sorted(incoming), "overlap": overlap,
            "protected": protected, "commercial": commercial}


def added_commercial_content(base: str, upstream: str, paths: set[str]) -> bool:
    source = sorted(p for p in paths if p.endswith(
        (".cpp", ".h", ".mm", ".py", ".js", ".ts", ".cmake", ".md", ".yml", ".yaml")))
    if not source:
        return False
    patch = git("diff", "--no-ext-diff", "--no-renames", "--unified=0",
                base, upstream, "--", *source)
    return any(COMMERCIAL_CONTENT.search(line[1:]) for line in patch.splitlines()
               if line.startswith("+") and not line.startswith("+++"))


def classify(upstream: str, output: Path | None = None,
             main: str = "HEAD", policy: dict | None = None) -> dict:
    policy = policy if policy is not None else json.loads(POLICY.read_text(encoding="utf-8"))
    base = git("merge-base", main, upstream)
    incoming = changed_paths(base, upstream)
    fork_changes = changed_paths(base, main)
    result = decide(incoming, fork_changes, policy,
                    added_commercial_content(base, upstream, incoming))
    result.update(merge_base=base, upstream_commit=git("rev-parse", upstream),
                  main_commit=git("rev-parse", main))
    if output is not None:
        output.write_text(json.dumps(result, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    return result


def assert_deleted_files_absent(policy: dict | None = None, tree: str | None = None) -> None:
    policy = policy if policy is not None else json.loads(POLICY.read_text(encoding="utf-8"))
    for path in policy["deleted_files"]:
        exists = (run("git", "cat-file", "-e", f"{tree}:{path}", check=False).returncode == 0
                  if tree else (ROOT / path).exists() or
                  run("git", "ls-files", "--error-unmatch", path, check=False).returncode == 0)
        if exists:
            raise RuntimeError(f"Removed commercial file was restored: {path}")


def assert_forbidden_runtime_content_absent() -> None:
    violations: list[str] = []
    for root_name in COMMUNITY_SCAN_ROOTS:
        root = ROOT / root_name
        if not root.exists():
            continue
        for path in root.rglob("*"):
            if not path.is_file() or path.suffix.lower() not in COMMUNITY_SCAN_SUFFIXES:
                continue
            relative = path.relative_to(ROOT).as_posix()
            if relative in COMMUNITY_SCAN_EXCLUSIONS:
                continue
            try:
                content = path.read_text(encoding="utf-8")
            except UnicodeDecodeError:
                continue
            match = FORBIDDEN_COMMUNITY_CONTENT.search(content)
            if match:
                line = content.count("\n", 0, match.start()) + 1
                violations.append(f"{relative}:{line}: {match.group(0)}")
    if violations:
        raise SystemExit(
            "Forbidden account/telemetry/upstream runtime content found:\n" +
            "\n".join(violations[:50])
        )


def assert_community_update_signers_disabled() -> None:
    try:
        document = json.loads(COMMUNITY_UPDATE_KEYS.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as error:
        raise SystemExit(f"Community update trust store is invalid: {error}") from error
    if not isinstance(document, dict) or set(document) != {"keys"} or document["keys"] != []:
        raise SystemExit(
            "Community updater must not embed remote release signing keys; "
            "publishers are disabled until the community establishes its own signer."
        )


def immutable_git_dependency(specification: dict) -> bool:
    revision = specification.get("rev")
    return (
        isinstance(specification.get("git"), str)
        and isinstance(revision, str)
        and re.fullmatch(r"[0-9a-fA-F]{40}", revision) is not None
        and "branch" not in specification
        and "tag" not in specification
    )


def _git_dependency_specs(value, location: str = ""):
    if isinstance(value, dict):
        if "git" in value:
            yield location or "<root>", value
        for key, child in value.items():
            child_location = f"{location}.{key}" if location else str(key)
            yield from _git_dependency_specs(child, child_location)
    elif isinstance(value, list):
        for index, child in enumerate(value):
            yield from _git_dependency_specs(child, f"{location}[{index}]")


def assert_external_source_pins() -> None:
    violations: list[str] = []
    gitmodules = ROOT / ".gitmodules"
    if gitmodules.exists():
        content = gitmodules.read_text(encoding="utf-8")
        if re.search(r"(?m)^\s*branch\s*=", content):
            violations.append(".gitmodules: submodules must follow the recorded gitlink, not a branch")
        for match in re.finditer(r"(?m)^\s*path\s*=\s*(.+?)\s*$", content):
            submodule_path = match.group(1)
            candidate = Path(submodule_path)
            if candidate.is_absolute() or ".." in candidate.parts:
                violations.append(f".gitmodules: unsafe submodule path: {submodule_path}")
                continue
            staged = subprocess.run(
                ["git", "ls-files", "--stage", "--", submodule_path],
                cwd=ROOT, capture_output=True, text=True
            )
            fields = staged.stdout.strip().split(maxsplit=3)
            if (
                staged.returncode != 0
                or len(fields) < 4
                or fields[0] != "160000"
                or re.fullmatch(r"[0-9a-fA-F]{40}", fields[1]) is None
            ):
                violations.append(
                    f"{submodule_path}: submodule must be a tracked 160000 gitlink at a full commit"
                )

    tracked = run("git", "ls-files", "-z").stdout.split("\x00")
    manifests = sorted(
        path
        for path in tracked
        if path and (
            path == "Cargo.toml" or path.endswith("/Cargo.toml")
        )
    )
    for relative in manifests:
        path = ROOT / relative
        try:
            document = tomllib.loads(path.read_text(encoding="utf-8"))
        except (OSError, tomllib.TOMLDecodeError) as error:
            violations.append(f"{relative}: cannot inspect Cargo source pins: {error}")
            continue
        for location, specification in _git_dependency_specs(document):
            if not immutable_git_dependency(specification):
                violations.append(
                    f"{relative}:{location}: git dependency requires an exact 40-character rev "
                    "and must not use branch/tag"
                )
    if violations:
        raise SystemExit(
            "External source dependencies must be immutable:\n" + "\n".join(violations)
        )



def managed_section(metadata: dict, note: str, reasons: list[str],
                    feature_overlap: list[str] | None = None) -> str:
    # A versioned machine-readable marker is also checked by PR CI.
    marker = json.dumps(metadata, sort_keys=True, separators=(",", ":"))
    detail = "\n".join(f"- {r}" for r in reasons) if reasons else "- None detected"
    overlapping = "\n".join(f"- {r}" for r in (feature_overlap or [])) or "- None detected"
    return (f"{BOT_START}\n<!-- upstream-sync-state:{marker} -->\n"
            f"### Automated upstream integration\n\n{note}\n\n"
            f"- Target upstream: `{metadata['target']}`\n"
            f"- Initial Community base: `{metadata['baseline']}`\n"
            f"- Risk level: **{metadata['risk']}**\n\n"
            f"**Policy findings**\n{detail}\n\n"
            f"**Overlapping open feature PRs (advisory)**\n{overlapping}\n\n"
            "This is a real-code PR. A merge conflict requires manual resolution; "
            "the bot never force-pushes or directly changes `main`. "
            "Mark Ready for review to freeze automated changes.\n"
            f"{BOT_END}")


def parse_metadata(body: str) -> dict:
    if body.count(BOT_START) != 1 or body.count(BOT_END) != 1:
        raise RuntimeError("Missing/ambiguous managed sync PR section; refusing to overwrite")
    within = body.split(BOT_START, 1)[1].split(BOT_END, 1)[0]
    match = re.search(r"<!-- upstream-sync-state:(\{[^\n]*\}) -->", within)
    if not match:
        raise RuntimeError("Missing sync metadata; refusing to overwrite")
    m = json.loads(match.group(1))
    if m.get("version") != 1 or any(not SHA.fullmatch(m.get(k, ""))
                                    for k in ("target", "baseline")):
        raise RuntimeError("Invalid sync metadata")
    return m


def replace_managed(body: str, section: str) -> str:
    if body.count(BOT_START) != 1 or body.count(BOT_END) != 1:
        raise RuntimeError("Refusing to replace unexpected PR body")
    before, rest = body.split(BOT_START, 1)
    _, after = rest.split(BOT_END, 1)
    return before + section + after


def write_pr_body(number: int, original: str, section: str) -> None:
    updated = replace_managed(original, section)
    if updated == original:
        return
    with tempfile.NamedTemporaryFile("w", encoding="utf-8", suffix=".md") as f:
        f.write(updated)
        f.flush()
        gh("pr", "edit", str(number), "--body-file", f.name)


def feature_pr_overlaps(prs: list[dict], incoming: set[str]) -> list[str]:
    overlaps = []
    for pr in prs:
        branch = pr["headRefName"]
        if branch.startswith(BRANCH_PREFIX) or branch.startswith("review/upstream-main-") or branch == "automation/upstream-review":
            continue
        try:
            files = gh("pr", "view", str(pr["number"]), "--json", "files")
            touched = {p["path"] for p in files["files"]}
        except (KeyError, RuntimeError):
            # Advisory-only comparison must never break upstream synchronization.
            continue
        common = sorted(incoming & touched)
        if common:
            overlaps.append(f"#{pr['number']} `{branch}`: " + ", ".join(common[:8]))
    return overlaps


def merge_candidate(other: str) -> tuple[bool, list[str]]:
    p = run("git", "merge", "--no-ff", "--no-edit", other, check=False)
    if p.returncode == 0:
        return True, []
    files = git("diff", "--name-only", "--diff-filter=U").splitlines()
    if not files or not Path(ROOT / ".git" / "MERGE_HEAD").exists():
        raise RuntimeError(f"git merge failed for a non-conflict reason: {p.stderr}")
    git("merge", "--abort")
    return False, files


def publish_branch(branch: str, expected: str | None = None) -> None:
    # Normal push, never --force.  The remote rejects a race with manual edits.
    result = run("git", "push", "origin", f"HEAD:refs/heads/{branch}", check=False)
    if result.returncode:
        raise RuntimeError(f"Could not push real upstream integration branch. "
                           f"GitHub App with workflow write permission may be required. "
                           f"No report-only fallback is created. {result.stderr.strip()}")


def summary(message: str) -> None:
    print(message)
    if path := os.environ.get("GITHUB_STEP_SUMMARY"):
        with open(path, "a", encoding="utf-8") as f:
            f.write(message + "\n")


def sync(auto_merge: bool = False) -> None:
    """Git orchestration executed only on the scheduled default-branch checkout."""
    repo = os.environ.get("GITHUB_REPOSITORY", "")
    if not repo or not os.environ.get("GH_TOKEN"):
        raise RuntimeError("GITHUB_REPOSITORY and GH_TOKEN are required")
    git("config", "user.name", "github-actions[bot]")
    git("config", "user.email", "41898282+github-actions[bot]@users.noreply.github.com")
    git("fetch", "--no-tags", "origin", "+refs/heads/main:refs/remotes/origin/main")
    policy = json.loads(git("show", "origin/main:.github/upstream-protection.json"))
    upstream_repo = policy["upstream"]
    if not re.fullmatch(r"[a-zA-Z0-9_.-]+/[a-zA-Z0-9_.-]+", upstream_repo):
        raise RuntimeError("Invalid upstream repository in main branch policy")
    if "upstream" not in git("remote").splitlines():
        git("remote", "add", "upstream", f"https://github.com/{upstream_repo}.git")
    git("fetch", "--no-tags", "upstream", "+refs/heads/main:refs/remotes/upstream/main")
    main_sha = git("rev-parse", "origin/main")
    upstream_sha = git("rev-parse", "upstream/main")
    open_prs = gh("pr", "list", "--repo", repo, "--base", "main", "--state", "open",
                  "--limit", "500", "--json", "number,headRefName,isDraft,body")
    managed = [p for p in open_prs if p["headRefName"].startswith(BRANCH_PREFIX)]
    if len(managed) > 1:
        raise RuntimeError("Multiple active upstream integration PRs; refusing to select one")
    if not managed:
        legacy = [p for p in open_prs if p["headRefName"].startswith("review/upstream-main-")]
        if legacy:
            summary("### Upstream sync waiting for existing integration\n"
                    f"- Latest upstream: `{upstream_sha}`\n"
                    "- Legacy integration PR(s): " + ", ".join("#" + str(p["number"]) for p in legacy) +
                    "\n- No parallel upstream PR was created; finish or explicitly close the legacy integration first.")
            return
        if ancestor(upstream_sha, main_sha):
            summary("Upstream already integrated into main; no PR needed.")
            return
        result = classify("upstream/main", main="origin/main", policy=policy)
        branch = BRANCH_PREFIX + main_sha[:12]
        orphan = git("ls-remote", "--heads", "origin", f"refs/heads/{branch}")
        if orphan:
            # A push might have succeeded while gh pr create failed. Reuse that
            # exact remote candidate rather than creating a different merge SHA.
            git("fetch", "--no-tags", "origin",
                f"+refs/heads/{branch}:refs/remotes/origin/{branch}")
            git("switch", "--detach", f"origin/{branch}")
            if not ancestor(main_sha, "HEAD") and not ancestor("HEAD", upstream_sha):
                raise RuntimeError("Orphaned sync branch has unfamiliar history; inspect manually")
            bootstrap = not ancestor(main_sha, "HEAD")
            upstream_target = git("merge-base", "HEAD", upstream_sha)
            result["reasons"].append("Reused an existing orphan sync branch after a partial API failure")
            files = []
        else:
            git("switch", "--detach", main_sha)
            ok, files = merge_candidate(upstream_sha)
            bootstrap = not ok
            upstream_target = upstream_sha
            if bootstrap:
                # This is the *real upstream tree*; don't invent a conflict-free merge.
                git("switch", "--detach", upstream_sha)
            else:
                # Catch the policy failure in the PR CI; never push to main.
                try:
                    assert_deleted_files_absent(policy, "HEAD")
                except RuntimeError as ex:
                    result["reasons"].append(str(ex))
        risk = "review" if bootstrap or result["reasons"] else "auto"
        metadata = {"version": 1, "baseline": main_sha, "target": upstream_target,
                    "risk": risk}
        overlaps = feature_pr_overlaps(open_prs, set(result["upstream_paths"]))
        if overlaps:
            summary("Open feature PRs share files with incoming upstream changes (advisory): " + "; ".join(overlaps))
        note = ("**Git conflicts: " + ", ".join(files[:30]) + "**. This PR currently points to the "
                "real upstream commit. Merge `origin/main` into this branch, resolve and commit "
                "all conflicts, then rerun the ancestry and Community checks. **Do not merge "
                "the upstream-only tree.**" if bootstrap else
                "Real upstream merge commit is present; inspect the Community behavior and CI before merging.")
        section = managed_section(metadata, note, result["reasons"], overlaps)
        if not orphan:
            publish_branch(branch)
        pr_body = ("This PR is the sole rolling upstream-integration candidate for the current batch.\n\n"
                   + section)
        with tempfile.NamedTemporaryFile("w", encoding="utf-8", suffix=".md") as f:
            f.write(pr_body)
            f.flush()
            creation = gh("pr", "create", "--repo", repo, "--base", "main", "--head", branch,
                          "--title", "sync: integrate official upstream", "--body-file", f.name,
                          *(["--draft"] if (risk == "review" or not auto_merge) else []))
        summary(f"### Upstream integration PR created\n- {creation}\n- Mode: {risk}\n"
                + ("- Human conflict resolution is required.\n" if bootstrap else ""))
        if risk == "auto" and auto_merge:
            gh("pr", "merge", branch, "--repo", repo, "--auto", "--merge")
        return

    pr = managed[0]
    number = pr["number"]
    metadata = parse_metadata(pr["body"])
    branch = pr["headRefName"]
    if not pr["isDraft"]:
        summary(f"### Upstream sync PR #{number} is Ready (frozen)\n"
                f"- Latest upstream: `{upstream_sha}`\n- Bot will not rewrite a Ready PR.")
        return
    git("fetch", "--no-tags", "origin", f"+refs/heads/{branch}:refs/remotes/origin/{branch}")
    initial = git("rev-parse", f"origin/{branch}")
    git("switch", "--detach", initial)
    missing_baseline = not ancestor(metadata["baseline"], initial)
    if missing_baseline:
        # Only fast-forward an untouched upstream-only draft.  Otherwise, a
        # maintainer may be editing the branch; don't interfere.
        if not ancestor(initial, upstream_sha):
            summary(f"### Sync PR #{number} requires manual Community merge\n"
                    "- The branch does not contain its initial Community baseline.\n"
                    "- Human changes are preserved; fetch this branch, merge `origin/main`, resolve and push.")
            return
        if initial != upstream_sha:
            git("switch", "--detach", upstream_sha)
            publish_branch(branch)
            metadata["target"] = upstream_sha
        note = ("This PR still contains the upstream-only tree. **Merge `origin/main` into "
                "the PR branch and resolve conflicts before merging this PR.**")
        section = managed_section(metadata, note, ["Awaiting initial manual merge of Community main"])
        write_pr_body(number, pr["body"], section)
        summary(f"### Sync PR #{number} waiting for conflict resolution\n"
                f"- Current upstream: `{upstream_sha}`\n- The full Community baseline is not yet integrated.")
        return

    if not ancestor(main_sha, "HEAD"):
        ok, files = merge_candidate(main_sha)
        if not ok:
            metadata["target"] = upstream_sha if ancestor(upstream_sha, "HEAD") else metadata["target"]
            section = managed_section(metadata, "**Blocked:** Community main advanced and cannot "
                                      "be merged automatically. Resolve on this branch; the bot preserved "
                                      "the existing remote commits.", ["Merge conflicts with Community main: " + ", ".join(files[:30])])
            write_pr_body(number, pr["body"], section)
            summary(f"Sync PR #{number} blocked by conflicts with updated Community main; no branch rewrite.")
            return
    if not ancestor(upstream_sha, "HEAD"):
        ok, files = merge_candidate(upstream_sha)
        if not ok:
            section = managed_section(metadata, "**Blocked:** Official upstream advanced. "
                                      "Resolve the new upstream merge into this existing PR branch; "
                                      "the bot preserved all previous manual resolutions.",
                                      ["New upstream target: " + upstream_sha,
                                       "Conflicts: " + ", ".join(files[:30])])
            write_pr_body(number, pr["body"], section)
            summary(f"Sync PR #{number} blocked on new upstream conflicts. Remote branch remains unchanged.")
            return
    new_head = git("rev-parse", "HEAD")
    if new_head != initial:
        publish_branch(branch)
    metadata["target"] = upstream_sha
    # Review risk is sticky for this batch; don't silently downgrade after human changes.
    delta = classify("upstream/main", main="origin/main", policy=policy)
    if delta["reasons"]:
        metadata["risk"] = "review"
    try:
        assert_deleted_files_absent(policy, "HEAD")
    except RuntimeError as ex:
        metadata["risk"] = "review"
        delta["reasons"].append(str(ex))
    section = managed_section(metadata,
        "Upstream and Community main are both present in the PR ancestry. "
        "All updates were appended without overwriting manual work.", delta["reasons"],
        feature_pr_overlaps(open_prs, set(delta["upstream_paths"])))
    write_pr_body(number, pr["body"], section)
    summary(f"### Sync PR #{number} updated\n- Current upstream: `{upstream_sha}`\n"
            f"- New commit appended: {new_head != initial}\n- Risk: {metadata['risk']}")


def validate_pr(body: str, head: str, base: str) -> None:
    m = parse_metadata(body)
    for label, val in (("PR head", head), ("PR base", base)):
        if not SHA.fullmatch(val):
            raise RuntimeError(f"Invalid {label} SHA")
    if not ancestor(m["baseline"], head):
        raise RuntimeError("PR head lacks original Community baseline (upstream-only or invalid PR)")
    if not ancestor(m["target"], head):
        raise RuntimeError("PR head lacks declared upstream commit")
    if not ancestor(base, head):
        raise RuntimeError("Sync PR is not integrated with the latest Community main")
    if not ancestor(m["target"], "upstream/main"):
        raise RuntimeError("Declared upstream SHA is not in official upstream/main history")
    if m["risk"] not in ("auto", "review"):
        raise RuntimeError("Unrecognized risk")
    if run("git", "cat-file", "-e", f"{head}:.github/upstream-review-pending.md",
           check=False).returncode == 0:
        raise RuntimeError("Report-only placeholder must not be merged")
    assert_deleted_files_absent(tree=head)
    print("Upstream PR ancestry and Community deletion guards: PASS")


def main() -> None:
    ap = argparse.ArgumentParser()
    ap.add_argument("command", choices=["classify", "verify-deletions", "sync", "validate-pr"])
    ap.add_argument("--upstream", default="upstream/main")
    ap.add_argument("--output", type=Path)
    ap.add_argument("--body-file", type=Path)
    ap.add_argument("--head")
    ap.add_argument("--base")
    ap.add_argument("--auto-merge", action="store_true")
    args = ap.parse_args()
    if args.command == "verify-deletions":
        assert_deleted_files_absent()
        assert_forbidden_runtime_content_absent()
        assert_community_update_signers_disabled()
        assert_external_source_pins()
        print("Community policy: PASS")
    elif args.command == "classify":
        print(json.dumps(classify(args.upstream, args.output), ensure_ascii=False))
    elif args.command == "validate-pr":
        if not (args.body_file and args.head and args.base):
            ap.error("validate-pr needs --body-file, --head and --base")
        validate_pr(args.body_file.read_text(encoding="utf-8"), args.head, args.base)
    elif args.command == "sync":
        sync(auto_merge=args.auto_merge)


if __name__ == "__main__":
    try:
        main()
    except (RuntimeError, ValueError) as err:
        print(f"::error::{err}", file=sys.stderr)
        sys.exit(1)
