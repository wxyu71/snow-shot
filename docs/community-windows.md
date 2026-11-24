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

## Next beta: modern installer and transactional in-app updates (development)

The visible distribution remains two Full Windows x64 EXEs (online/offline). Use Inno Setup 7.1.1's modern dynamic Windows 11 style, a per-user default for new installs, and preserve existing installation scope and directory. Neither a normal upgrade nor a legacy migration should run an old uninstaller before copying new files. Treat a legacy NSIS installation record as an ownership manifest, not permission to delete arbitrary user files. Validate registration and scope, close only the affected application gracefully, stage and verify the full new payload, preserve user data and shortcuts, and commit new registration only after successful replacement. Existing NSIS and Inno uninstall identities must not produce duplicate Apps entries. Keep the old packaging route until real beta.1-to-beta.2 upgrade, rollback, scope, variant and uninstall tests pass.

For in-app updates, use the existing Rust transaction engine rather than launching a second full installer wizard or leaving a background installer running. Manual Check, Download, Verify, Install & Restart. No automatic install on startup or exit, no duplicate toast after a manual download, and no forced close while capturing/recording/exporting or while unsaved edits remain. Reuse the coordinator, backup journal, exact owned-file inventory, startup probe and rollback. A single lightweight native progress window can remain while the app is closed; elevation only when the existing installation requires it. Never force-dismiss UAC or Windows security warnings.

The signed schema 3 `latest-version.json` contains two EXE identities and two internal full update ZIP identities with exact inventories. Only the EXEs are user-facing installers; the ZIPs are update transport, not portable distributions. The signing helper independently checks both EXEs, both ZIP bytes and archive/installer ownership inventories. No new platform or edition. Public release publication must authenticate the signed feed and all assets before enabling production. RSA-PSS trust store remains empty until the maintainer independently provisions and reviews the real public key. The published beta.1 cannot acquire a new About page remotely: a one-time manual install of the first updater-enabled release is required.

Current draft status: manual About flow and schema 3 verifier/signing foundation staged; no production signing key, no final installer migration acceptance, no beta.2 publication. The CI deliberately blocks new version publication until these gates are implemented.
