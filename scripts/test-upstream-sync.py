#!/usr/bin/env python3
"""Policy and real Git-history regression tests for the rolling sync PR."""
from __future__ import annotations

import json
import pathlib
import subprocess
import sys
import tempfile
import unittest
from unittest.mock import patch

ROOT = pathlib.Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "scripts"))
import upstream_sync as sync  # noqa: E402


class ClassifierTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.policy = json.loads(sync.POLICY.read_text(encoding="utf-8"))

    def decision(self, incoming, local=(), patch=False):
        return sync.decide(set(incoming), set(local), self.policy, patch)

    def test_noop(self):
        self.assertEqual(self.decision([])["mode"], "noop")

    def test_unrelated_source_is_eligible_for_ci_gated_automatic_sync(self):
        self.assertEqual(self.decision(["snow_shot/src/selection/safe.cpp"])["mode"], "auto")

    def test_documentation_is_eligible(self):
        self.assertEqual(self.decision(["docs/upstream-notes.md"])["mode"], "auto")

    def test_workflows_require_manual_review(self):
        self.assertEqual(self.decision([".github/workflows/release.yml"])["mode"], "review")

    def test_same_file_without_git_conflict_still_requires_review(self):
        result = self.decision(["snow_shot/src/example.cpp"], ["snow_shot/src/example.cpp"])
        self.assertEqual(result["mode"], "review")
        self.assertTrue(result["overlap"])

    def test_community_ocr_is_protected(self):
        self.assertEqual(self.decision(["snow_shot/src/network/baidupaddleocrclient.cpp"])["mode"], "review")

    def test_deleted_account_code_is_protected(self):
        self.assertEqual(self.decision(["snow_shot/src/app/accountsessionservice.cpp"])["mode"], "review")

    def test_macos_login_item_not_commercial_account(self):
        self.assertEqual(self.decision(["snow_shot/tests/login_item_native_tests.mm"])["mode"], "auto")
        self.assertEqual(self.decision(["snow_shot/tests/login.cpp"])["mode"], "review")

    def test_commercial_content_requires_review(self):
        self.assertEqual(self.decision(["new_module.cpp"], patch=True)["mode"], "review")

    def test_release_config_is_protected(self):
        self.assertEqual(self.decision(["CMakePresets.json"])["mode"], "review")

    def test_body_preserves_human_text(self):
        meta = {"version": 1, "baseline": "a" * 40, "target": "b" * 40, "risk": "review"}
        section = sync.managed_section(meta, "start", ["risk"])
        body = "Human review notes\n\n" + section + "\n\nHuman checklist"
        self.assertEqual(sync.parse_metadata(body), meta)
        replacement = sync.managed_section(meta, "updated", [])
        new = sync.replace_managed(body, replacement)
        self.assertIn("Human review notes", new)
        self.assertIn("Human checklist", new)
        self.assertNotIn("start", new)
        self.assertIn("updated", new)

    def test_body_refuses_ambiguous_marker(self):
        with self.assertRaises(RuntimeError):
            sync.parse_metadata("GitHub text without bot section")


class GitIntegrationTests(unittest.TestCase):
    """Exercise actual git merge against diverged temporary repositories."""

    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.repo = pathlib.Path(self.temp.name) / "repo"
        self.repo.mkdir()
        self.sh("git", "init", "-b", "main")
        self.sh("git", "config", "user.email", "tester@example.com")
        self.sh("git", "config", "user.name", "Tester")
        self.write("shared.txt", "initial\n")
        self.write("local.txt", "original\n")
        self.sh("git", "add", ".")
        self.sh("git", "commit", "-m", "base")
        self.base = self.sh("git", "rev-parse", "HEAD")
        self.sh("git", "branch", "upstream")
        self.orig_root = sync.ROOT
        sync.ROOT = self.repo
        self.addCleanup(setattr, sync, "ROOT", self.orig_root)

    def sh(self, *args):
        return subprocess.check_output(args, cwd=self.repo, text=True, stderr=subprocess.STDOUT).strip()

    def write(self, path, data):
        target = self.repo / path
        target.parent.mkdir(parents=True, exist_ok=True)
        target.write_text(data, encoding="utf-8")

    def commit(self, path, data, message):
        self.write(path, data)
        self.sh("git", "add", "-A")
        self.sh("git", "commit", "-m", message)
        return self.sh("git", "rev-parse", "HEAD")

    def test_clean_merge_preserves_main_and_upstream_parents(self):
        self.sh("git", "switch", "upstream")
        upstream = self.commit("new-source.cpp", "upstream\n", "official source")
        self.sh("git", "switch", "main")
        main = self.commit("local.txt", "Community\n", "Community feature")
        ok, conflicts = sync.merge_candidate(upstream)
        self.assertTrue(ok)
        self.assertEqual(conflicts, [])
        head = sync.git("rev-parse", "HEAD")
        self.assertTrue(sync.ancestor(main, head))
        self.assertTrue(sync.ancestor(upstream, head))
        self.assertEqual(len(sync.git("rev-list", "--parents", "-n", "1", "HEAD").split()), 3)

    def test_first_conflict_does_not_create_fake_merge(self):
        self.sh("git", "switch", "upstream")
        upstream = self.commit("shared.txt", "official change\n", "official shared change")
        self.sh("git", "switch", "main")
        main = self.commit("shared.txt", "Community change\n", "Community shared change")
        ok, conflicts = sync.merge_candidate(upstream)
        self.assertFalse(ok)
        self.assertEqual(conflicts, ["shared.txt"])
        self.assertEqual(sync.git("rev-parse", "HEAD"), main)
        self.assertFalse(sync.ancestor(upstream, "HEAD"))
        self.assertFalse((self.repo / ".git" / "MERGE_HEAD").exists())
        self.sh("git", "switch", "--detach", upstream)
        self.assertFalse(sync.ancestor(main, "HEAD"))
        # The upstream-only tree must not pass PR ancestry validation.
        policy = {"deleted_files": []}
        with patch.object(sync, "assert_deleted_files_absent", return_value=None):
            metadata = {"version": 1, "baseline": main, "target": upstream, "risk": "review"}
            body = sync.managed_section(metadata, "needs resolution", [])
            with self.assertRaisesRegex(RuntimeError, "lacks original Community baseline"):
                sync.validate_pr(body, upstream, main)

    def test_rolling_merges_append_without_rewriting_human_resolution(self):
        self.sh("git", "switch", "upstream")
        upstream1 = self.commit("new-source.cpp", "first upstream\n", "official one")
        self.sh("git", "switch", "main")
        main1 = self.commit("local.txt", "Community\n", "Community feature")
        ok, _ = sync.merge_candidate(upstream1)
        self.assertTrue(ok)
        integrated = sync.git("rev-parse", "HEAD")
        manual = self.commit("community-human.txt", "manually resolved\n", "human amendment")
        self.sh("git", "branch", "integration")
        self.sh("git", "switch", "upstream")
        upstream2 = self.commit("new-feature.cpp", "second upstream\n", "official two")
        self.sh("git", "switch", "integration")
        ok, _ = sync.merge_candidate(upstream2)
        self.assertTrue(ok)
        self.assertTrue(sync.ancestor(manual, "HEAD"))
        self.assertTrue(sync.ancestor(integrated, "HEAD"))
        self.assertTrue(sync.ancestor(upstream2, "HEAD"))
        self.assertEqual((self.repo / "community-human.txt").read_text(), "manually resolved\n")

    def test_upstream_delta_after_prior_merge_is_not_reprocessed(self):
        self.sh("git", "switch", "upstream")
        upstream1 = self.commit("one.cpp", "1\n", "official one")
        self.sh("git", "switch", "main")
        self.commit("local.txt", "Community\n", "local")
        sync.merge_candidate(upstream1)
        self.sh("git", "branch", "integration")
        self.sh("git", "switch", "upstream")
        upstream2 = self.commit("two.cpp", "2\n", "official two")
        self.sh("git", "switch", "integration")
        base = sync.git("merge-base", "HEAD", upstream2)
        self.assertEqual(base, upstream1)
        self.assertEqual(sync.changed_paths(base, upstream2), {"two.cpp"})


if __name__ == "__main__":
    unittest.main(verbosity=2)
