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

## CI and prerelease pipeline

The Community fork follows the repository's native test design instead of maintaining a second
layer of source-text contract tests. New behavior is tested in the existing C++/Rust test targets
and executed through CMake/CTest. Compiler, CMake, Cargo and NSIS failures are validated by the
real tools that produce the application.

Every pull request enters one workflow:

1. A small Ubuntu preflight runs the upstream-sync policy tests, deletion guard, the existing
   Qt/CMake contract, PaddleOCR fixture self-test, and release helper self-test. Documentation-only
   changes stop here.
2. Native changes use the pinned Windows toolchain and the cached vcpkg/Cargo/static-Qt dependency
   layers.
3. A dedicated `snow-shot-msvc-test` build enables the repository's real tests without enabling
   packaging or release optimization. Its aggregate `snow-shot-community-tests` target builds only
   the focused native test executables that CTest will run. PRs do not link the production
   `snow_shot.exe`; the focused build has its own two-hour timeout so a stalled compiler or linker
   cannot consume the whole Windows job.
4. A normal `workflow_dispatch` additionally smoke-tests the pinned NSIS/CPack toolchain, runs
   the production Cargo/license collector, creates the `snow-shot-msvc-release` production build
   tree, links the full application once, packages the Full-only NSIS online/offline installers,
   verifies their manifests and SHA-256 files, and uploads them as private workflow artifacts.
5. With `publish_prerelease=true`, the same manually verified production artifacts continue into
   the draft/prerelease publication gate.

The pipeline caches dependencies and the audited static Qt kit, not CMake/MSVC application build
trees. This avoids stale compiled state while retaining the expensive dependency work.

The separate source-grep `test-community-*` scripts, standalone PaddleOCR CMake harness, and
duplicate OCR license workflow are intentionally removed. The production Cargo license preflight
already resolves the locked Windows release graph and runs the same collector used by packaging.

### Prerelease publication

Start **Community Windows x64 installer and beta release** manually from current `main` with
`publish_prerelease=true`. The workflow runs the same focused Windows tests and a clean production
build first. Only after the six local release assets pass verification does it create the version
tag, create or reuse a draft release, upload the assets, download them again and run the same asset
verification on the GitHub-hosted copies. The draft is then published as a prerelease.

A failed build creates no release tag. If publication fails after a draft was created, rerun the
same workflow while the release is still a draft; an already-published release is never silently
overwritten.

### Upstream synchronization

The scheduled upstream classifier still auto-merges only documentation changes that do not touch
fork-protected paths. Source changes use one reusable `automation/upstream-review` branch. When
the upstream merge is conflict-free, that branch contains the real merge candidate so the normal
pull-request CI tests the code that would actually enter `main`; merge conflicts produce a small
report on the same branch instead of opening a new notification PR every day.
