# Snow Shot Community v1.5.8-beta.1 — testing prerelease checklist

**Status: candidate undergoing validation.** This is one Windows x64 Community
product, without a Mini edition. Local screenshot/pinned histories default on;
Online OCR is PaddleOCR-only and user-triggered; Windows updates are manual.

## Required before publishing a testing prerelease

- [ ] Final candidate SHA and reviewed upstream ancestry/Community protections.
- [x] Quick CI (actionlint, ShellCheck, zizmor, Ruff, PSScriptAnalyzer, Lychee,
      Cargo manifests, Git lifecycle, real Clang/MSVC, Qt AUTOMOC and CTests)
      passed on the source tree at `f9041c01`, Actions #161; the final
      release-acceptance run repeats these checks on its exact final SHA.
- [ ] Full Windows x64 production MSVC/static Qt build and the actual native
      CTests on the exact final candidate SHA.
- [ ] Real Windows online/offline NSIS installers built, with OCR assets,
      checked manifest metadata and SHA-256 sidecars, and automated installer smoke.
- [ ] Only the one Community x64 product; no Mini or ARM64 release assets;
      original author and third-party attribution kept intact.
- [ ] No outstanding failure hidden by an older green CI run.

## Hands-on testing after the testing prerelease is available

These checks require an actual Windows installation and/or live provider
credentials. Record results before recommending this build for general use:

- [ ] Fresh install and launch; local capture and pinned histories are on by
      default, remain on disk, and do not upload automatically.
- [ ] Upgrade an installed official 1.5.7 while preserving prior opt-outs,
      shortcuts, saved screenshots, OCR history and user settings.
- [ ] Local PP-OCRv6 using the actual bundled model files while offline.
- [ ] Explicit online OCR with each of the three real PaddleOCR AI Studio models,
      plus handling of invalid credentials, failures, timeouts and retries.
- [ ] Settings expose no Vision OCR fallback or unsolicited cloud connection;
      manual update button opens this fork's Releases page.
- [ ] Offline startup, installed UI smoke, rollback and uninstall.

## Evidence ledger

| Check | Commit / workflow / artifact | Result |
| --- | --- | --- |
| Final candidate SHA | Pending | Unverified |
| Quick CI | [#161](https://github.com/wxyu71/snow-shot/actions/runs/45119471522) at `f9041c01` | Passed: Preflight; Clang 9/9, native CTest 3/3; Windows MSVC OCR widget and Qt CTest 2/2 |
| Production Windows build and native CTests | Pending | Unverified |
| NSIS online/offline and automated installer smoke | Pending | Unverified |
| Personal testing prerelease | Pending | Not published |
| Installed OCR and upgrade on user's PC | Pending | Hands-on testing |

Publishing first requires a complete, successful production build and package
smoke. The GitHub release workflow operates on reviewed `main`, not on an
unmerged Draft PR. Do not misrepresent after-install manual verification as a
completed CI test or bypass failed acceptance.

macOS Sparkle/Homebrew and Windows ARM64 remain separate release projects.

See [the Windows runbook](../community-windows.md),
[CI design](../community-ci-architecture.md) and
[architecture overview](../architecture/overview.md).
