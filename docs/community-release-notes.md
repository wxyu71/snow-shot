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

**Security and limitations:** Cloud screenshots are sent to the user's configured PaddleOCR API. The token is stored unencrypted in user-local Qt settings. A community signing/update trust chain has not been provisioned, so the **in-app signed auto-updater is not validated**. No live PaddleOCR credentials were provided; API mock tests pass but the three live models require testing by the user.

Source code, original author copyright and licensing notices are preserved. Please report verified issues to the [community fork](https://github.com/wxyu71/snow-shot/issues).
