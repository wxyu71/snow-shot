#!/usr/bin/env python3
"""Fast full-tree C++ API guard before expensive Windows CI builds.

Scan every Git-tracked C++/Objective-C++ source for qualified references to
SnowShotApiClient that are absent from its own public class declaration.
This catches stale signal connections after removing the account subsystem.
Native C++ compilation remains mandatory; this is only a cheap preflight.
"""
from pathlib import Path
import re
import subprocess
import unittest

ROOT = Path(__file__).resolve().parents[1]
CLIENT_HEADER = ROOT / "snow_shot/include/snow_shot/network/snowshotapiclient.h"
TRANSLATION = ROOT / "snow_shot/src/app/translationservice.cpp"
MEMBER_REFERENCE = re.compile(r"\bSnowShotApiClient::([A-Za-z_]\w*)\b")


def tracked_cpp_files():
    result = subprocess.check_output(
        ["git", "ls-files", "-z", "--", "*.cpp", "*.cc", "*.cxx", "*.h", "*.hpp", "*.mm"],
        cwd=ROOT,
    )
    return [
        ROOT / name.decode("utf-8")
        for name in result.split(b"\0") if name
    ]


class CommunityCppApiContracts(unittest.TestCase):
    def test_all_qualified_client_references_exist_in_class_declaration(self):
        header = CLIENT_HEADER.read_text(encoding="utf-8")
        start = header.find("class SnowShotApiClient final : public QObject {")
        self.assertGreaterEqual(start, 0)
        declaration = header[start:]
        names = set(re.findall(r"\b[A-Za-z_]\w*\b", declaration))
        paths = tracked_cpp_files()
        self.assertGreater(len(paths), 100, "C++ source scan did not cover the repository")
        missing = []
        for path in paths:
            content = path.read_text(encoding="utf-8")
            for match in MEMBER_REFERENCE.finditer(content):
                if match.group(1) not in names:
                    line = content.count("\n", 0, match.start()) + 1
                    missing.append(f"{path.relative_to(ROOT)}:{line} ({match.group(1)})")
        self.assertEqual(missing, [], "Unresolved SnowShotApiClient member: " + "; ".join(missing))

    def test_removed_account_signal_is_not_connected_anywhere(self):
        offenders = [
            str(path.relative_to(ROOT))
            for path in tracked_cpp_files()
            if "accountIdentityChanged" in path.read_text(encoding="utf-8")
        ]
        self.assertEqual(offenders, [], "Deleted account signal still referenced")
        self.assertIn("&SnowShotApiClient::chatModelsChanged", TRANSLATION.read_text(encoding="utf-8"))


if __name__ == "__main__":
    unittest.main(verbosity=2)
