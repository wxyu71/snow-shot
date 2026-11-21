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
./scripts/package-snow-shot.ps1 -Architecture x64 -SkipBuild
```

The package script creates online and offline full installers; the **offline**
variant includes the packaged OCR runtime and its default local model.

The CI pipeline deliberately separates normal review from release acceptance:
ordinary PR updates (including Draft Beta PR #2) run only the Ubuntu
repository-policy and helper **preflight**; they do **not** start a Windows
runner, build production installers, or upload release candidates. Freeze a
reviewed Beta candidate, then use **Actions > Community Windows x64 > Run
workflow** on the exact Beta branch with `publish_prerelease=false`. That
single explicit `workflow_dispatch` runs focused native Windows tests, builds
the production application, packages real online/offline NSIS installers,
verifies their checksums/manifests, exercises online-to-offline installation
and uninstall, and uploads six verified CI artifacts. The artifacts are
test outputs, not private or approved release binaries.

Only after the manual official **1.5.7** upgrade acceptance and merge into
the reviewed current `main` branch may a maintainer start the same workflow
from `main` with `publish_prerelease=true`. A fresh verified production build
must succeed before the workflow creates the version tag (for example,
`v1.5.8-beta.1_snow-shot`), stages a draft release, re-verifies all six
GitHub-hosted assets, and publishes a prerelease. Neither PR events nor
`publish_prerelease=false` runs create a tag or GitHub Release. Releases
contain only the x64 Full online/offline NSIS installers, checksums and
manifests, never Mini, ARM64, macOS or portable builds.

For reproducible Windows CI, the workflow pins MSVC to the exact installed
`14.52.37025` toolset and checks it **before** installing dependencies. If
that toolset disappears from the hosted runner, the job stops immediately
instead of silently switching compilers. Local builds may still select the
newest supported toolset unless `SNOW_MSVC_TOOLSET_VERSION` is set explicitly.
The vcpkg **binary** cache is keyed by the selected MSVC and Windows SDK
versions as well as dependency inputs. A separate `VCPKG_DOWNLOADS` cache
preserves source archives and downloads even if dependency bootstrap fails.
The Windows CI x264 overlay uses `mirror/x264` with the exact pinned
Git commit to bypass unreliable VideoLAN GitLab downloads; the regular
VideoLAN archive source and its SHA-512 check remain intact outside CI.
Git's pinned commit verifies the mirror checkout; it is not a claim that
GitHub's differently generated archive matches the VideoLAN SHA-512.
Updating the MSVC pin or x264 source is a reviewed dependency change.

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
unit tests; installer smoke on the latest candidate; a **separate manual** NSIS upgrade
over an installed official 1.5.7; preservation of user settings and shortcuts;
local OCR model bootstrapping and all three AI Studio model responses. A version-number change alone does not prove
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

Every pull request enters the lightweight preflight, while the expensive
Windows job runs only when the workflow is **explicitly dispatched**:

1. The Ubuntu preflight runs upstream-sync policy tests, deletion guard,
   Qt/CMake contract tests, PaddleOCR fixture self-test, and the release
   helper self-test. Documentation-only and native PR changes stop here.
2. On one deliberate `workflow_dispatch`, the Windows job checks the exact
   MSVC/SDK toolchain, restores dependency caches, and builds focused
   `snow-shot-msvc-test` C++/Qt tests:
   `snow-shot-paddleocr-client-tests` and
   `snow-shot-recognition-session-controller-tests`. It never substitutes
   a text-only check for the real compiler, CMake or CTest.
3. The same Windows job tests NSIS/CPack, validates production Cargo licenses,
   builds `snow-shot-msvc-release`, creates both real Full NSIS installers,
   verifies SHA-256/manifest assets, silently installs the online variant,
   upgrades it in place with the offline variant, checks OCR payloads,
   uninstalls, and uploads verified CI artifacts.
4. `publish_prerelease=false` stops after artifact validation. Only
   `publish_prerelease=true` on current, reviewed `main` can continue
   to the draft/prerelease publication gate; the release source guard fails
   closed on other branches.

The pipeline caches dependencies and the audited static Qt kit, not CMake/MSVC application build
trees. This avoids stale compiled state while retaining the expensive dependency work.

**CI budget for maintainers and automated agents:** Follow
[repository Agent guidelines](../AGENTS.md). Run only locally relevant
formatting, static and targeted tests before pushing; consolidate verified
changes into one commit rather than a push-per-failure loop. Keep PR
`concurrency` cancellation enabled for obsolete runs. Avoid toggling
`ready_for_review`, no-op commits, manual reruns or automatic full-build
dispatches to force CI. Analyze the last failure log, freeze a candidate,
then spend **one** intentional full Windows CI run on it. If the run fails,
fix and test a batch of problems before the next manual acceptance run.
The workflow uses a new concurrency namespace so the first lightweight
PR check does not cancel the already-running previous full Beta check.
Do not infer Windows acceptance for a new head commit from a run on an older SHA.

The separate source-grep `test-community-*` scripts, standalone PaddleOCR CMake harness, and
duplicate OCR license workflow are intentionally removed. The production Cargo license preflight
already resolves the locked Windows release graph and runs the same collector used by packaging.

### Prerelease publication

After verifying a frozen Beta head with `publish_prerelease=false`,
complete the official 1.5.7 installed-upgrade check, obtain review approval
and integrate the accepted candidate. Then start **Community Windows x64**
manually from current `main` with `publish_prerelease=true`. The workflow
runs the same focused Windows tests and a clean production
build first. Only after the six local release assets pass verification does it create the version
tag, create or reuse a draft release, upload the assets, download them again and run the same asset
verification on the GitHub-hosted copies. The draft is then published as a prerelease.

A failed build creates no release tag. If publication fails after a draft was created, rerun the
same workflow while the release is still a draft; an already-published release is never silently
overwritten.

### Upstream synchronization

The scheduled upstream classifier still auto-merges only documentation changes that do not touch
fork-protected paths. Source changes require Draft PR review. Existing human-reviewed upstream
PR branches are preserved without force-push; new review branches use stable upstream/base SHA
names. A clean integration contains a real two-parent Git merge. Conflicts open a real upstream
review candidate if GitHub permits; workflow-token restrictions may instead require an explicit
blocked tracking PR with a pending marker and mandatory upstream-ancestry checks. Report-only PRs
must never be mistaken for merged code. See [upstream sync](community-upstream-sync.md) and the
[architecture review](community-upstream-architecture-review.md).
