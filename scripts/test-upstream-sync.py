#!/usr/bin/env python3
"""Unit tests for automatic upstream reconciliation safety policy."""
import json
import os
import pathlib
import re
import shutil
import subprocess
import sys
import tempfile
import textwrap
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "scripts"))
import upstream_sync  # noqa: E402


class UpstreamSyncGuardTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.policy = json.loads(upstream_sync.POLICY.read_text(encoding="utf-8"))

    def decision(self, incoming, local=(), patch=False):
        return upstream_sync.decide(set(incoming), set(local), self.policy, patch)

    def test_empty_upstream_is_noop(self):
        self.assertEqual(self.decision([])["mode"], "noop")

    def test_unrelated_safe_documentation_is_auto(self):
        self.assertEqual(
            self.decision(["docs/upstream-release-notes.md"])["mode"],
            "auto"
        )

    def test_arbitrary_text_file_is_not_treated_as_documentation(self):
        self.assertEqual(self.decision(["cmake/toolchain.txt"])["mode"], "review")

    def test_unrelated_source_requires_normal_pr_review(self):
        result = self.decision(["snow_shot/src/presentation/selection/safe_selection.cpp"])
        self.assertEqual(result["mode"], "review")
        self.assertEqual(result["review_required"], ["snow_shot/src/presentation/selection/safe_selection.cpp"])

    def test_build_workflow_cannot_silently_auto_merge(self):
        self.assertEqual(
            self.decision([".github/workflows/release.yml"])["mode"], "review"
        )

    def test_overlapping_modifications_require_review(self):
        self.assertEqual(
            self.decision(["snow_shot/src/presentation/same.cpp"],
                          ["snow_shot/src/presentation/same.cpp"])["mode"],
            "review"
        )

    def test_deleted_account_file_cannot_reappear(self):
        self.assertEqual(
            self.decision(["snow_shot/src/app/accountsessionservice.cpp"])["mode"],
            "review"
        )

    def test_protected_cloud_ocr_file_requires_review(self):
        self.assertEqual(
            self.decision(["snow_shot/src/network/baidupaddleocrclient.cpp"])["mode"],
            "review"
        )

    def test_commercial_feature_in_new_file_requires_review(self):
        self.assertEqual(
            self.decision(["snow_shot/src/app/billing_gateway.cpp"])["mode"], "review"
        )

    def test_commercial_content_in_innocent_path_requires_review(self):
        self.assertEqual(
            self.decision(["snow_shot/src/presentation/new_ui.cpp"], patch=True)["mode"],
            "review"
        )

    def test_release_configuration_is_protected(self):
        self.assertEqual(self.decision(["CMakeLists.txt"])["mode"], "review")


    def test_sync_workflow_reuses_review_pr_and_handles_pr_restriction(self):
        workflow = (ROOT / ".github" / "workflows" / "upstream-sync.yml").read_text(
            encoding="utf-8"
        )
        self.assertIn('startswith("review/upstream-main-")', workflow)
        self.assertIn('.headRefName == "automation/upstream-review"', workflow)
        self.assertLess(
            workflow.index("existing_review="),
            workflow.index('git switch -C "$branch"'),
        )  # Do not force-push over a PR that is being resolved manually.
        self.assertIn("GitHub Actions is not permitted to create or approve pull requests", workflow)
        self.assertIn("GITHUB_STEP_SUMMARY", workflow)
        self.assertIn("pull/new/${branch}", workflow)
        self.assertIn("No changes were merged into main.", workflow)
        self.assertIn("exit 1", workflow)  # Unexpected PR failures remain fatal.

        # Shared Community policy is authoritative; no moving Action tags or
        # official repository identity hardcoded into a distributable workflow.
        self.assertRegex(workflow, r"uses:\s+actions/checkout@[0-9a-f]{40}\\b")
        self.assertNotIn("mg-chao/snow-shot", workflow)
        self.assertIn('upstream_repo="$(python3 -c', workflow)
        self.assertIn('git remote add upstream "https://github.com/${upstream_repo}.git"', workflow)

        self.assertIn('git switch -C "$branch" upstream/main', workflow)
        self.assertIn('git diff --name-only --diff-filter=U', workflow)
        self.assertIn('git push origin "HEAD:refs/heads/$branch"', workflow)
        self.assertIn('git ls-remote --exit-code --heads origin "$branch"', workflow)
        self.assertIn('automation/upstream-review-${upstream_sha:0:12}-${base_sha:0:8}', workflow)
        self.assertNotIn('git push --force origin', workflow)
        self.assertIn('upstream-review-pending.md', workflow)
        self.assertIn('refusing to allow a GitHub App to create or update workflow', workflow)

    def test_upstream_review_candidate_is_real_and_retries_preserve_it(self):
        workflow = (ROOT / ".github" / "workflows" / "upstream-sync.yml").read_text(
            encoding="utf-8"
        )
        script = textwrap.dedent(workflow.split(
            "      - name: Apply safe sync or update the review candidate\n", 1
        )[1].split("        run: |\n", 1)[1])
        subprocess.run(["bash", "-n"], input=script, text=True, check=True)

        for overlapping, blocked in ((False, False), (True, False), (True, True)):
            with self.subTest(conflict=overlapping, token_blocked=blocked), tempfile.TemporaryDirectory() as temp:
                base = pathlib.Path(temp)
                work, bare = base / "work", base / "origin.git"
                subprocess.run(["git", "init", "-q", "--bare", str(bare)], check=True)
                subprocess.run(["git", "init", "-q", "-b", "main", str(work)], check=True)

                def git(*args):
                    return subprocess.check_output(
                        ["git", *args], cwd=work, text=True, stderr=subprocess.PIPE
                    ).strip()

                git("config", "user.name", "CI test")
                git("config", "user.email", "ci-test@example.invalid")
                (work / ".github").mkdir()
                (work / ".github" / "keep.txt").write_text("baseline\n", encoding="utf-8")
                (work / "shared.txt").write_text("base\n", encoding="utf-8")
                git("add", ".")
                git("commit", "-m", "common ancestor")
                git("remote", "add", "origin", str(bare))
                git("push", "-u", "origin", "main")
                git("switch", "-c", "official")
                (work / "shared.txt").write_text("official\n", encoding="utf-8")
                git("add", ".")
                git("commit", "-m", "official upstream")
                upstream_sha = git("rev-parse", "HEAD")
                git("update-ref", "refs/remotes/upstream/main", upstream_sha)
                git("switch", "main")
                local_file = "shared.txt" if overlapping else "community.txt"
                (work / local_file).write_text("community\n", encoding="utf-8")
                git("add", ".")
                git("commit", "-m", "community customization")
                main_sha = git("rev-parse", "HEAD")
                git("push", "origin", "main")

                mock_bin = base / "bin"
                mock_bin.mkdir()
                mock_gh = mock_bin / "gh"
                mock_gh.write_text(
                    '#!/usr/bin/env bash\n'
                    'if [[ "$1 $2" == "pr list" ]]; then\n'
                    '  printf "%s" "$MOCK_REVIEW_PR"\n'
                    'elif [[ "$1 $2" == "pr create" ]]; then\n'
                    '  echo create >> "$MOCK_EVENT_FILE"\n'
                    '  echo "https://github.com/example/fork/pull/42"\n'
                    'else exit 99; fi\n',
                    encoding="utf-8",
                )
                mock_gh.chmod(0o755)
                mock_git = mock_bin / "git"
                mock_git.write_text(
                    '#!/usr/bin/env bash\n'
                    'if [[ "$1" == "push" && "$MOCK_BLOCK_WORKFLOWS" == "1" '
                    '&& ! -f "$MOCK_BLOCKED_FILE" ]]; then\n'
                    '  touch "$MOCK_BLOCKED_FILE"\n'
                    '  echo "remote: refusing to allow a GitHub App to create or '
                    'update workflow .github/workflows/example.yml without workflows permission" >&2\n'
                    '  exit 1\n'
                    'fi\n'
                    'exec "' + str(shutil.which("git")) + '" "$@"\n',
                    encoding="utf-8",
                )
                mock_git.chmod(0o755)
                sync_file = base / "sync.json"
                sync_file.write_text(json.dumps({
                    "upstream_commit": upstream_sha,
                    "reasons": ["manual review requested"],
                }), encoding="utf-8")
                events = base / "events"
                env = dict(os.environ)
                env.update({
                    "PATH": str(mock_bin) + os.pathsep + env["PATH"],
                    "RUNNER_TEMP": str(base),
                    "GITHUB_STEP_SUMMARY": str(base / "summary.md"),
                    "GITHUB_REPOSITORY": "example/fork",
                    "GH_TOKEN": "test",
                    "CLASSIFIED_MODE": "review",
                    "MOCK_REVIEW_PR": "",
                    "MOCK_EVENT_FILE": str(events),
                    "MOCK_BLOCK_WORKFLOWS": "1" if blocked else "0",
                    "MOCK_BLOCKED_FILE": str(base / "push-blocked"),
                })
                subprocess.run(
                    ["bash", "-c", script], cwd=work, env=env,
                    capture_output=True, text=True, check=True
                )
                review_ref = (
                    "refs/heads/automation/upstream-review-"
                    + upstream_sha[:12] + "-" + main_sha[:8]
                )
                review_sha = git("--git-dir=" + str(bare), "rev-parse", review_ref)
                self.assertEqual(git("--git-dir=" + str(bare), "rev-parse", "main"), main_sha)
                if blocked:
                    self.assertNotEqual(
                        subprocess.run(
                            ["git", "merge-base", "--is-ancestor", upstream_sha, review_sha],
                            cwd=work, capture_output=True
                        ).returncode, 0
                    )
                    self.assertIn(
                        "Unresolved upstream integration",
                        git("show", review_sha + ":.github/upstream-review-pending.md")
                    )
                else:
                    self.assertEqual(
                        git("merge-base", review_sha, upstream_sha), upstream_sha
                    )
                    if overlapping:
                        self.assertEqual(review_sha, upstream_sha)
                    else:
                        self.assertEqual(
                            len(git("rev-list", "--parents", "-n1", review_sha).split()), 3
                        )

                # A subsequent scheduled run must reuse the PR, not replace its
                # branch or create a new PR.
                env["MOCK_REVIEW_PR"] = "42"
                subprocess.run(
                    ["bash", "-c", script], cwd=work, env=env,
                    capture_output=True, text=True, check=True
                )
                self.assertEqual(
                    git("--git-dir=" + str(bare), "rev-parse", review_ref), review_sha
                )
                self.assertEqual(events.read_text(encoding="utf-8"), "create\n")


    def test_review_pr_requires_upstream_ancestry(self):
        # Runs as a PR preflight check. A marker-only PR can never pass simply
        # by deleting its conflict report: it must contain the actual target.
        head_ref = os.environ.get("GITHUB_HEAD_REF", "")
        if not head_ref.startswith("automation/upstream-review-"):
            return
        match = re.fullmatch(
            r"automation/upstream-review-([0-9a-f]{12})-[0-9a-f]{8}",
            head_ref,
        )
        self.assertIsNotNone(match, "Invalid automated upstream review branch")
        self.assertFalse(
            (ROOT / ".github/upstream-review-pending.md").exists(),
            "Resolve the upstream merge and remove the pending marker first",
        )
        target = match.group(1)
        result = subprocess.run(
            ["git", "merge-base", "--is-ancestor", target, "HEAD"],
            cwd=ROOT, capture_output=True, text=True,
        )
        self.assertEqual(
            result.returncode, 0,
            "Upstream review must contain the real upstream commit " + target
            + "; a report-only branch must not be merged",
        )


if __name__ == "__main__":
    unittest.main(verbosity=2)
