#!/usr/bin/env python3
"""Guard against accidental reintroduction of original-author commercial redirects.

Legal attribution is intentionally excluded: the fork must preserve copyright
and GPL/third-party notices even when its UI and support targets change.
"""
from pathlib import Path
import re
import unittest

ROOT = Path(__file__).resolve().parents[1]
FILES = (
    "README.md",
    "README.zh-CN.md",
    "CMakeLists.txt",
    "snow_shot/CMakeLists.txt",
    "snow_shot/src/presentation/components/aboutpagewidget.cpp",
    "snow_shot/include/snow_shot/app/distribution.h",
    "snow_shot/packaging/README.txt",
)

class CommunityBrandingTests(unittest.TestCase):
    def test_no_author_website_or_qq_support_links(self):
        for name in FILES:
            content = (ROOT / name).read_text(encoding="utf-8")
            with self.subTest(name=name):
                self.assertNotIn("https://snowshot.top", content)
                self.assertNotIn("https://test.snowshot.top", content)
                self.assertNotIn("qm.qq.com", content)
                self.assertNotIn("SNOW_SHOT_QQ_GROUP_4_URL", content)
                self.assertNotIn("chao@mgchao.top", content)

    def test_no_account_payment_ux_or_legacy_service(self):
        cmake = (ROOT / "snow_shot/CMakeLists.txt").read_text(encoding="utf-8")
        about = (ROOT / "snow_shot/src/presentation/components/aboutpagewidget.cpp").read_text(encoding="utf-8")
        self.assertNotIn("AccountSessionService", cmake)
        self.assertNotIn("AccountPageWidget", cmake)
        self.assertNotIn("accountsecretstore", cmake.lower())
        self.assertNotIn("QQ group 4", about)
        self.assertRegex(cmake, r'set\(SNOW_SHOT_API_BASE_URL "" CACHE STRING')

    def test_fork_identity_and_nonofficial_beta_notice(self):
        root = (ROOT / "CMakeLists.txt").read_text(encoding="utf-8")
        self.assertIn('set(SNOW_SHOT_VERSION "1.5.8-beta.1")', root)
        self.assertIn('set(SNOW_SHOT_VENDOR "Snow Shot Community")', root)
        for name in ("README.md", "README.zh-CN.md"):
            content = (ROOT / name).read_text(encoding="utf-8")
            self.assertIn("wxyu71/snow-shot", content)
            self.assertIn("mg-chao/snow-shot", content)  # required upstream attribution
        self.assertTrue((ROOT / "snow_shot/COPYRIGHT").exists())
        self.assertTrue((ROOT / "snow_shot/LICENSE").exists())

if __name__ == "__main__":
    unittest.main(verbosity=2)
