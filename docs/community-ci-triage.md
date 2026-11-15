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
