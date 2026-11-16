# Snow Shot Community (Windows x64 beta)

> **Independent open-source fork** of [Snow Shot by mg-chao](https://github.com/mg-chao/snow-shot).
> This project is **not an official release** and is not operated or supported
> by the original author. Copyright and GPL/third-party notices remain intact.

[简体中文](README.zh-CN.md) · [Changes](docs/community-changelog.md) ·
[Windows build and verification](docs/community-windows.md) ·
[Issues](https://github.com/wxyu71/snow-shot/issues)

## Status — v1.5.8-beta.1

**Testing branch: not production-ready.** A release can only be published
after the Windows x64 full installer, offline OCR assets, in-place upgrade
from the original 1.5.7 and native regression tests pass. Do **not** download
an upstream original-author release expecting these community changes.

## Features

- Intelligent window/element selection, capture, annotations, hotkeys and
  pin-to-screen; retain the existing screenshot engine.
- **Fast local PP-OCRv6**: no network required; automatically copies the
  complete recognized text and keeps the selectable overlay.
- **Cloud intelligent OCR**: direct, optional [Baidu AI Studio PaddleOCR
  API](https://ai.baidu.com/ai-doc/AISTUDIO/fml7mozw5) with your own
  API URL and Access Token. Choose PP-OCRv6, PP-StructureV3 or
  PaddleOCR-VL-1.6. Download structured Markdown, automatically copy it,
  preview, edit and copy any selection.
- No Snow Shot login, registration, membership or paid credits.
- Protected scheduled upstream synchronization; changes requiring full
  Windows verification are sent for human review.

Screenshots submitted to cloud OCR go **directly to the configured API
endpoint**, not to an account service. The API token is currently stored
unencrypted in the user's local Qt settings file. See
[privacy](docs/community-privacy.md).

## Build and test

For a full Windows x64 build, use VS 2026, CMake 4.2+, the repository's vcpkg
toolchain, audited static Qt 6.12.0 and the OCR release runtime:

```powershell
./scripts/bootstrap.ps1 -Preset snow-shot-msvc-release
./scripts/build.ps1 -Preset snow-shot-msvc-release
ctest --test-dir build/snow-shot-msvc-release -C Release --output-on-failure
./scripts/package-snow-shot.ps1 -Architecture x64 -SkipBuild
```

See [Windows beta release notes](docs/community-windows.md) for the Qt kit
bootstrap, build verification and installer-upgrade limitations. To test the
real AI Studio API with your own token, use
`python scripts/paddleocr_live_e2e.py --live` (with
`PADDLEOCR_ACCESS_TOKEN` set outside source control). Offline contract:
`python scripts/paddleocr_live_e2e.py --selftest`.

## Licensing and attribution

This project inherits code from the original Snow Shot repository and is
distributed under the applicable GPL-3.0-or-later license and dependency
licenses. See [LICENSE](snow_shot/LICENSE),
[COPYRIGHT](snow_shot/COPYRIGHT), and bundled third-party notices. The
original project's authors retain attribution; this page and the product UI
are not a claim of affiliation.
