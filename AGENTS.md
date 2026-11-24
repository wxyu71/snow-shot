# Repository Guidelines

## Project Structure & Module Organization
- `snow_shot/` – Snow Shot app (GPL-3.0). Code lives in `src/{app,image,network,platform,presentation,storage}`, tests in `tests/`, assets in `resources/` and `i18n/`.
- `snow_image_viewer/`, `snow_image/`, `ant_design_qt/`, `snow_draw_engine_qt/` – Qt/C++ libraries and the viewer app; each has its own `CMakeLists.txt`, `tests/`, and license file.
- `snow-crates/crates/*` – Rust workspace (`snow-capture`, `snow-ocr-process`, …); `snow_rust_ffi/` bundles the C FFI static library consumed by CMake.
- `cmake/` – shared modules, `vcpkg-overlay-ports/`, `vcpkg-overlay-triplets/`, `test-support/`. `scripts/` – PowerShell entry points. `build/<preset>/` and `.tools/vcpkg/` are generated; never commit them.

## Build and test
- Community production: Windows x64 full edition only. Use the pinned toolchain, presets and packaging instructions in `docs/community-windows.md`.
- Use `scripts/bootstrap.ps1`, `scripts/build.ps1`, and focused CTest/Cargo tests. Benchmarks require the Release performance preset.
- Preserve other platform sources for upstream compatibility, not Community distribution.

## Coding Style & Naming Conventions
- C++: `.clang-format` (LLVM base, 4 spaces, 100 columns, attached braces, `T* ptr`, includes unsorted). Strict warnings are on; `.clang-tidy` is enforced when `SNOW_APPS_ENABLE_CLANG_TIDY=ON`.
- Headers are lowercase without separators (`screenshothistoryservice.h`), namespaces `snow_shot::presentation`, classes `PascalCase`, functions `camelCase`. (snow_image_viewer and ant_design_qt use snake_case filenames; match the file you edit.)
- Rust: edition 2024, `rustfmt.toml` (100 columns), `cargo clippy -D warnings` must pass. Crates are kebab-case `snow-*`.
- `.editorconfig`: UTF-8, LF, final newline; JSON/TOML/YAML use 2 spaces.

## Internationalization
- Source language is `en_US`. User-visible copy (labels, titles, tooltips, accessible names/descriptions, errors, settings text) must be English wrapped in Qt translation APIs (`tr()`, `QCoreApplication::translate()`, `QT_TRANSLATE_NOOP`, or helpers such as `settingsText` / `TranslatableText`). Do not hardcode zh-CN or zh-TW in C++.
- Keep IDs, config keys, object names, and protocol values as `QStringLiteral`. Do not wrap those, and do not use translated text as a stable identifier.
- After adding or changing translatable strings, extract with `snow_shot_update_translations` or `ant_design_qt_update_translations`, then fill **every** catalog for that target. Leave no `type="unfinished"` (`snow_shot` `lrelease` uses `-fail-on-unfinished`). Catalogs: `snow_shot/i18n/<feature>/snow_shot_<feature>_{en_US,zh_CN,zh_TW}.ts` (`en_US` is identity, apart from language-name metadata and plural forms; context ownership is in `snow_shot/i18n/modules.json`); `ant_design_qt/.../i18n/ant_design_qt_{zh_CN,zh_TW}.ts` (English is source; no `en_US.ts`). Use Simplified in `zh_CN` and Traditional in `zh_TW`; preserve `%1` / `%n` placeholders. Widgets that cache strings must retranslate on `QEvent::LanguageChange`.

- Snow Shot catalogs are split by feature and merged only under `build/`. Use the application-wide `snow_shot_update_translations` target; do not run `lupdate` directly on all fragments or extract from test source subsets. Assign new contexts in `snow_shot/i18n/modules.json`. Keep catalogs sorted without source locations; see `snow_shot/i18n/README.md`.

## Testing Guidelines
- After any code change, run **only** the tests that cover those changes. Running the full suite is **strictly prohibited** unless the user explicitly asks.
- Legacy tests are plain executables with a `require(condition, message)` helper; **new native smoke contracts use Qt Test** (QCOMPARE/QVERIFY) and real production C++ sources, registered through CMake/CTest. The focused independent `ci/native-smoke/` project uses system Qt to catch logic/overload errors rapidly; it does not replace Windows production ABI validation. Existing native tests retain their target-specific CTest names.
- Name files `<feature>_tests.cpp`; benchmarks `<feature>_performance_benchmark.cpp` or `<feature>_benchmark.cpp`; UI Automation runs `<feature>_uia_e2e_test.cpp`.
- CTest labels: `unit` (default), `interactive`/`e2e`, `benchmark`, `windows`. Default presets exclude interactive, e2e, and benchmark labels.
- When running performance benchmarks, compile with **Release** via `windows-msvc-performance`. Debug and packaging presets do not build benchmark targets. Still run only the related benchmark, not the full performance suite.
- Rust tests are mostly inline `#[cfg(test)]` modules (a few crates also have `tests/` integration tests). After a crate change, run `cargo test -p <crate>` (optionally `-- <test_name>`) inside `snow-crates/`. Do not run `cargo test --workspace` unless the user explicitly asks.
- Cover every behavioral change with a deterministic, offscreen-capable test.

## CI and release
- Draft PRs run fast checks; mark ready for full Windows acceptance. Merge only the tested integrated head and retain Git ancestry. Do not use commit trailers, empty commits or suppressed errors to trigger or bypass gates.
- Prefer established linters and real native tests. Require full Windows build, CTest, NSIS packaging, checksums and installer smoke before releasing.
- Main version changes request a prerelease. Only the reviewed main SHA may publish after successful acceptance; do not overwrite published releases.
- Use exact toolchain/dependency ABI keys and validated immutable caches. Investigate failures at their source.
- Keep docs under `docs/`; use the canonical navigation and avoid duplicate runbooks.

## Commit & Pull Request Guidelines
- Conventional Commits, lowercase imperative: `fix(screenshot): stop cursor flicker during smart frame selection`. Types in use: `feat`, `fix`, `refactor`, `perf`, `test`, `style`, `build`. Scopes mirror features (`recording`, `pinned`, `dpi`, `ocr`, `toolbar`).
- PRs: state motivation, list affected presets/targets, link issues, and confirm format, lint, and related tests pass. Do not run the full suite for a PR unless the user explicitly asks.

## Licensing & Configuration
- Multi-license repo: keep Apache-2.0 code in `ant_design_qt/`, `snow_draw_engine_qt/`, and `snow_memory/`; keep GPL-3.0-or-later code in `snow_shot/`, `snow_image/`, `snow_image_viewer/`, `snow-crates/`, and `snow_rust_ffi/`. The independent `snow-crates/crates/simd-paddle-ocr-rs/` submodule retains Apache-2.0. After adding a dependency, run `scripts/collect-third-party-licenses.ps1` and update the project's `THIRD_PARTY_NOTICES.md` (`snow_shot/`, `ant_design_qt/`).
- `SNOW_APPS_API_BASE_URL` is set per preset; never commit secrets; retain the fixed Community release origin.
