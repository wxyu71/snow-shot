#!/usr/bin/env python3
"""Community Windows OCR/settings/toolbar/release source integration contracts."""
import json
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[1]


def source(path):
    return (ROOT / path).read_text(encoding="utf-8")


class CommunityOcrUiContracts(unittest.TestCase):
    def test_persistent_settings_and_live_probe(self):
        catalog = source("snow_shot/src/presentation/settings/settingscatalog.cpp")
        settings = source("snow_shot/src/presentation/components/cloudocrsettingswidget.cpp")
        routing = source("snow_shot/src/presentation/components/settingscustomwidget.cpp")
        controller = source("snow_shot/src/presentation/ocr/screenshotimageconversioncontroller.cpp")
        self.assertIn('QStringLiteral("text-recognition.cloud-ocr")', catalog)
        self.assertIn("SettingsCustomRenderer::CloudOcr", routing)
        self.assertIn("new CloudOcrSettingsWidget(parent)", routing)
        for key in ("paddleOcrApiUrl", "paddleOcrAccessToken", "paddleOcrModel",
                    "paddleOcrTestConnection"):
            self.assertIn(key, settings)
        self.assertIn("probe->recognize(sample", settings)
        self.assertIn("readCommunityCloudOcrSettings()", controller)
        self.assertIn("cloudRequestIdentity", controller)
        self.assertIn("restoreCachedResult(m_requestModel, m_requestFingerprint)", controller)
        self.assertIn("updateCurrentCache()", controller)
        self.assertNotIn("new QDialog(owner)", controller)
        self.assertIn("PaddleOCR-VL-1.6", settings)
        self.assertIn("PP-StructureV3", settings)
        self.assertIn("PP-OCRv6", settings)

    def test_windows_access_token_is_protected(self):
        secure = source("snow_shot/src/network/communitycloudocrsettings.cpp")
        self.assertIn("CryptProtectData", secure)
        self.assertIn("CryptUnprotectData", secure)
        self.assertIn("access_token_dpapi_v1", secure)
        self.assertIn('settings.remove(QStringLiteral("access_token"))', secure)
        self.assertIn("readCommunityCloudOcrSettings", secure)

    def test_one_markdown_action_and_separate_offline_recognition(self):
        edition = source("snow_shot/include/snow_shot/app/editionfeatures.h")
        layout = source("snow_shot/include/snow_shot/presentation/screenshottoolbarlayoutmodel.h")
        palette = source("snow_shot/src/presentation/tools/screenshottoolpalette.cpp")
        self.assertIn('id == QStringLiteral("barcode-recognition")', edition)
        self.assertIn('id == QStringLiteral("convert-to-html")', edition)
        self.assertIn('{QStringLiteral("convert-to-markdown")}', layout)
        self.assertIn('screenshotConvertToMarkdownButton', palette)
        self.assertIn('screenshotTableQrButton', palette)
        self.assertNotIn('screenshotConvertToHtmlButton', palette)
        self.assertNotIn('screenshotLatexRecognitionButton', palette)
        self.assertIn('emit ocrRequested()', palette)
        self.assertIn('emit qrRequested()', palette)
        self.assertIn("Cloud OCR credentials/model are configured in Settings", palette)

    def test_clipboard_notification_uses_existing_tray(self):
        conversion = source("snow_shot/src/presentation/ocr/screenshotimageconversioncontroller.cpp")
        tray = source("snow_shot/src/presentation/services/systemtraycontroller.cpp")
        app = source("snow_shot/src/app/applicationcontroller.cpp")
        self.assertLess(conversion.index("setText(m_source)"),
                        conversion.index("emit markdownCopied()"))
        self.assertIn("showMarkdownCopiedMessage", app)
        self.assertIn("QSystemTrayIcon::Information, BalloonKind::Capture", tray)
        self.assertIn("The OCR result has been copied to the clipboard.", tray)

    def test_about_credits_and_public_links(self):
        about = source("snow_shot/src/presentation/components/aboutpagewidget.cpp")
        self.assertIn('© 2025–2026 mg-chao · © 2026 WXYU71', about)
        self.assertIn("m_ui->community->hide()", about)
        self.assertIn("m_ui->license->hide()", about)
        self.assertNotIn("AboutResourceButton", about)
        self.assertNotIn("aboutCommunityReleases", about)
        self.assertIn("m_ui->userAgreement->setText", about)
        self.assertIn("m_ui->privacyPolicy->setText", about)
        presets = source("CMakePresets.json")
        self.assertNotIn("snowshot.top", presets)

    def test_simple_github_releases_without_auto_updater(self):
        workflow = source(".github/workflows/community-windows-x64.yml")
        about = source("snow_shot/src/presentation/components/aboutpagewidget.cpp")
        app = source("snow_shot/src/app/applicationcontroller.cpp")
        settings = source("snow_shot/src/presentation/settings/settingscatalog.cpp")
        cmake = source("CMakeLists.txt")
        self.assertIn('set(SNOW_SHOT_VERSION "1.5.8-beta.1")', cmake)
        self.assertIn('gh release create "$GITHUB_REF_NAME"', workflow)
        self.assertIn('gh release upload "$GITHUB_REF_NAME"', workflow)
        self.assertIn("--verify-tag", workflow)
        self.assertNotIn("gh release delete-asset community-beta", workflow)
        self.assertIn("if: github.event_name == 'push'", workflow)
        self.assertNotIn("COMMUNITY_UPDATE_SIGNING_PRIVATE_KEY", workflow)
        self.assertNotIn("latest-version.json", workflow)
        self.assertNotIn("-update.zip", workflow)
        self.assertIn('gh release create "$GITHUB_REF_NAME"', workflow)
        self.assertFalse((ROOT / "scripts/sign-community-beta-update.ps1").exists())
        self.assertIn("https://github.com/wxyu71/snow-shot/releases", about)
        self.assertIn("Check updates on GitHub", about)
        self.assertIn("Download and install manually.", about)
        self.assertIn("if (distribution::isStoreManaged())", app)
        self.assertIn("#ifndef Q_OS_WIN\n    {updateModeItem},", settings)
        self.assertNotIn("if: startsWith(github.ref, 'refs/tags/')", workflow)


if __name__ == "__main__":
    unittest.main(verbosity=2)

