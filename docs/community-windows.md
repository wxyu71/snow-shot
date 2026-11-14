# Snow Shot Community (Windows x64)

This fork keeps the upstream screenshot/element selection, global shortcuts,
pinning and local PP-OCRv6 implementation. The **Convert to Markdown** screenshot
action is the single cloud intelligent OCR entry point.

## Cloud PaddleOCR — bring your own Access Token

1. Open a screenshot and choose **Convert to Markdown**.
2. Open the conversion settings and supply your **Baidu AI Studio API URL** and
   **Access Token**. The default host is
   `https://paddleocr.aistudio-app.com`. The client calls AI Studio's
   `/api/v2/ocr/jobs` directly, without a Snow Shot account, local OCR model
   for cloud requests, relay, proxy, or bridge.
3. Select **PaddleOCR-VL-1.6** (mixed layout), **PP-StructureV3** (structured
   tables and formulas), or **PP-OCRv6** (text). All three write their complete
   response to the existing Markdown view.
4. Completion automatically copies **complete Markdown** to the clipboard.
   You may edit the Markdown preview and copy a selection in the existing UI.

The separate local text-recognition action uses the existing offline
PP-OCRv6 modules. On successful recognition it copies all OCR text while
keeping the selectable text overlay open. Cloud jobs send the selected image
to Baidu AI Studio. Local OCR never needs the API token.

Security notes: only HTTPS hosts are accepted (localhost HTTP is permitted
for tests); API token headers are sent to the configured job endpoints, never
to the returned signed Markdown URL. The API token is currently saved in
the user-local `SnowShot/CloudOcr.ini` via Qt QSettings **without encryption**.
Use a scoped token and protect your Windows user profile. Credentials and
screenshots must never be committed or included in bug reports.

## Independent Windows x64 release and compatibility

This branch increments upstream 1.5.7 to **1.5.8-beta.1**. The full NSIS installer
continues to use upstream's `snow_shot.exe`, `SnowShot` installer registry
key, `SnowShot` installation directory, and uninstall-before-install logic.
These values are required for an in-place upgrade from the official installer.

Build on a provisioned Windows x64 VS 2026 environment with CMake 4.2+,
static Qt, NSIS, the repository's vcpkg toolchain and OCR runtime assets:

```powershell
./scripts/build.ps1 -Preset snow-shot-msvc-release
ctest --test-dir build/snow-shot-msvc-release -C Release --output-on-failure
./scripts/package-snow-shot.ps1 -Architecture x64 -SkipBuild
```

The package script creates online and offline full installers; the **offline**
variant includes the packaged OCR runtime and its default local model.
`.github/workflows/community-windows-x64.yml` validates the build and
uploads both installers as artifacts **on successful completion**. Only a beta tag pushed from the reviewed `main` history (for example,
`v1.5.8-beta.1_snow-shot`) triggers automatic publication to this fork's
GitHub Releases **after** the Windows build, native packaging checks, the three-model
C++ Qt contract tests and SHA-256/manifest verification pass. PR and manual CI runs only upload private
workflow artifacts; they do not publish an installer. Releases contain only
the x64 Full online/offline NSIS installers, checksums and manifests, never
Mini, ARM64, macOS or portable builds.

The embedded update-source defaults and Rust origin allowlist now point to
`wxyu71/snow-shot` (including the fallback release host), preventing the
community fork from silently downloading an upstream official binary that
could restore removed features. **Signed community update publishing is not
configured:** an independently trusted signing key and verified fork release
metadata must be set up before automatic binary updates can function. The
NSIS upgrade-over-official-installer path is separate from this future update
channel.

**Verification required before public release:** actual Windows build and
unit tests; NSIS upgrade over an installed official 1.5.7; preservation of
user settings and shortcuts; local OCR model bootstrapping and all three
AI Studio model responses. A version-number change alone does not prove
these upgrade and runtime behaviors.

## Commercial-code removal and upstream guard

The account session/registration, member UI, persistent account-secret store,
and account-specific tests were removed. Generic translation service code is
still present for existing optional features; full audit of every historic
paid-service reference, updater/channel behavior and bundled translations is
required before treating the fork as a completely de-commercialized release.
See [sync policy](community-upstream-sync.md).

## Reproducible CI toolchain provisioning

The Windows x64 release preset explicitly disables Snow Shot Mini and the
upstream Snow Shot account API base URL. `package-snow-shot.ps1 -FullOnly`
skips Mini packaging; legacy Homebrew, WinGet and Scoop publication workflows
are not retained in the community fork.

The hosted Windows runner does not preinstall the repository's audited
Qt 6.12.0 static Release kit. The beta pipeline calls
`bootstrap.ps1 -SkipQtValidation -VcpkgVariants Static` to install pinned
vcpkg/Rust prerequisites, then `build-static-qt.ps1` to build and validate
Qt in `.tools/qt/6.12.0/x64-static-release` before the normal build and
installer steps. A keyed Actions cache can reuse the verified Qt kit.
Qt compilation is slow on fresh runners, and CI failure remains a release blocker.

The native Rust OCR license dependency audit is an independent CI job. It
installs nightly-2026-09-08, fetches the exact locked dependency trees for
Windows x64 and macOS x64/arm64, then runs the original offline audit.
Neither audit should be marked green merely because build dependencies were
missing.

The NSIS CompanyName metadata identifies **Snow Shot Community** rather
than the upstream vendor; the executable, registry key and installation
directory stay identical for upgrade compatibility. Original license and
copyright attributions remain in the bundled notices.
