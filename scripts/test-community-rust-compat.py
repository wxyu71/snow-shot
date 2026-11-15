#!/usr/bin/env python3
"""Preflight check for the deprecated atomic API that failed Windows CI #25/#26."""
from pathlib import Path
import re
import subprocess
import unittest

ROOT = Path(__file__).resolve().parents[1]
CAPTURE = ROOT / "snow-crates/crates/snow-capture/src/follow_mouse.rs"
AUDIO = ROOT / "snow-crates/crates/snow-audio-recorder/src/control.rs"
WORKFLOW = ROOT / ".github/workflows/community-windows-x64.yml"


class RustAtomicCompatibility(unittest.TestCase):
    def test_all_git_tracked_rust_sources_avoid_deprecated_atomic_method(self):
        # Scan every tracked .rs file, not merely the first two failing crates.
        # Using git ls-files excludes downloaded dependencies and submodule internals.
        names = subprocess.check_output(
            ["git", "ls-files", "-z", "--", "*.rs"], cwd=ROOT
        ).decode("utf-8").split("\0")
        paths = [ROOT / name for name in names if name]
        self.assertGreater(len(paths), 400, "Rust source scan must cover the repository")
        outdated = []
        for path in paths:
            content = path.read_text(encoding="utf-8")
            for match in re.finditer(r"\.fetch_update\s*\(", content):
                line = content.count("\n", 0, match.start()) + 1
                outdated.append(f"{path.relative_to(ROOT)}:{line}")
        self.assertEqual(
            outdated, [],
            "Deprecated Atomic::fetch_update (Rust nightly warnings-as-errors); "
            "replace with Atomic::try_update while preserving memory order: "
            + ", ".join(outdated),
        )

    def test_capture_pause_transition_preserves_atomic_ordering(self):
        source = CAPTURE.read_text(encoding="utf-8")
        self.assertIn(
            ".try_update(Ordering::AcqRel, Ordering::Acquire, |revision| {\n"
            "                ((revision & 1 != 0) != paused).then(|| revision.wrapping_add(1))",
            source,
        )
        self.assertIn(".is_ok()", source)

    def test_audio_peak_and_status_transitions_preserve_atomic_ordering(self):
        source = AUDIO.read_text(encoding="utf-8")
        self.assertEqual(
            source.count("meter.try_update(Ordering::AcqRel, Ordering::Acquire, |old| {"),
            2,
        )
        self.assertIn("if AudioSourceStatus::from_word(old) == status", source)
        self.assertIn("if !self.metering_enabled(source)", source)

    def test_native_ci_is_triggered_by_rust_source_changes(self):
        workflow = WORKFLOW.read_text(encoding="utf-8")
        for tracked_tree in (
            "snow-crates/**",
            "snow-draw-engine-gpui/**",
            "snow_draw_engine_qt/**",
            "snow-image-rs/**",
            "snow_rust_ffi/**",
        ):
            self.assertIn(f"      - {tracked_tree}", workflow)
        self.assertIn("actions/cache/restore@v4", workflow)
        self.assertIn("actions/cache/save@v4", workflow)
        self.assertLess(
            workflow.index("Save audited static Qt kit"),
            workflow.index("Build full Windows x64"),
        )


    def test_native_windows_build_is_gated_and_skips_redundant_bootstrap(self):
        workflow = WORKFLOW.read_text(encoding="utf-8")
        self.assertIn("needs: [preflight, verify-paddleocr-contract]", workflow)
        self.assertIn("python3 scripts/test-community-cpp-api-contract.py", workflow)
        self.assertIn("run: ./scripts/build.ps1 -Preset snow-shot-msvc-release -SkipBootstrap", workflow)
        self.assertIn("VCPKG_BINARY_SOURCES=clear;files,$cache,readwrite", workflow)
        self.assertIn("Save vcpkg binary archives for subsequent builds", workflow)
        self.assertIn("verify-assets --directory dist", workflow)
        self.assertIn("contents: read", workflow)
        self.assertIn("contents: write", workflow)

if __name__ == "__main__":
    unittest.main(verbosity=2)
