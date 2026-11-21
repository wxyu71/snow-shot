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

    def test_removed_homebrew_workflow_cannot_reappear(self):
        path = ".github/workflows/snow-shot-homebrew-checks.yml"
        self.assertIn(path, self.policy["deleted_files"])
        self.assertFalse((ROOT / path).exists())
        self.assertEqual(self.decision([path])["mode"], "review")

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

    def test_update_trust_store_and_submodule_metadata_are_protected(self):
        for path in (
            "snow_shot/resources/update-trusted-keys.json",
            ".gitmodules",
            "snow-crates/crates/simd-paddle-ocr-rs",
            "snow-draw-engine-gpui/Cargo.toml",
            "snow-draw-engine-gpui/Cargo.lock",
        ):
            result = self.decision([path])
            self.assertEqual(result["mode"], "review")
            self.assertEqual(result["protected"], [path])

    def test_community_updater_has_no_trusted_remote_signer(self):
        upstream_sync.assert_community_update_signers_disabled()

    def test_sensitive_local_control_defaults_are_fail_closed(self):
        upstream_sync.assert_sensitive_local_control_defaults_disabled()

    def test_privacy_persistence_defaults_are_opt_in(self):
        upstream_sync.assert_privacy_persistence_defaults_disabled()

    def test_workflow_action_parser_detects_mutable_major_tag(self):
        match = upstream_sync.ACTION_USE.search("      - uses: actions/checkout@v4")
        self.assertIsNotNone(match)
        self.assertEqual(match.groups(), ("actions/checkout", "v4"))

    def test_workflow_actions_are_pinned_to_commit_shas(self):
        upstream_sync.assert_workflow_actions_pinned()

    def test_external_source_dependency_parser_rejects_mutable_refs(self):
        pinned = {
            "git": "https://example.invalid/dependency.git",
            "rev": "0123456789abcdef0123456789abcdef01234567",
        }
        self.assertTrue(upstream_sync.immutable_git_dependency(pinned))
        self.assertFalse(
            upstream_sync.immutable_git_dependency(
                {"git": pinned["git"], "branch": "main", "rev": pinned["rev"]}
            )
        )
        self.assertFalse(
            upstream_sync.immutable_git_dependency(
                {"git": pinned["git"], "rev": "v1.2.3"}
            )
        )

    def test_external_source_dependencies_are_immutable(self):
        upstream_sync.assert_external_source_pins()


    def test_local_control_privilege_boundaries_are_protected(self):
        for path in (
            "snow_shot/src/app/mcp/screenshotmcpserver.cpp",
            "snow_shot/src/app/singleinstancecoordinator.cpp",
            "snow_shot/src/platform/windows/administratorlaunch.cpp",
            "snow_shot/src/storage/configurationschema.cpp",
        ):
            result = self.decision([path])
            self.assertEqual(result["mode"], "review")
            self.assertEqual(result["protected"], [path])

    def test_upstream_workflow_reuses_manual_review_and_reports_pr_permission_limit(self):
        workflow = (ROOT / ".github" / "workflows" / "upstream-sync.yml").read_text(
            encoding="utf-8"
        )
        self.assertIn('startswith("review/upstream-main-")', workflow)
        self.assertIn("GITHUB_STEP_SUMMARY", workflow)
        self.assertIn("GitHub Actions is not permitted to create or approve pull requests", workflow)
        self.assertIn("No changes were merged into main.", workflow)
        self.assertIn("exit 1", workflow)  # Other PR creation errors still fail.


    def test_current_tree_has_no_account_telemetry_or_upstream_runtime_endpoints(self):
        upstream_sync.assert_forbidden_runtime_content_absent()

    def test_forbidden_content_guard_catches_upstream_identity_and_telemetry(self):
        forbidden = upstream_sync.FORBIDDEN_COMMUNITY_CONTENT
        for value in (
            "mg-chao/snow-shot",
            "mg-chao.snow-shot",
            "releases@snowshot.top",
            "120.79.232.67",
            "telemetry",
            "posthog",
        ):
            self.assertRegex(value, forbidden)
        self.assertNotRegex("const qreal amplitude = 1.0;", forbidden)


if __name__ == "__main__":
    unittest.main(verbosity=2)
