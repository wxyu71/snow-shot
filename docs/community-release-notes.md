# Snow Shot Community — Windows x64 Beta

**Independent, unofficial fork** of [mg-chao/snow-shot](https://github.com/mg-chao/snow-shot).
Release version: **@VERSION@**. This independently versioned Community beta retains upgrade compatibility with the official **1.5.7** installer and incorporates source changes through official upstream commit **7f34f246** (official 1.6.1 line), subject to beta acceptance testing.

This prerelease contains **only the full Snow Shot Windows Intel/AMD x64 installer**.
Choose the offline installer for bundled local PP-OCRv6 OCR resources, or the online installer for the smaller download. No Snow Shot Mini, macOS or ARM64 release is supplied.

- No Snow Shot sign-in, registration, subscription, credits or payment UI.
- Intelligent window/element capture, hotkeys, pinning and fast local OCR are retained.
- One Online OCR button with provider selection in Settings: direct PaddleOCR AI Studio (PP-OCRv6, PP-StructureV3 or PaddleOCR-VL-1.6) returns Markdown with formulas preserved as LaTeX; the inherited large-model OCR pipeline can use a user-configured OpenAI-compatible vision model and output Markdown, HTML or LaTeX.
- Windows NSIS executable name, install location and uninstall identity remain compatible with the official installer, although **an actual installed-over-1.5.7 upgrade must still be manually validated**.
- SHA-256 checksums and build/native packaging verification manifests are attached.

**Security and limitations:** Online OCR screenshots are sent directly to the provider selected by the user (PaddleOCR or a configured OpenAI-compatible vision endpoint), never to a Snow Shot account service. The fork contains no analytics/telemetry upload path. On Windows the PaddleOCR Access Token is saved using user-scoped DPAPI protection. Updates are **manual**: the in-app button opens this fork's public GitHub Releases page; no signed update feed, automatic download, or background install is required. GitHub HTTPS does not replace independent artifact signing; optional SHA-256 hashes are provided for manual comparison. Live PaddleOCR API behavior must be tested with real credentials.

Source code, original author copyright and licensing notices are preserved. Please report verified issues to the [community fork](https://github.com/wxyu71/snow-shot/issues).
