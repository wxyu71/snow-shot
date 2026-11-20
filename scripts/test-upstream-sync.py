#!/usr/bin/env python3
"""Unit tests for automatic upstream reconciliation safety policy."""
import json
import pathlib
import sys
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


if __name__ == "__main__":
    unittest.main(verbosity=2)
