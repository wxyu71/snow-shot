#!/usr/bin/env python3
"""Fail-fast packaging toolchain contract, without requiring Windows or NSIS."""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[1]
WORKFLOW = ROOT / ".github/workflows/community-windows-x64.yml"
BOOTSTRAP = ROOT / "scripts/bootstrap-community-nsis.ps1"
PACKAGE = ROOT / "scripts/package-snow-shot.ps1"


class CommunityNsisToolchainContract(unittest.TestCase):
    def test_windows_tooling_smoke_precedes_every_expensive_step(self):
        workflow = WORKFLOW.read_text(encoding="utf-8")
        self.assertIn("needs: [preflight, verify-paddleocr-contract]", workflow)
        markers = (
            "Bootstrap pinned NSIS 3.12 and run CPack smoke test",
            "Restore verified vcpkg binary archive cache",
            "Install pinned vcpkg, Rust and OCR build dependencies",
            "Build full Windows x64 (Mini disabled in release preset)",
            "Package full Windows x64 NSIS installers only",
            "Verify native installer manifests and SHA-256 files",
        )
        positions = [workflow.index(marker) for marker in markers]
        self.assertEqual(positions, sorted(positions))
        self.assertIn("run: ./scripts/bootstrap-community-nsis.ps1", workflow)
        self.assertIn("run: ./scripts/package-snow-shot.ps1 -Architecture x64 -SkipBuild -FullOnly", workflow)
        self.assertIn("python3 scripts/test-community-nsis-toolchain.py", workflow)
        # Packaging success must still require complete installer artifact guards.
        self.assertIn("python scripts/community_release.py verify-assets", workflow)
        self.assertIn("needs: [build-full-installer, verify-paddleocr-contract]", workflow)

    def test_nsis_installation_and_real_cpack_smoke_are_fail_closed(self):
        script = BOOTSTRAP.read_text(encoding="utf-8")
        for text in (
            "$version = '3.12.0'",
            "install nsis",
            "if ($LASTEXITCODE -ne 0)",
            "Get-Command choco.exe -ErrorAction Stop",
            "makensis.exe",
            "-VERSION",
            "CPackConfig.cmake",
            "-G NSIS",
            "if ($installers.Count -ne 1",
            "throw 'CPack NSIS smoke packaging failed.'",
        ):
            self.assertIn(text, script)
        packaging = PACKAGE.read_text(encoding="utf-8")
        self.assertIn("NSIS compiler 'makensis' was not found", packaging)
        self.assertIn("cpack --config $variantConfig -G NSIS", packaging)


if __name__ == "__main__":
    unittest.main(verbosity=2)
