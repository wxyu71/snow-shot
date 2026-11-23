# Community Windows CI cache contract

This is a maintenance contract for the Community fork's Windows x64 build.
Caches accelerate deterministic builds; they are **never** a source of truth.

## Three different kinds of cache

| Cache | Content | Key / compatibility decision |
| --- | --- | --- |
| vcpkg binary archives | Packages produced by vcpkg | Inputs from the manifest, overlays and bootstrap plus a hash of the actual compiler/linker/CMake toolchain. **vcpkg additionally checks the ABI hash of each individual package.** |
| Cargo sources | Downloaded Cargo registry and Git sources; no build output | Lockfile and toolchain/configuration inputs. Rust/Cargo verify downloaded content. |
| Static Qt installation | Full locally compiled Qt SDK, not downloaded prebuilt Qt | SHA-256 identity of source archive, audited feature policy and source patches, actual zlib/libpng installed vcpkg ABI records, MSVC/linker/SDK/CMake/PowerShell/build tool identities, build scripts, target architecture, configuration and development-module selection. |

The Qt cache identity is calculated by `scripts/qt-cache-identity.ps1` **after**
`scripts/bootstrap.ps1` has resolved and installed the actual vcpkg dependencies.
This matters: a source manifest does not uniquely identify a built binary when
vcpkg package ABI or the hosted compiler changes.

Qt uses an **exact** cache key and deliberately has no `restore-keys`.
Restoring an ABI-compatible *package archive* is vcpkg's responsibility;
restoring a compiled Qt SDK from a different toolchain is not safe.

A Qt build is saved only after `build-static-qt.ps1` validates its
`static-qt-build.json` stamp, actual exported Qt features, required
development modules, licensing provenance and host tools. A cache hit is
validated again on every run. If the exact-key cached kit fails validation,
CI fails: it does not silently run `-Force`, accept bad libraries, or write
a replacement cache entry using an immutable key. Investigate a missing
identity input, a corrupted cache entry, or an upstream build regression.
Delete only the particular corrupted cache entry from GitHub Actions cache
management before rerunning; do not invent a `-v2` / `-r2` key.

The repository has no manual cache generation numbers. Changes to the actual
source, declared dependency ABI, target toolchain or policy compute a new
cache key automatically. The on-runner installation location still includes
the audited Qt version for clarity; that path is **not** a cache revision.

## Test and failure triage

Run `pwsh -File scripts/test-qt-cache-identity.ps1` on a Windows build host.
The test uses real build tools and isolated fake vcpkg ABI records. It checks:
identical inputs reuse a key; changing libpng ABI changes the key; changing
zlib ABI changes the key. The test needs no Qt download or compilation.

When a cache miss triggers a complete rebuild, the normal Qt source archive
SHA-256 verification, native architecture validation, static feature checks,
Qt license checks and CTest gates still apply. Never skip tests because a cache
hit occurred.

GitHub `actions/cache` entries are immutable. For more information:
- [GitHub dependency caching](https://docs.github.com/en/actions/reference/workflows-and-actions/dependency-caching)
- [Microsoft vcpkg binary caching and ABI hashes](https://learn.microsoft.com/en-us/vcpkg/reference/binarycaching)
- [Microsoft vcpkg binary cache troubleshooting](https://learn.microsoft.com/en-us/vcpkg/users/binarycaching-troubleshooting)

Notes: vcpkg's former `x-gha` binary source has been removed from recent
vcpkg releases. We retain its supported `files` binary cache with native
GitHub Actions caching rather than introducing an obsolete backend or
requiring personal access credentials for GitHub Packages.
