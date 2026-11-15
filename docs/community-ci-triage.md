# Community Windows CI failure triage and gates

Verified from the GitHub Actions job/step logs on 2026-11-15. These are
**first actionable errors**, not the generic `build.ps1:86` exception emitted
after MSBuild aborts.

| Windows run | Failing job / step | First actionable cause | Category |
| --- | --- | --- | --- |
| [#21](https://github.com/wxyu71/snow-shot/actions/runs/43624092479) | build-full-installer / Build or validate audited Qt kit | First-run Qt download destination directory missing before File.Open | Qt bootstrap |
| [#25](https://github.com/wxyu71/snow-shot/actions/runs/43629410455) | build-full-installer / Build full Windows x64 | snow-capture/follow_mouse.rs:442 uses deprecated AtomicU32::fetch_update under -D warnings | Rust API |
| [#26](https://github.com/wxyu71/snow-shot/actions/runs/43664415633) | build-full-installer / Build full Windows x64 | snow-audio-recorder/control.rs:149,248 use deprecated AtomicU64::fetch_update | Same Rust API category |
| [#27](https://github.com/wxyu71/snow-shot/actions/runs/43670691161) | build-full-installer / Verify version, safety policy and offline API contracts | New full-tree Rust preflight reported 5 more deprecated calls | Same Rust API category, now caught early |
| [#28](https://github.com/wxyu71/snow-shot/actions/runs/43670848752) | build-full-installer / Build full Windows x64 | translationservice.cpp:128 connects deleted SnowShotApiClient::accountIdentityChanged | Removed account API / C++ |
| [#29](https://github.com/wxyu71/snow-shot/actions/runs/43675295729) | build-full-installer / Package full Windows x64 NSIS installers only | package-snow-shot.ps1:141 cannot find makensis after successfully linking snow_shot.exe and staging PP-OCRv6 assets | Missing packaging tool, unrelated to compiler/API failures |
| [#30](https://github.com/wxyu71/snow-shot/actions/runs/43683967993) | build-full-installer / Package full Windows x64 NSIS installers only | package-snow-shot.ps1:471 requires `SNOW_APPS_BUILD_SNOW_SHOT_MINI:BOOL=ON` despite release preset and CI guard requiring OFF | Inconsistent FullOnly/Store packaging contract; native x64 Full build succeeded |

All tracked Rust `.rs` files were checked for the deprecated atomic call
and repaired with `try_update` while preserving memory order. The
`scripts/test-community-cpp-api-contract.py` test now scans every tracked
C++/Objective-C++ source/header for a qualified `SnowShotApiClient::member`
not declared in the corresponding class header; it also explicitly forbids
references to the removed account identity signal. It is a guard, **not a
replacement for compiling the native C++ application**.

## New fast-to-slow pipeline

`preflight` (Ubuntu; upstream deletion protection, offline PaddleOCR,
Qt/CMake config contracts, branding, all Rust and C++ source references,
release selftest) and `verify-paddleocr-contract` (Ubuntu; compiles and
executes actual C++ Qt three-model client tests) start concurrently.
`build-full-installer` **requires both jobs to succeed**. A failed quick
check prevents allocation of the expensive native build, but cannot produce
a green release. Release tags additionally require matching version,
reviewed commit ancestry, native NSIS installers, trusted manifests and
SHA256 checks; publication requires the successful Windows and C++ jobs.

The independent community safety, Rust OCR license, and Qt contract workflows
remain quality checks. Their obsolete PR attempts are canceled on new pushes;
tag releases are not canceled. The Windows PR group already cancels superseded
builds.

## Bottlenecks observed in Windows run #28

- Even with validated static Qt restored from cache, the initial vcpkg
  bootstrap installed 33 dependencies from source and reported **21 minutes**.
  A manifest/pinned-overlay keyed vcpkg binary archives cache restores these
  exact packages; after successful bootstrap it is saved **before** later
  application compilation can fail. Successful dependency installation and
  architecture/version checks still execute; cache bytes are not trusted as
  an excuse to skip those checks.
- `build.ps1` previously ran `bootstrap.ps1 -SkipDependencyInstall`
  again after the workflow's full `bootstrap.ps1` step, repeating Rust
  toolchain and cargo metadata checks. CI now passes `-SkipBootstrap` after
  the one successful bootstrap, leaving CMake configure/build and all other
  native validation unchanged.
- The already verified static Qt 6.12.0 kit uses a separate pinned cache,
  validated on every build; this cache is saved ahead of the app build.
- Cargo registry/git sources are cached separately by relevant lockfiles,
  including on a non-cancelled native compilation failure. Rust compilation
  outputs are not blindly cached: massive target directories, changing MSVC
  flags and workspace features can make that expensive and unsafe.
- Source contracts execute before Windows toolchain provisioning; the true
  Windows C++/Rust build and NSIS packaging remain required release gates.

**No installer, speed improvement or GitHub Release should be claimed as
verified until the corresponding new CI run finishes successfully.**

## Windows #29 packaging prerequisite failure and fix

Windows run #29 passed the separate Linux `preflight` and the compiled
Qt C++ PaddleOCR contract test. On the native runner, pinned vcpkg packages,
Rust builds and the **complete Windows x64 Full executable** built successfully
(`snow_shot.vcxproj -> .../Release/snow_shot.exe`). It staged the local
PP-OCRv6 model assets. `SNOW_APPS_BUILD_SNOW_SHOT_MINI=OFF` also passed.

Packaging then failed before generating either installer because the runner
image did not contain `makensis` (`scripts/package-snow-shot.ps1:141`).
Thus #29 is **not** a continuation of the earlier Rust deprecated-atomic or
C++ deleted-account-signal failures. It is a missing NSIS prerequisite.

`scripts/bootstrap-community-nsis.ps1` now pins Chocolatey NSIS 3.12.0,
checks the actual executable version and exports its directory to the build
PATH. It performs a real minimal CMake/CPack NSIS installer build before the
workflow spends resources on vcpkg, Qt, Rust or application compilation.
A failed install, missing compiler, incompatible CPack generator or empty
smoke installer causes a non-optional failure. Static Linux checks verify that
the smoke step runs before the expensive Windows steps and that the complete
native installer and release-integrity guards are still required. The normal
CPack online/offline NSIS packaging and native probing must still succeed.

#29 also saved the pinned vcpkg binary archives and Cargo source caches;
subsequent attempts may reuse them, subject to key validity. The successful
Windows binary from #29 was **not uploaded** because final packaging failed,
so no installable release artifact exists from that run.

## Windows run #30: Full-only release contract

Run #30 passed both Linux gates, compiled `snow_shot.exe` for x64 Full,
staged the OCR assets, and passed the explicit Mini=OFF verification. The
packaging script alone retained legacy assumptions at lines 450 and 476:
it required Mini=ON whenever `-DistributionChannel standalone`, and
would select `snow_shot_mini` for a build with `-FullOnly`. The error at
line 471 was not a genuine unsafe CMake cache or a compiler failure.

The packaging script now derives one `$miniEnabled = -not ($store -or
$FullOnly)` value, shared by the strict cache check and the optional native
target list. All existing static Qt/CRT, OCR manifest, native startup and
installer SHA-256/manifest release checks remain enforced.

Fast Python source-contract tests and an **actual Windows PowerShell
four-mode test** (legacy standalone, standalone FullOnly, Store, Store
FullOnly) run before dependency installation. The PowerShell test evaluates
expressions extracted from the *production package script* to prevent
future disagreement between cache requirements and build target selection.

### Build output reuse limitation and mitigation

Run #30 did not upload any build artifacts because installation packaging
failed; its hosted runner workspace is ephemeral. The successful executable
cannot be recovered from that run for an immediate packaging-only retry.
The next successful native compilation saves its entire
`build/snow-shot-msvc-release` CMake/MSVC/Rust build directory to a
source-keyed **PR-only** GitHub Actions cache *before* packaging. A
subsequent packaging-script-only change can restore this exact native state
and run the same native build command incrementally. A diagnostic copy
of the compiled Full executable is also uploaded before packaging, even
if the installer step fails. Neither mechanism bypasses native compilation
verification, trusted dependencies, installer integrity checks or release
gates. Tag releases cannot reuse this PR-native cache. Cache size/quota
limitations and initial cache misses may still require a full rebuild;
there is no claimed speedup until a cache restore proves it.
