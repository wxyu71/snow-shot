#!/usr/bin/env python3
"""Guard against reintroducing the Rust 1.99+ deprecated atomic call that broke CI #25."""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / "snow-crates/crates/snow-capture/src/follow_mouse.rs"
WORKFLOW = ROOT / ".github/workflows/community-windows-x64.yml"


class RustPauseRevisionCompatibility(unittest.TestCase):
    def test_atomic_pause_transition_is_not_deprecated(self):
        source = SOURCE.read_text(encoding="utf-8")
        self.assertNotIn(".fetch_update(", source)
        self.assertIn(
            ".try_update(Ordering::AcqRel, Ordering::Acquire, |revision| {\n"
            "                ((revision & 1 != 0) != paused).then(|| revision.wrapping_add(1))",
            source,
        )
        self.assertIn(".is_ok()", source)

    def test_native_ci_is_triggered_by_capture_changes(self):
        workflow = WORKFLOW.read_text(encoding="utf-8")
        self.assertIn("      - snow-crates/**", workflow)
        self.assertIn("actions/cache/restore@v4", workflow)
        self.assertIn("actions/cache/save@v4", workflow)
        self.assertLess(
            workflow.index("Save audited static Qt kit"),
            workflow.index("Build full Windows x64"),
        )


if __name__ == "__main__":
    unittest.main(verbosity=2)
