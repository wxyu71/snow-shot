# Snow Shot Community architecture

## Module boundaries

| Concern | Production owner | Primary verification |
| --- | --- | --- |
| Application lifecycle, edition and local policies | `snow_shot/src/app/` | Native Qt/C++ CTest |
| Windows and macOS platform services | `snow_shot/src/platform/` | Native platform tests |
| Screenshots, OCR UI, history, clipboard and export | `snow_shot/src/presentation/` | Qt Test and CTest |
| User-selected cloud OCR and HTTPS endpoints | `snow_shot/src/network/` | Loopback HTTP integration tests |
| Preferences, privacy defaults and DPAPI | `snow_shot/src/storage/` | Persistence and native OS tests |
| Local OCR worker / protocol | `snow-crates/crates/snow-ocr-process/` | Cargo and real process lifecycle tests |
| Rust ↔ C/C++ FFI | `snow_rust_ffi/` and `snow-crates/` | Cargo, ABI and native tests |
| Build, signing, packaging and staging | `cmake/`, `snow_shot/packaging/` | CMake, MSBuild, CTest, installer smoke |
| Synchronization and publication | `.github/workflows/`, `scripts/` | actionlint, zizmor, Git integration tests |

Public app headers belong in `snow_shot/include/snow_shot/<domain>/`,
with implementations under the corresponding `snow_shot/src/<domain>/`.
New targets and source files must be registered with the owning CMakeLists
and validated by a real configure/build. Keep fixtures beside module tests;
do not add production-only abstractions just to make a textual test pass.

## Community-specific architecture rules

- Independent Community branding, version and release/update endpoints.
- Do not restore upstream account, membership, billing, purchase or analytics.
- Offline OCR must remain local; cloud OCR sends screenshots directly to
  the selected provider with user-managed credentials.
- Windows DPAPI-protected credentials and privacy opt-ins must have actual
  persistence and native regression checks.
- Rust OCR protocol, native C++ consumer and pinned model manifests must
  remain compatible at runtime, not merely share source-text version strings.
- Keep official document-recognition service integration disabled until
  independently supported and tested in Community.
- macOS Sparkle, signing and feed origin require their own release approval.

For the recorded conflict decisions see
[the upstream review](../community-upstream-architecture-review.md).
For executable checks see [testing](../development/testing.md) and
[Beta1 acceptance](../releases/beta1-checklist.md).
