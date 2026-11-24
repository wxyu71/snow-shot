# Windows x64 Community build and release

The Community release is the full Windows x64 edition only. Use a recursive submodule checkout, PowerShell 7, Visual Studio/MSVC, Windows SDK and the pinned dependencies selected by the bootstrap scripts.

## Build locally

```powershell
./scripts/bootstrap.ps1 -Preset snow-shot-msvc-release -VcpkgVariants Static -SkipQtValidation
./scripts/build-static-qt.ps1 -Architecture x64 -Configuration Release -DevelopmentModules
./scripts/build.ps1 -Preset snow-shot-msvc-release -SkipBootstrap
./scripts/package-snow-shot.ps1 -Architecture x64 -SkipBuild -FullOnly
```

Follow the Qt builder's printed installation path and bootstrap instructions if using a nondefault prefix. Production artifacts are in `build/snow-shot-msvc-release`.

## Development and testing

Follow [repository conventions](../AGENTS.md). Use small, reviewable changes, CTest labels/presets and focused Cargo tests. Runtime invariants belong in executable tests rather than source-text assertions; fail when an expected test is missing. Preserve upstream fixtures unless a reviewed behavior change requires migration. The independent Qt smoke project does not replace production ABI validation.

## OCR validation

The unified Community Windows x64 CI tests the real Qt/C++ PaddleOCR client against deterministic local HTTP responses for all three supported models, authentication, quotas and result parsing. The Python fixture self-test also runs without credentials. The optional live diagnostic remains local: install `requests` and `pillow`, supply your own `PADDLEOCR_ACCESS_TOKEN` in the environment, and run `python scripts/paddleocr_live_e2e.py --live`. It is not a release gate or a standalone workflow, and no repository credential is needed. Live provider behavior can change independently of the client.

## Verify and publish

The Community Windows x64 workflow validates policy, source contracts and native smoke tests. Ready code PRs and relevant main pushes also run the full static Qt build, Windows CTest, packaging and real installer/upgrade smoke. Draft PRs receive fast feedback. The Qt binary cache is keyed by the actual toolchain, dependency ABI, source and feature policy; cache hits are validated rather than repaired silently.

The version in the root CMake configuration is the release identity. A version change on main requests a prerelease; manual dispatch with `publish_prerelease=true` is also available on main. Publication requires a successful production build and acceptance, verifies online/offline installers, SHA-256 files and manifests, stages a draft and downloads the assets again before making it public. It never replaces a published version.

Install from this fork's [Releases](https://github.com/wxyu71/snow-shot/releases). Automatic executable self-update remains disabled without an independent trusted signing key. Check the installer SHA-256 and close the running application before upgrading. Back up important history and settings; automated installer smoke does not prove migration from every previously installed version.

## Next beta: modern EXE and manual in-app updates (development contract)

Use Inno Setup 7.1.1 as the initial candidate, with a Windows 11-style responsive wizard and system light/dark mode. Pin and authenticate the compiler download before CI use. Keep the existing online/offline full x64 staging and versioned EXE names, existing installation identity, scope, directory, owned-file inventory, shell registration, data paths and uninstall behavior. Do not switch production packaging until a real beta.1 NSIS-to-Inno upgrade, repair, cancellation, failure recovery, uninstall, reboot and data preservation test passes. Preserve other platform source. Do not add a second distribution framework or a new workflow.

About page target: explicitly Check for updates, Download update (progress/cancel), then Install update after verification. No browser detour, background download, automatic installation or install-on-launch. Reuse existing Qt update state and Rust helper coordination, graceful shutdown and restart; use the exact EXE variant matching the existing installation. Do not enable the old ZIP/portable updater against EXE-only releases. Keep the repository fixed to wxyu71/snow-shot, strictly compare SemVer, reject wrong product/architecture/tag, redirects outside the existing GitHub asset allowlist, unsigned metadata, mismatched size/hash, and stale/corrupted cached files. Verify again at execution and retain local user confirmation, UAC and Windows security checks. No elevated execution from a user-writable unverified path.

The new signed EXE-only payload schema 2 uses the existing RSA-3072/PSS-SHA256 envelope (32-byte salt). The future release asset `community-installers.json` binds the product, fixed repository, platform, version, timestamp and two exact EXE names, sizes and SHA-256 digests. It is additional to the six existing release assets. The key store remains empty until the maintainer provisions an independent production signing key; test keys never enter production. Signing is a separate protected release step, after final EXE bytes are produced, and verification must occur before publication and immediately before installation. Authenticode is separate from signed update metadata; Windows trust warnings must not be bypassed. Existing beta.1 cannot acquire new About-page code remotely: a one-time trusted manual installation of the first new updater-capable release is required.

Current status: signed metadata parsing and validation foundation only; the release signing tool, key provisioning, actual installer migration, update service transport/launch, About-page UI and end-to-end Windows tests must be completed before enabling this feature or publishing the next beta. The already published beta.1 remains unchanged.
