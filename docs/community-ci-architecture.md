# Community Windows x64 CI architecture and incident review

This document explains the target-completion gates for Community Beta PR #2.
The goal is to detect preventable faults **before** the long MSVC/static-Qt
build, without treating a Linux smoke build as a Windows binary acceptance.

## Lessons from recent failures

| Run | First failing step | Cause | New earlier gate |
| --- | --- | --- | --- |
| [44915382390](https://github.com/wxyu71/snow-shot/actions/runs/44915382390) | vcpkg bootstrap | x264 source acquisition / environment passthrough | Pinned mirror and cache |
| [44918116046](https://github.com/wxyu71/snow-shot/actions/runs/44918116046) | vcpkg bootstrap | OpenCV GitHub commit-patch URL responded 503 | Pinned in-tree OpenCV patch |
| [44921713041](https://github.com/wxyu71/snow-shot/actions/runs/44921713041) | Native CTest after roughly 146 minutes | Old settings object name and startup-schema expectation | Compiled Qt and frozen-schema review |
| [44990677817](https://github.com/wxyu71/snow-shot/actions/runs/44990677817) | Native CTest after cache restore | QComboBox overload discarded engine userData; Community-only schema addition | Production option code compiled by fast Qt Test |

The former native test build/link took roughly **70 minutes** even when the
vcpkg and Qt caches were restored; its five CTest cases took about **23
seconds**. The production static `/MT` Qt kit includes `/GL` objects, and
MSVC may restart the link with `/LTCG` even when the test preset turns off
CMake interprocedural optimization. First analyze the collected
`community-tests-msbuild.binlog` with MSBuild Structured Log Viewer before
altering toolchain flags. Switching the release ABI or turning off tests
just to make a green badge is unacceptable.

## Stage-by-stage checks

| Gate | Runner | Standard tools | Failures and evidence |
| --- | --- | --- | --- |
| P0: syntax and security | Ubuntu | actionlint, embedded ShellCheck, zizmor, clang-format, PSScriptAnalyzer | Action syntax, unpinned/unsafe Actions, native code formatting and script warnings |
| P0: compiled native | Ubuntu | CMake/Ninja, Qt Test, CTest with distro Qt6 | Real PaddleOCR protocol, OCR settings persistence, QComboBox engine data, JUnit report |
| P1: dependency supply | Windows | vcpkg built-in file/NuGet binary cache, SHA-addressed asset cache, audited static Qt | Source/hash/CRT mismatches, license graph |
| P1: Windows acceptance | Windows | CMake/MSBuild/CTest, Rust/Cargo and exact MSVC pin | All focused native tests, MSBuild structured binlog, JUnit, CMake logs |
| P2: real installable build | Windows | production build, NSIS/CPack, SHA-256 verification, install/upgrade/uninstall | Full x64 online/offline candidate installers and six verified assets |
| Release | Maintainer-approved main | manual workflow_dispatch, audited GitHub artifact release | Explicit tag and prerelease only on current main |

The workflow's `needs` dependency makes the compiled Ubuntu smoke a
hard prerequisite for Windows. On ordinary commits the Windows job does not
run; the exact standalone `CI-Full: true` trailer on a completed trusted
goal (or a deliberate manual validation dispatch with publication disabled)
runs the **entire** Windows pipeline, not only a static code scan.

## Use maintained ecosystem tools, not new Python linters

- [actionlint](https://github.com/rhysd/actionlint) validates workflow
  syntax, expressions and embedded shell.
- [zizmor](https://docs.zizmor.sh/) audits GitHub Actions security and
  immutable action pins.
- [PSScriptAnalyzer](https://github.com/PowerShell/PSScriptAnalyzer)
  validates changed PowerShell files.
- [Qt Test](https://doc.qt.io/qt-6/qtest-overview.html) tests actual
  Qt object state, not source-code string fragments.
- [CTest](https://cmake.org/cmake/help/latest/manual/ctest.1.html)
  runs native tests and exports JUnit.
- [vcpkg binary caching](https://learn.microsoft.com/en-us/vcpkg/consume/binary-caching-overview),
  [vcpkg asset caching](https://learn.microsoft.com/en-us/vcpkg/consume/asset-caching)
  and [GitHub Packages NuGet](https://learn.microsoft.com/en-us/vcpkg/consume/binary-caching-github-packages)
  reduce dependency rebuilds while retaining immutable sources.

Retain Python where it implements **Snow Shot-specific fail-closed trust
rules**, such as upstream integration permissions, privacy defaults,
release manifests, OCR protocol versions and immutable model assets. No
generic Python string check can substitute for a Qt test, CMake configure,
Cargo compile or GitHub Actions parser.

## Diagnostic and Warning ownership

When a run fails, inspect the **first failing step**. Download
`community-native-smoke-<sha>` or `community-windows-diagnostics-<sha>`
from that exact commit. Review JUnit/CTest LastTest.log for failures,
MSBuild Structured Log Viewer for per-target compile and link times, and
CMake configure logs plus production transcript when they exist.

Fix warnings at their point of origin: application QObject lifetime and
serialization first; third-party Qt/PCRE2 warnings through a compatible
upstream patch or audited dependency update. All warnings remain visible.
Do not add global compiler suppressions, `QT_LOGGING_RULES`,
`--no-warn-unused-cli`, CMake `-Wno-*` or test
`continue-on-error` as a shortcut. Production remains audited
static Qt with the intended CRT linkage, and the manual upgrade test from
official installed 1.5.7 is still required after the final integrated
Windows SHA passes.
