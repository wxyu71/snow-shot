# Snow Shot Community (Windows x64)

This fork keeps the upstream screenshot/element selection, global shortcuts,
pinning and local PP-OCRv6 implementation. The **Convert to Markdown** screenshot
action is the single cloud intelligent OCR entry point.

## Cloud PaddleOCR — configure once in application settings

Open **Settings > Text recognition & translation > Cloud OCR for Markdown**.
Enter your **PaddleOCR AI Studio API URL**, **Access Token** and a model:
**PaddleOCR-VL-1.6**, **PP-StructureV3**, or **PP-OCRv6**.
The default host is `https://paddleocr.aistudio-app.com`; requests go
directly to `/api/v2/ocr/jobs` without the upstream Snow Shot account.

Click **Test connection** to submit a small synthetic OCR image through the
*actual authenticated asynchronous API*. The test may consume provider quota.
Click **Save** to keep the selected endpoint/model/credentials. On Windows the
Access Token is protected with user-scoped DPAPI.

After capturing a selection, click the **single Convert to Markdown** action.
There is no five-action popup or screenshot-local credential editor. The job
runs in the background; success copies the complete Markdown to the clipboard
and shows a short system-tray notification. Completed Markdown is stored with the
existing screenshot recognition history; reopening the same recognition key with
the same provider/model/endpoint identity restores that result immediately instead
of uploading the image again. Markdown edits update the stored conversion result,
and partial copy remains available in the viewer. **Ctrl+D, local text recognition, and the QR
scanner in the selection toolbar stay independent and offline.** For now the
supported cloud protocol is Baidu AI Studio's OCR jobs API; arbitrary third
party APIs require individual protocol adapters, not just an arbitrary URL.

Security: network endpoints require HTTPS except loopback for local testing.
The image is transmitted to the configured provider; access tokens must not
be included in screenshots or bug reports. Cloud result download URLs do not
receive the API token.

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
uploads both installers as artifacts **on successful completion**. A prerelease is
started manually from **Actions > Community Windows x64 installer and beta release**
on the current `main` branch with `publish_prerelease=true`. The workflow first
performs the clean Windows build, native packaging checks, three-model C++ Qt contract
tests and SHA-256/manifest verification. Only after those pass does it create the
version tag (for example, `v1.5.8-beta.1_snow-shot`), assemble a draft GitHub
Release, verify that all six expected assets are attached, and publish it as a
prerelease. PR runs and manual runs with `publish_prerelease=false` only upload
private workflow artifacts. Releases contain only
the x64 Full online/offline NSIS installers, checksums and manifests, never
Mini, ARM64, macOS or portable builds.

### Simple GitHub Releases updates

Community Windows releases use one public download page:
https://github.com/wxyu71/snow-shot/releases

The **About > Check updates on GitHub** button opens this page in the
default web browser; it does **not** download, unpack, or execute software.
Choose the latest online/offline NSIS installer and install it manually.
Each beta version creates its own versioned GitHub prerelease (for example,
`v1.5.8-beta.1_snow-shot`). The tag is created **after** the clean native build
and installer verification, so failed builds do not leave release tags behind.
Publication uses a draft-first flow: create/reuse the draft, upload the six allowed
assets, verify the exact asset-name set, then switch the draft to a prerelease.
If publication fails after the tag or draft was created, rerun the same failed
workflow so it resumes against the same commit; a new manual release run from a
different main commit refuses to move the existing version tag.

The Windows Community build no longer starts the legacy automatic updater
on launch, nor offers automatic download/install controls. GitHub HTTPS
protects transport, but alone does not establish source provenance or
protect against a compromised GitHub account; review the repository and
optionally compare the published SHA-256 checksums. The original updater's
legacy code may remain linked for installer compatibility, but it is not
used to check or apply Community updates. No dedicated signing keys,
`latest-version.json`, or `*-update.zip` release assets are required.
The embedded legacy updater is not enabled by this workflow.

This branch remains `1.5.8-beta.1` while the PR is under review; a future
version bump is a separate release decision.

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

## Windows CI Rust compatibility and Qt caching

Windows CI run #25 passed static Qt 6.12.0 but failed when the pinned Rust
nightly treated deprecated `AtomicU32::fetch_update` in the screenshot
capture pause/revision state as an error (`-D warnings`). The implementation
uses `AtomicU32::try_update` with identical AcqRel/Acquire orderings and the
same conditional revision transition. `scripts/test-community-rust-compat.py`
checks the source contract, and changes to `snow-crates/**` now trigger the
Windows installer workflow. The release keeps warnings-as-errors enabled.

The validated Qt kit is saved immediately after the Qt build, rather than only
at successful job completion; a subsequent unrelated application build failure
no longer wastes the already successful Qt compilation. Cache upload failures
cannot bypass native release tests or turn a failed build green.

## Windows CI #26 follow-up

Windows CI run #26 got beyond the screenshot capture deprecation fix from
#25, but the same pinned Rust nightly rejected two further deprecated atomic
calls at `snow-crates/crates/snow-audio-recorder/src/control.rs:149,248`.
They now use `AtomicU64::try_update`, retaining the same conditional
compare-and-swap updates and `Ordering::AcqRel`/`Ordering::Acquire` memory
orderings. `scripts/test-community-rust-compat.py` scans **all git-tracked
Rust sources** before the expensive Qt and Windows builds for new uses of
the deprecated method. This preflight does not replace actual native CI.

## Rust atomic compatibility preflight coverage

The new repository-wide test caught five more pre-existing deprecated calls
before Windows could reach them: `snow-recording-export/src/gpu.rs`,
`snow-ui-selector/src/windows/msaa.rs`, `snow-draw-engine-gpui/src/gpu_wake.rs`,
`snow-draw-engine-gpui/src/smart_erase.rs`, and
`snow-image-rs/src/animation.rs`. They now use `try_update` with their
original memory orderings, closures and error handling. Windows PR builds
are triggered by changes in all tracked Rust workspace source trees.

## Failure triage and fast-to-slow CI

See [the CI triage ledger](community-ci-triage.md) for the first real errors
behind Windows runs #21, #25, #26, #27 and #28. The latest independent
failure in #28 is a C++ translation listener for the removed account identity
signal. The listener has been removed and a full-tree C++ API source contract
now runs in the Linux fast preflight.

The Windows workflow is now the single Community PR gate for fast policy checks,
the real C++ Qt PaddleOCR contract, and the conditional native Windows build.
Every PR enters the workflow; documentation-only changes stop after the fast
classification/preflight, while any non-documentation change reaches the native
gate. This avoids brittle build-input path allowlists and removes the duplicate
standalone safety/PaddleOCR workflows. The Windows build waits for **both** the
fast Python preflight and the real C++ Qt PaddleOCR contract before provisioning
vcpkg and compiling. It reuses pinned vcpkg binary archives and Cargo sources,
retains the audited static Qt cache, and avoids a duplicate
`build.ps1`-invoked bootstrap. Native compilation and installer verification cannot be skipped by a cache.
Prerelease publication is an explicit manual mode and never consumes the PR-only native build cache.

## Early NSIS packaging-toolchain check

Windows run #29 completed the native x64 Full application and staged verified
local OCR assets, but the installer step failed because the hosted runner did
not include `makensis`. The pinned `scripts/bootstrap-community-nsis.ps1`
now installs NSIS 3.12.0 with Chocolatey, verifies the installed compiler and
runs an actual minimal CPack NSIS packaging smoke test **before** the costly
vcpkg/Rust/C++ initialization. `scripts/test-community-nsis-toolchain.py`
checks this ordering in the Linux fast preflight. See
[CI triage](community-ci-triage.md) for evidence. Native end-to-end
installation, offline payload and release checksum checks remain mandatory.

## FullOnly Mini policy and native compilation checkpoints

Windows run #30 again linked the complete Full executable, but the
packaging script (rather than CMake) incorrectly demanded
`SNOW_APPS_BUILD_SNOW_SHOT_MINI:BOOL=ON`. Community x64 Full must
always have Mini=OFF. The packaging code now uses a shared
`$miniEnabled` decision for both CMake cache enforcement and optional
native build targets; `-FullOnly` suppresses Mini, while legacy
Full+Mini standalone packaging remains available outside the community
release. Python and actual early Windows PowerShell tests validate all
four Store/FullOnly combinations before vcpkg/Rust initialization.

Build outputs from a previous failed hosted run are not retained unless
explicitly saved; #30 had no artifacts. For future native-success /
packaging-failure cases the PR workflow now saves a source-keyed,
PR-only cache of the CMake/MSVC/Rust build directory immediately after
successful native build and the Mini=OFF guard; packaging-only edits
do not invalidate that key. A diagnostic x64 Full executable artifact
is also uploaded before attempting NSIS packaging. The native CMake
build still runs, and tagged releases never consume the PR build cache.

## Cargo release license audit preflight and native reuse

Windows #31 compiled the full x64 binary and saved the native build cache,
but packaging failed because the mandatory offline Cargo metadata audit
for `snow-ocr-process` needed `anes v0.1.6` that compilation and the
old Cargo source cache had not downloaded. To avoid repeated hour-long
compile-then-audit failures, the Windows job now uses a **shared,
audited Cargo license manifest/feature plan**, fetches complete locked
workspace dependencies for all targets, checks offline metadata and
dependency trees, and executes the full third-party license collector
after Qt provisioning **before** the native build. Production packaging
runs the same collector again without relaxing `--offline`, license
notice provenance or the release-integrity gates. A separate v2 Cargo
source cache stores this resolved graph, restoring from the old cache
when available. Script-only changes preserve the PR-only compiled
native cache from #31; native CMake validation still runs.

### Pinned nightly toolchain and incomplete Cargo caches

The first Windows execution of the early license audit revealed that
`Get-SnowOcrToolchain` used `-cnotmatch` followed by `$Matches[1]`,
which can return an empty string when invoked in a fresh scope.
An explicit regex Match/Group now produces the pinned nightly reliably;
an early real PowerShell regression test validates both fresh and stale
capture scenarios. The Cargo license preflight constructs explicit
typed CLI argument arrays and validates the nightly name.
Any v2 Cargo cache written after an incomplete preflight remains only a
restore fallback; successful complete license preflight is required
before writing the new v3 cache. The PR-only native compile cache key
remains unchanged.
