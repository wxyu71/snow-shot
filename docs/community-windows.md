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

## Verify and publish

The Community Windows x64 workflow validates policy, source contracts and native smoke tests. Ready code PRs and relevant main pushes also run the full static Qt build, Windows CTest, packaging and real installer/upgrade smoke. Draft PRs receive fast feedback. The Qt binary cache is keyed by the actual toolchain, dependency ABI, source and feature policy; cache hits are validated rather than repaired silently.

The version in the root CMake configuration is the release identity. A version change on main requests a prerelease; manual dispatch with `publish_prerelease=true` is also available on main. Publication requires a successful production build and acceptance, verifies online/offline installers, SHA-256 files and manifests, stages a draft and downloads the assets again before making it public. It never replaces a published version.

Install from this fork's [Releases](https://github.com/wxyu71/snow-shot/releases). Automatic executable self-update remains disabled without an independent trusted signing key. Check the installer SHA-256 and close the running application before upgrading. Back up important history and settings; automated installer smoke does not prove migration from every previously installed version.
