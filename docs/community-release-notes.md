# Snow Shot Community — Windows x64 Beta

**Independent, unofficial fork** of [mg-chao/snow-shot](https://github.com/mg-chao/snow-shot).
Release version: **1.5.8-beta.1**, following official **1.5.7**.

This prerelease contains **only the full Snow Shot Windows Intel/AMD x64 installer**.
Choose the offline installer for bundled local PP-OCRv6 OCR resources, or the online installer for the smaller download. No Snow Shot Mini, macOS or ARM64 release is supplied.

- No Snow Shot sign-in, registration, subscription, credits or payment UI.
- Intelligent window/element capture, hotkeys, pinning and fast local OCR are retained.
- Direct PaddleOCR AI Studio OCR with user-provided HTTPS API URL and Access Token: PP-OCRv6, PP-StructureV3 and PaddleOCR-VL-1.6. Markdown results are copied automatically and can be previewed, edited and selectively copied.
- Windows NSIS executable name, install location and uninstall identity remain compatible with the official installer, although **an actual installed-over-1.5.7 upgrade must still be manually validated**.
- SHA-256 checksums and build/native packaging verification manifests are attached.

**Security and limitations:** Cloud screenshots are sent to the user's configured PaddleOCR API. On Windows the Access Token is saved using user-scoped DPAPI protection (older beta settings are readable until the next save). Updates are **manual**: the in-app button opens this fork's public GitHub Releases page; no signed update feed, automatic download, or background install is required. GitHub HTTPS does not replace independent artifact signing; optional SHA-256 hashes are provided for manual comparison. Live PaddleOCR API behavior must be tested with real credentials.

Source code, original author copyright and licensing notices are preserved. Please report verified issues to the [community fork](https://github.com/wxyu71/snow-shot/issues).
