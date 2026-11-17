#!/usr/bin/env python3
"""Fail-closed upstream-sync classifier for the community Snow Shot fork.

The classifier never modifies a worktree. The caller may auto-merge only in
'auto' mode, and must test and verify the merged tree before pushing.
"""
from __future__ import annotations

import argparse
import fnmatch
import json
import re
import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
POLICY = ROOT / ".github" / "upstream-protection.json"
COMMUNITY_UPDATE_KEYS = ROOT / "snow_shot" / "resources" / "update-trusted-keys.json"
CONFIGURATION_SCHEMA = ROOT / "snow_shot" / "src" / "storage" / "configurationschema.cpp"
WORKFLOW_ROOT = ROOT / ".github" / "workflows"
ACTION_USE = re.compile(r"uses:\\s+([^@\\s]+)@([^\\s#]+)")
COMMERCIAL_PATH = re.compile(
    r"(?:^|[/_-])(account|membership|membercenter|billing|payment|purchase|topup|"
    r"recharge|credits?|points?|paywall|subscription|login|signin|signup|register|checkout)(?:[/_.-]|$)", re.I
)
COMMERCIAL_CONTENT = re.compile(
    r"AccountSessionService|AccountPageWidget|account_secret|"
    r"insufficient_credits|account_required|purchase.credits|"
    r"membership.payment|top.up|recharge|sign.in|sign.up|register|checkout|会员|充值|支付|积分", re.I
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


def git(*args: str) -> bytes:
    return subprocess.check_output(["git", *args], cwd=ROOT)


def changed_paths(base: str, head: str) -> set[str]:
    # --no-renames keeps deleted and newly-added paths visible independently.
    return {
        p.decode("utf-8", errors="replace")
        for p in git("diff", "--no-renames", "--name-only", "-z", base, head).split(b"\x00")
        if p
    }


def is_protected(path: str, patterns: list[str]) -> bool:
    return any(fnmatch.fnmatchcase(path, pattern) for pattern in patterns)


def decide(incoming: set[str], fork_changes: set[str], policy: dict,
           suspicious_patch: bool = False) -> dict:
    if not incoming:
        return {"mode": "noop", "reasons": [], "upstream_paths": []}
    protected = set(policy["deleted_files"])
    patterns = policy["protected_globs"]
    overlap = sorted(incoming & fork_changes)
    danger = sorted(p for p in incoming if p in protected or is_protected(p, patterns))
    commercial = sorted(p for p in incoming if COMMERCIAL_PATH.search(p))
    # Only obvious documentation is eligible for direct synchronization.
    # Every source/build/tooling change is reviewed through the normal PR CI,
    # which runs the real Windows CMake/CTest suite.
    review_required = sorted(
        p for p in incoming
        if not (p.startswith("docs/") or
                p.lower().endswith((".md", ".markdown", ".rst")))
    )
    reasons = []
    if overlap:
        reasons.append("Fork and upstream both changed: " + ", ".join(overlap[:20]))
    if danger:
        reasons.append("Protected or intentionally deleted paths: " +
                       ", ".join(danger[:20]))
    if commercial or suspicious_patch:
        reasons.append("Potential commercial code in incoming upstream changes: " +
                       ", ".join(commercial[:20]))
    if review_required:
        reasons.append("Source/build/tooling changes require normal PR review: " +
                       ", ".join(review_required[:20]))
    return {
        "mode": "review" if reasons else "auto",
        "reasons": reasons,
        "upstream_paths": sorted(incoming),
        "overlap": overlap,
        "protected": danger,
        "review_required": review_required,
    }


def added_commercial_content(base: str, upstream: str, paths: set[str]) -> bool:
    # Only scan incoming changes, not historical commercial code already present.
    source_paths = sorted(
        p for p in paths if p.endswith((".cpp", ".h", ".mm", ".py", ".js",
                                       ".ts", ".cmake", ".md", ".yml", ".yaml"))
    )
    if not source_paths:
        return False
    patch = git("diff", "--no-ext-diff", "--no-renames", "--unified=0",
                base, upstream, "--", *source_paths).decode("utf-8", errors="replace")
    return any(COMMERCIAL_CONTENT.search(line[1:])
               for line in patch.splitlines()
               if line.startswith("+") and not line.startswith("+++"))


def classify(upstream: str, output: Path | None = None) -> dict:
    policy = json.loads(POLICY.read_text(encoding="utf-8"))
    base = git("merge-base", "HEAD", upstream).decode().strip()
    incoming = changed_paths(base, upstream)
    fork_changes = changed_paths(base, "HEAD")
    result = decide(incoming, fork_changes, policy,
                    added_commercial_content(base, upstream, incoming))
    result["merge_base"] = base
    result["upstream_commit"] = git("rev-parse", upstream).decode().strip()
    if output is not None:
        output.write_text(json.dumps(result, ensure_ascii=False, indent=2) + "\n",
                          encoding="utf-8")
    return result


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


def assert_sensitive_local_control_defaults_disabled() -> None:
    try:
        schema = CONFIGURATION_SCHEMA.read_text(encoding="utf-8")
    except OSError as error:
        raise SystemExit(f"Community configuration schema is unavailable: {error}") from error
    required = (
        '{u"system/launch_as_administrator", booleanDefault(false), ConfigurationValueKind::Boolean}',
        '{u"mcp/enabled", booleanDefault(false), ConfigurationValueKind::Boolean}',
    )
    missing = [entry for entry in required if entry not in schema]
    if missing:
        raise SystemExit(
            "Sensitive local-control defaults must remain fail-closed in Community: " +
            ", ".join(missing)
        )


def assert_privacy_persistence_defaults_disabled() -> None:
    try:
        schema = CONFIGURATION_SCHEMA.read_text(encoding="utf-8")
    except OSError as error:
        raise SystemExit(f"Community configuration schema is unavailable: {error}") from error
    required = (
        '{u"system/auto_start_at_boot", booleanDefault(false), ConfigurationValueKind::Boolean}',
        '{u"capture_history/enabled", booleanDefault(false), ConfigurationValueKind::Boolean}',
        '{u"pinned_history/enabled", booleanDefault(false), ConfigurationValueKind::Boolean}',
    )
    missing = [entry for entry in required if entry not in schema]
    if missing:
        raise SystemExit(
            "Community persistence defaults must remain opt-in: " + ", ".join(missing)
        )


def assert_workflow_actions_pinned() -> None:
    violations: list[str] = []
    for path in sorted(WORKFLOW_ROOT.glob("*.y*ml")):
        content = path.read_text(encoding="utf-8")
        for match in ACTION_USE.finditer(content):
            action, reference = match.groups()
            if action.startswith("./"):
                continue
            if re.fullmatch(r"[0-9a-fA-F]{40}", reference) is None:
                line = content.count("\n", 0, match.start()) + 1
                violations.append(f"{path.relative_to(ROOT).as_posix()}:{line}: {action}@{reference}")
    if violations:
        raise SystemExit(
            "External GitHub Actions must be pinned to immutable 40-character commit SHAs:\n" +
            "\n".join(violations)
        )


def assert_deleted_files_absent() -> None:
    policy = json.loads(POLICY.read_text(encoding="utf-8"))
    for path in policy["deleted_files"]:
        if (ROOT / path).exists() or subprocess.run(
            ["git", "ls-files", "--error-unmatch", path],
            cwd=ROOT, capture_output=True
        ).returncode == 0:
            raise SystemExit(f"Previously removed commercial file was restored: {path}")
    assert_forbidden_runtime_content_absent()
    assert_community_update_signers_disabled()
    assert_sensitive_local_control_defaults_disabled()
    assert_privacy_persistence_defaults_disabled()
    assert_workflow_actions_pinned()


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("command", choices=["classify", "verify-deletions"])
    parser.add_argument("--upstream", default="upstream/main")
    parser.add_argument("--output", type=Path)
    args = parser.parse_args()
    if args.command == "verify-deletions":
        assert_deleted_files_absent()
        print("Commercial deletion guard: PASS")
    else:
        print(json.dumps(classify(args.upstream, args.output), ensure_ascii=False))


if __name__ == "__main__":
    main()
