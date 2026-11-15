#!/usr/bin/env python3
"""Fail-fast FullOnly/Store/Mini packaging contract (no Qt or Windows needed)."""
import json
from pathlib import Path
import re
import unittest

ROOT = Path(__file__).resolve().parents[1]
PACKAGE = ROOT / "scripts/package-snow-shot.ps1"
WINDOWS = ROOT / ".github/workflows/community-windows-x64.yml"
SAFETY = ROOT / ".github/workflows/community-static-checks.yml"
PRESET = ROOT / "CMakePresets.json"


class CommunityFullOnlyPackagingTests(unittest.TestCase):
    def test_full_only_release_preset_and_single_mini_decision(self):
        presets = json.loads(PRESET.read_text(encoding="utf-8"))
        release = [p for p in presets["configurePresets"] if p["name"] == "snow-shot-msvc-release"]
        self.assertEqual(len(release), 1)
        self.assertEqual(release[0]["cacheVariables"]["SNOW_APPS_BUILD_SNOW_SHOT_MINI"], "OFF")
        self.assertEqual(release[0]["cacheVariables"]["SNOW_APPS_PACKAGE_SNOW_SHOT"], "ON")

        script = PACKAGE.read_text(encoding="utf-8")
        self.assertEqual(script.count("$miniEnabled = -not ($store -or $FullOnly)"), 1)
        self.assertEqual(
            script.count(
                '"SNOW_APPS_BUILD_SNOW_SHOT_MINI:BOOL=$(if ($miniEnabled) { '
                "'ON' } else { 'OFF' })\""
            ),
            1,
        )
        self.assertRegex(
            script,
            r"(?s)\[string\[\]\]\$releaseTargets = if \(\$miniEnabled\) "
            r"\{\s*@\('snow_shot', 'snow_shot_mini'\)\s*\}"
            r" else \{\s*@\('snow_shot'\)\s*\}",
        )
        self.assertNotIn(
            "SNOW_APPS_BUILD_SNOW_SHOT_MINI:BOOL=$(if ($store)", script
        )
        self.assertIn("if (-not $FullOnly)", script)
        self.assertIn("Release cache is not production-safe; missing", script)

    def test_all_four_store_full_only_combinations(self):
        # The PowerShell job evaluates the expressions extracted from the
        # actual package script. Keep a cheap Python truth-table check too.
        for store, full_only, expected_mini in [
            (False, False, True),
            (False, True, False),
            (True, False, False),
            (True, True, False),
        ]:
            with self.subTest(store=store, full_only=full_only):
                mini_enabled = not (store or full_only)
                self.assertEqual(mini_enabled, expected_mini)

    def test_real_powershell_preflight_before_native_compile(self):
        workflow = WINDOWS.read_text(encoding="utf-8")
        self.assertIn("python3 scripts/test-community-package-modes.py", workflow)
        self.assertIn(
            "run: ./scripts/test-community-package-mode.ps1", workflow
        )
        self.assertLess(
            workflow.index("Run actual FullOnly/Store/Mini packaging mode tests"),
            workflow.index("Install pinned vcpkg, Rust and OCR build dependencies"),
        )
        self.assertIn("python3 scripts/test-community-package-modes.py",
                      SAFETY.read_text(encoding="utf-8"))
        ps = (ROOT / "scripts/test-community-package-mode.ps1").read_text(encoding="utf-8")
        for expected in ("$modeExpression", "$cacheExpression", "$targetsExpression",
                         "standalone FullOnly community beta", "Store channel",
                         "standalone Full+Mini legacy"):
            self.assertIn(expected, ps)

    def test_native_cache_is_saved_before_packaging_failure(self):
        workflow = WINDOWS.read_text(encoding="utf-8")
        self.assertIn("Restore cached native CMake and Rust build outputs", workflow)
        self.assertIn("Save successful native build before packaging", workflow)
        self.assertLess(
            workflow.index("Save successful native build before packaging"),
            workflow.index("Package full Windows x64 NSIS installers only"),
        )
        self.assertIn("github.event_name == 'pull_request'", workflow)
        self.assertIn("build/snow-shot-msvc-release", workflow)
        self.assertIn("run: ./scripts/build.ps1 -Preset snow-shot-msvc-release -SkipBootstrap", workflow)
        self.assertIn("verify-assets --directory dist", workflow)


    def test_real_cpack_contract_runs_before_native_build(self):
        workflow = WINDOWS.read_text(encoding="utf-8")
        package = PACKAGE.read_text(encoding="utf-8")
        policy = (ROOT / "scripts/assert-community-release-config.ps1").read_text(encoding="utf-8")
        build = (ROOT / "scripts/build.ps1").read_text(encoding="utf-8")
        self.assertLess(
            workflow.index("Check actual CPack and FullOnly release configuration"),
            workflow.index("Build full Windows x64 (Mini disabled in release preset)"),
        )
        self.assertIn("-SkipBootstrap -ConfigureOnly", workflow)
        self.assertIn("[switch]$ConfigureOnly", build)
        self.assertIn("assert-community-release-config.ps1", package)
        self.assertIn("CPACK_PACKAGE_HOMEPAGE_URL = $homepage", policy)
        self.assertIn("https://github.com/wxyu71/snow-shot", policy)
        self.assertNotIn('CPACK_PACKAGE_HOMEPAGE_URL = "https://snowshot.top"', package)

    def test_stable_native_cache_keys_and_release_trigger(self):
        workflow = WINDOWS.read_text(encoding="utf-8")
        self.assertLess(
            workflow.index("Freeze native cache key before build-side file generation"),
            workflow.index("Install pinned vcpkg, Rust and OCR build dependencies"),
        )
        self.assertEqual(workflow.count("key: ${{ steps.native-build-key.outputs.key }}"), 2)
        self.assertIn("restore-keys:", workflow)
        self.assertIn("community-windows-native-x64-v2-", workflow)
        self.assertIn(
            "if: github.event_name == 'push' && startsWith(github.ref, 'refs/tags/')",
            workflow,
        )
        self.assertNotIn("if: startsWith(github.ref, 'refs/tags/')", workflow)

    def test_release_build_presets_exclude_disabled_mini(self):
        presets = json.loads(PRESET.read_text(encoding="utf-8"))
        for name in ("build-snow-shot-msvc-release", "build-snow-shot-msvc-fast"):
            entries = [p for p in presets["buildPresets"] if p["name"] == name]
            self.assertEqual(len(entries), 1)
            self.assertEqual(entries[0]["targets"], ["snow_shot"])


if __name__ == "__main__":
    unittest.main(verbosity=2)
