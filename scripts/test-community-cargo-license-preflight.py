#!/usr/bin/env python3
"""Keep release-license metadata complete, offline and before native compilation."""
from pathlib import Path
import re
import unittest

ROOT = Path(__file__).resolve().parents[1]
WORKFLOW = ROOT / ".github/workflows/community-windows-x64.yml"
PREFLIGHT = ROOT / "scripts/preflight-community-release-licenses.ps1"
PLAN = ROOT / "scripts/snow-community-release-cargo-licenses.ps1"
COLLECTOR = ROOT / "scripts/collect-third-party-licenses.ps1"
PACKAGE = ROOT / "scripts/package-snow-shot.ps1"


class OfflineReleaseLicensePreflightTests(unittest.TestCase):
    def test_shared_manifest_and_feature_plan_in_real_collector(self):
        plan = PLAN.read_text(encoding="utf-8")
        package = PACKAGE.read_text(encoding="utf-8")
        preflight = PREFLIGHT.read_text(encoding="utf-8")
        for manifest in (
            "snow_rust_ffi/Cargo.toml",
            "snow-crates/crates/snow-ocr-process/Cargo.toml",
            "snow_shot/rust/snow-shot-mcp/Cargo.toml",
            "snow_shot/rust/snow-shot-updater/Cargo.toml",
        ):
            self.assertIn(manifest, plan)
        self.assertIn("'selected-text,language-detection'", plan)
        self.assertIn("'crash-diagnostics,gpu'", plan)
        self.assertIn("'--no-default-features'", plan)
        self.assertIn("if (-not $Store)", plan)
        for source in (package, preflight):
            self.assertIn("snow-community-release-cargo-licenses.ps1", source)
            self.assertIn("Get-SnowShotReleaseCargoLicensePlan", source)
            self.assertIn("-CargoManifest $licensePlan.Manifests" if source == package
                          else "CargoManifest = $plan.Manifests", source)
        self.assertIn("collect-third-party-licenses.ps1", preflight)

    def test_locked_all_target_fetch_then_exact_offline_audit(self):
        source = PREFLIGHT.read_text(encoding="utf-8")
        collector = COLLECTOR.read_text(encoding="utf-8")
        self.assertIn("'fetch', '--locked', '--manifest-path', $manifest", source)
        self.assertIn("& $cargo @fetchArgs", source)
        self.assertNotRegex(source, r"fetch\s+--locked\s+--target")
        self.assertIn("'metadata', '--locked', '--offline', '--filter-platform', $target", source)
        self.assertIn("& $cargo @metadataArgs", source)
        self.assertIn("'tree', '--locked', '--offline', '--target', $target", source)
        self.assertIn("& $cargo @treeArgs", source)
        self.assertIn("'--edges', 'normal,build'", source)
        self.assertIn("Get-SnowOcrToolchain", source)
        self.assertIn("collect-third-party-licenses.ps1", source)
        self.assertIn("Full release license collection passed before native build", source)
        self.assertIn("metadata --locked --offline", collector)
        self.assertIn("tree --locked --offline", collector)
        self.assertNotIn("--no-deps", collector)
        self.assertIn("Resolved Rust packages are missing distributable license files", collector)

    def test_ocr_toolchain_pin_is_explicit_and_independent_of_matches(self):
        pin = (ROOT / "scripts/snow-ocr-toolchain.ps1").read_text(encoding="utf-8")
        smoke = (ROOT / "scripts/test-community-ocr-toolchain-pin.ps1").read_text(encoding="utf-8")
        preflight = PREFLIGHT.read_text(encoding="utf-8")
        workflow = WORKFLOW.read_text(encoding="utf-8")
        self.assertIn("[regex]::Match", pin)
        self.assertIn("return $match.Groups[1].Value", pin)
        self.assertNotIn("return $Matches[1]", pin)
        self.assertIn("Remove-Variable Matches", smoke)
        self.assertIn("Get-SnowOcrToolchain", smoke)
        self.assertIn("Invalid pinned OCR toolchain", preflight)
        self.assertIn("Verified pinned OCR Cargo toolchain", preflight)
        self.assertIn("run: ./scripts/test-community-ocr-toolchain-pin.ps1", workflow)
        self.assertLess(
            workflow.index("Validate pinned OCR nightly toolchain without stale Matches"),
            workflow.index("Install pinned vcpkg, Rust and OCR build dependencies"),
        )

    def test_windows_preflight_first_and_cached_native_preserved(self):
        workflow = WORKFLOW.read_text(encoding="utf-8")
        keys = (
            "Build or validate audited Qt 6.12.0 Release kit",
            "Prefetch full locked Rust license graph and collect notices offline",
            "Save complete Cargo sources after release license preflight",
            "Restore cached native CMake and Rust build outputs",
            "Build full Windows x64 (Mini disabled in release preset)",
            "Save successful native build before packaging",
            "Package full Windows x64 NSIS installers only",
            "Verify native installer manifests and SHA-256 files",
        )
        missing = [marker for marker in keys if marker not in workflow]
        self.assertEqual(missing, [], "Windows release gating steps disappeared: " + ", ".join(missing))
        positions = [workflow.index(marker) for marker in keys]
        self.assertEqual(positions, sorted(positions))
        self.assertIn("run: ./scripts/preflight-community-release-licenses.ps1", workflow)
        self.assertIn("community-cargo-license-v3-x64-", workflow)
        self.assertIn("community-cargo-license-v2-x64-", workflow)  # previous partial fallback
        self.assertIn("community-cargo-x64-", workflow)  # old cache fallback
        self.assertNotIn("community-windows-native-x64-v1-", workflow)
        self.assertIn("python3 scripts/test-community-cargo-license-preflight.py", workflow)
        self.assertEqual(workflow.count("python3 scripts/test-community-cargo-license-preflight.py"), 1)
        self.assertIn("needs: [changes, preflight, verify-paddleocr-contract]", workflow)
        self.assertIn("needs: [build-full-installer]", workflow)
        self.assertIn("verify-assets --directory dist", workflow)


if __name__ == "__main__":
    unittest.main(verbosity=2)
