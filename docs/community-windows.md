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

The CI pipeline separates **intermediate commits** from **completed user
goals**, not routine development from real testing. Ordinary PR updates run a
fast Ubuntu workflow/security preflight **followed by real compiled C++/Qt
contract tests**; a failed early native test prevents the full Windows job. When a goal is implemented and locally reviewed, the
Agent must put `CI-Full: true` on its final commit (a standalone commit
message trailer). For same-repository PRs, that single PR update then
automatically enters the full Windows job **even when PR #2 is Draft**. No
extra draft/ready toggling or empty commit is required. A maintainer may
instead use **Actions > Community Windows x64 > Run workflow** on the exact
candidate branch with `publish_prerelease=false` (avoid duplicate runs).

The full goal-completion run compiles the focused native C++/Qt tests, builds
the production Windows x64 executable, creates both real Full online/offline
NSIS installers, validates checksums/manifests, performs online-to-offline
in-place install and uninstall smoke, and uploads six verified CI artifacts.
An ordinary green preflight is never sufficient evidence of a completed
goal. CI artifacts are test outputs, not approved release binaries.

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
The vcpkg **binary** cache is keyed by the selected MSVC/SDK versions and
dependency inputs. vcpkg's official SHA-addressed `X_VCPKG_ASSET_SOURCES`
asset cache complements a separate `VCPKG_DOWNLOADS` archive cache.
The Windows workflow optionally configures GitHub Packages NuGet as a per-port
binary provider, retaining the local vcpkg file cache as a fallback. This uses
supported providers, **not** the removed `x-gha` binary backend.
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

A completed user goal still requires **one full Windows production build and both real NSIS
installers** on the final SHA. The stages are deliberately ordered: cheap workflow/policy
checks, **compiled native** C++/Qt tests, then the slow production static-Qt Windows build.
A CTest contract that fails in seconds must not be discovered for the first time after
70+ minutes of static MSVC linking.

1. **P0 — workflow and supply-chain preflight:** `actionlint v1.7.12` checks
   GitHub Actions YAML, expressions and embedded ShellCheck; `zizmor v1.32.0`
   checks workflow security and SHA pins. `clang-format` checks new native
   code; `PSScriptAnalyzer` checks changed PowerShell. Domain-specific
   upstream trust, privacy, immutable source and release policy tests remain
   fail-closed. Generic text-matching Python assertions for Actions and C++
   behavior have been removed.
2. **P0 — compiled native smoke:** `ci/native-smoke/` uses CMake Presets,
   Ninja, Ubuntu distribution Qt6, Qt Test and CTest. It compiles the **real**
   PaddleOCR client and Community OCR settings sources, plus the exact
   `QComboBox` engine population implementation from the production settings
   widget. It checks userData, normalization and persistence in real Qt
   objects and produces JUnit and CTest diagnostics. This detects functional
   errors early but does not claim Windows ABI compatibility.
3. **P1 — Windows dependency supply:** Check the exact supported MSVC toolset
   and Windows SDK; restore binary/source caches and a SHA-addressed vcpkg
   asset cache; configure an optional GitHub Packages NuGet per-port binary
   cache with the local file cache as fallback. Restore/build the audited
   **static `/MT` Qt** kit; run the production Cargo license graph before
   expensive test links.
4. **P1 — native and production build:** Configure `snow-shot-msvc-test`,
   build real cross-layer test executables, run `test-snow-shot-msvc-community`
   with JUnit, and then build the actual production `snow-shot-msvc-release`
   executable. Upload a structured MSBuild `.binlog`, JUnit, CTest
   `LastTest.log`, CMake configure logs and a production transcript after
   success **or failure**, rather than rerunning a large job simply to debug.
5. **P2 — installable delivery:** Build online/offline Windows x64 Full NSIS
   installers, verify manifests and SHA-256, install/upgrade/uninstall on
   Windows, and upload candidate artifacts. Every completed same-repository
   goal with exact standalone `CI-Full: true` triggers this full acceptance;
   external forks cannot invoke expensive trusted jobs via commit trailers.

**Warning policy:** A successful CTest/NSIS run with unresolved warnings is
*not* release approval. Do not globally silence Qt/CMake warnings, MSVC or
Cargo diagnostics, or set `continue-on-error` on tests. The previously
observed Qt policy warnings, PCRE2 conversion warnings, Qt `QVariant::save`
serialization warnings and cross-thread timer messages require targeted
source or reviewed upstream fixes. Offscreen Qt uses the real Windows font
directory to avoid the known missing-font environment warning. Record the
exact fix and link the diagnostic to the same tested SHA.

Publication remains separate and **manual only**: after a successful full
run on reviewed current `main`, plus human validation of an in-place
upgrade from an actually installed official **1.5.7**, use
`workflow_dispatch` with `publish_prerelease=true`. Ordinary PR events
never create a tag or public release. Preserve PR Draft state during
unreviewed development and never reuse an older run's success after the
candidate SHA changes.

The workflow deliberately does not cache mutable CMake/MSBuild application
build trees, but it does reuse immutable dependency sources and audited Qt
outputs. See [CI architecture and failure investigation](community-ci-architecture.md)
and [repository guidelines](../AGENTS.md) for the staged engineering policy.

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
