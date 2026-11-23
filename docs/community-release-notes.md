# Snow Shot Community — Windows x64 Beta

**Independent, unofficial fork** of [mg-chao/snow-shot](https://github.com/mg-chao/snow-shot).
Release version: **@VERSION@**. This independently versioned Community beta retains upgrade compatibility with the official **1.5.7** installer and incorporates source changes through official upstream commit **7f34f246** (official 1.6.1 line), subject to beta acceptance testing.

This testing prerelease contains **one Community Windows Intel/AMD x64 product**.
Choose the offline installer for bundled local PP-OCRv6 OCR resources, or the online installer for the smaller download. No Snow Shot Mini, macOS or ARM64 release is supplied.

- No Snow Shot sign-in, registration, subscription, credits or payment UI.
- Intelligent window/element capture, hotkeys, pinning and fast local OCR are retained.
- One user-triggered, PaddleOCR-only Online OCR button: configure AI Studio credentials and choose PP-OCRv6, PP-StructureV3 or PaddleOCR-VL-1.6. The result is Markdown with formulas preserved as LaTeX; obsolete Vision provider settings are ignored.
- Local screenshot and pinned-image histories default enabled and are never uploaded automatically. Existing opt-outs and retention settings remain under user control.
- Windows NSIS executable name, install location and uninstall identity remain compatible with the official installer, although **an actual installed-over-1.5.7 upgrade must still be manually validated**.
- SHA-256 checksums and build/native packaging verification manifests are attached.

**Security and limitations:** Only explicitly requested Online OCR sends a screenshot directly to the user's configured PaddleOCR endpoint, never to a Snow Shot account service. The fork contains no analytics/telemetry upload path. On Windows the PaddleOCR Access Token is saved using user-scoped DPAPI protection. Updates are **manual**: the in-app button opens this fork's public GitHub Releases page. No automatic signed-update feed or background installation is required. A personal-use Windows installer does not need a purchased code-signing certificate, although SmartScreen may warn about an unsigned build. SHA-256 sidecars support manual integrity checks. Live provider behavior and the real installed upgrade must be tested on a Windows PC.

Source code, original author copyright and licensing notices are preserved. Please report verified issues to the [community fork](https://github.com/wxyu71/snow-shot/issues).
