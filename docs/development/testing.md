# Testing strategy: observable behavior first

The presence of a function name or a workflow snippet is not evidence that
its behavior works. Test outcomes, native target registration, actual files
and process side effects instead of `source.read_text()` assertions.

| Layer | Community tool | Observable evidence |
| --- | --- | --- |
| GitHub Actions syntax / shell | actionlint, ShellCheck | Valid workflows and executable shell |
| Actions security / provenance | zizmor | Permission, pin and unsafe-trigger diagnostics |
| PowerShell scripts | PSScriptAnalyzer | Parsed rules and actionable findings |
| Upstream sync | Python unittest + temporary Git repos + real subprocesses | Merge ancestry, retries, permission failures and PR branch preservation |
| Qt/CMake configuration | CMake configure, Ninja, CTest | Resolved targets, feature and kit validation |
| Qt UI, OCR, persistence and network | Qt Test, native CTest | Real widget state, protocol and loopback HTTP effects |
| Rust OCR worker | Cargo test and CTest | Process, payload and error behavior |
| Internal Markdown links | Lychee offline | Actual local link targets, without network flakiness |
| Windows delivery | MSBuild, NSIS and installer smoke | Packaged app, checksums and in-place upgrades |

## How to run focused checks

```bash
python3 scripts/test-upstream-sync.py
python3 scripts/upstream_sync.py verify-review-ancestry
python3 scripts/upstream_sync.py verify-deletions
python3 scripts/test-qt-cmake-contract.py
(cd ci/native-smoke && cmake --preset community-native-smoke && \
  cmake --build --preset community-native-smoke && \
  ctest --preset community-native-smoke --output-on-failure)
```

Use the Windows pipeline described in
[CI architecture](../community-ci-architecture.md) for complete MSVC,
native Qt, NSIS and installed-upgrade acceptance. It must run on the exact
final Beta candidate with the reviewed `CI-Full: true` commit trailer,
not on a previous successful SHA. For macOS use the
[native platform guide](../platforms/macos-build.md).

Retain production JSON and manifest *data contract* checks where they
protect release inputs, but never claim that a text-scanning Python test
validates C++ behavior, bundled OCR, OS permissions or package signing.
Do not replace these layers with new homegrown parsers when a maintained
community tool or native executable already handles the job.

## Fast-fail gates for Community Windows

Full Windows release linking uses MSVC and audited static Qt and remains a
**release acceptance** step, not an appropriate first detector for tiny
configuration-policy mismatches.

1. Every code PR runs the Linux system-Qt `native-smoke`, including
   `snow-shot-community-native-toolbar-policy-tests`, which calls the
   production Community action availability and floating-toolbar default
   functions against the immutable Windows Full baseline. It is a real native
   executable, not a source-text grep.
2. Once the reviewed `CI-Full: true` trailer requests Windows acceptance,
   build and execute the existing production
   `snow-shot-configuration-schema-tests` **first** with CTest `-R`.
   Only after it passes link the other, more expensive UI/OCR acceptance
   executables. Incremental MSBuild reuses the compiled schema target.
3. Continue with full native acceptance, production binary, NSIS online/offline
   packages and in-place upgrade smoke. A fast green test never approves release.

Run the canary locally with the existing `ci/native-smoke` CMake preset:
```bash
cd ci/native-smoke
cmake --preset community-native-smoke
cmake --build --preset community-native-smoke --parallel 4
ctest --preset community-native-smoke --output-on-failure
```

For later performance tuning, compare the CI job step durations and CTest XML
at the same exact SHA. Consider proven compiler caches (e.g. sccache) after
separate verification against the MSVC/static-Qt toolchain; cache compilation
but do not treat cache hits as proof of correct linkage or installed artifacts.

### Static C++ declaration gate (before long-running Windows CI)

`scripts/check-cpp-member-declarations.py` uses the pinned, upstream
`tree-sitter` and `tree-sitter-cpp` parser wheels in the cheap Linux preflight.
It parses the **real** `ScreenshotToolPalette` class body and all of its
out-of-class member definitions across preprocessor branches. It fails if a
source definition names a member that is absent from the class declaration.
The mandatory `--self-test` reinserts the original
`createImageConversionActionFamily()` MSVC C2039 regression into the actual
source in memory and asserts that the parser rejects it. Minimum AST
declaration/definition counts prevent silent parsing gaps.

This is a **narrow structural preflight**. It does not compare C++ function
signatures or overload resolution, validate generated Qt/moc output or replace
Clang/MSVC compilation. For full semantic checks, configure the real
dependency graph, export `compile_commands.json` with CMake, and run
`clang-tidy -p <build-directory> <changed-translation-unit>` or
`clang++ -fsyntax-only` with the same compile arguments. Do not run a
compiler without project include paths and claim that it verified the app.
The original Windows Release build, native CTests, NSIS packaging and
installer-upgrade smoke remain mandatory release gates.

The preflight also runs `compileall` on Python scripts; existing actionlint,
zizmor, PSScriptAnalyzer, clang-format and Qt/CMake contract tests remain active.

### Real Clang semantic checks from CMake's compilation database

The optional `community-native-clang` CMake preset uses Ninja, Clang 18,
system Qt6 and `CMAKE_EXPORT_COMPILE_COMMANDS=ON`. Unlike the structural
Tree-sitter gate, `scripts/check-clang-compile-database.py` **executes real
Clang on production translation units** using the `-D`, `-I`,
`-isystem`, C++ standard and related arguments exported by CMake. Only
object/dependency-file output switches are removed; `-fsyntax-only`
still runs the preprocessor, parser and semantic analyzer.

On PRs, production sources changed since the PR's base commit are checked
first. Every production translation unit in this small native compilation
database is also checked, so edits to their included headers can be caught.
The job fails when the compiler is not Clang, the database lacks expected
production files, or any checked file has a semantic error. An additional
real-compiler negative test builds a tiny CMake compilation database and
checks that an undeclared class member is rejected and a fixed one accepted.

The `build/ci-native-clang/clang-syntax-report.json` artifact records checked
files, changed-source coverage, and changed production files **not present**
in this build graph. Uncovered sources are reported explicitly, not
silently counted as passing. The curated lightweight graph currently covers eight production `.cpp`
files: PaddleOCR client, cloud OCR settings, cloud OCR engine choices,
screenshot geometry, recording geometry, OCR text transform, OCR paragraph
layout and canvas status readout. The latter five are excluded from the
ordinary native test build but still undergo real Clang syntax analysis. The Windows Release compiler must
still validate the full app, platform macros, linking, ABI, NSIS and upgrade.

Local validation requires Qt6, Ninja and Clang 18:

```bash
cd ci/native-smoke
cmake --preset community-native-clang
cmake --build --preset community-native-clang --parallel 4 \
  --target snow-shot-community-native-paddleocr-tests_autogen \
           snow-shot-community-native-settings-tests_autogen \
           snow-shot-community-native-toolbar-policy-tests_autogen
cd ../..
python3 scripts/test-clang-compile-database.py
python3 scripts/check-clang-compile-database.py \
  --database build/ci-native-clang/compile_commands.json \
  --report build/ci-native-clang/clang-syntax-report.json
```

The original `community-native-smoke` preset remains available with GCC
for developers who do not have Clang 18 installed.

The more specialized `screenshotocrtextlayout.cpp` uses
`QTextLayout::RetrieveGlyphIndexes`, unavailable in Ubuntu 24.04's system
Qt 6.4. It is explicitly **not** covered by this older system-Qt gate;
the production Windows build uses Qt 6.12 and must compile it. Do not
modify that source merely to appease the older Clang preflight SDK.

### Pre-release fast-gate budget and upstream tooling (2026-11)

The verified #145 baseline was **78 seconds preflight + 53 seconds Linux
native Qt/Clang** (about 2m 11s sequentially). These are measured GitHub
runner log durations, not optimistic estimates.

The fast path now includes a **single Windows-native** contract job alongside
Ubuntu, using maintained, SHA-pinned `jurplel/install-qt-action` to install
Qt 6.12.0 desktop binaries and Visual Studio 2022 CMake/MSVC. No static Qt
source build, vcpkg bootstrap, Cargo graph or release installer is involved.
The real `communitycloudocrsettings.cpp` is compiled on Windows (including
DPAPI), `processpowerqos.cpp` is compiled against actual Windows SDK types,
the existing Settings and toolbar policy Qt/CTest binaries run natively, and
the production `CanvasStatusReadout` `Q_OBJECT` header/source are compiled
alongside **CMake-generated AUTOMOC output** (object files only; no full app
link). The same production Qt meta-object sources are compiled on Linux,
so a missing or invalid MOC declaration is not silently ignored.

A small selected set of real changed Linux production C++ is also analyzed
by upstream `clang-tidy-18` with `clang-analyzer-core.*` and
`bugprone-use-after-move`, reusing the actual CMake compilation database.
This targets possible dataflow/logic mistakes not covered by basic syntax
checking. No duplicate handmade C++ matcher or fabricated feature test was
added; the existing three native Qt/CTest cases still exercise production code.

**Hard limits:** Preflight 5 minutes; Ubuntu CMake/Clang/CTest 12 minutes;
Windows prebuilt-Qt/MSVC contracts 12 minutes. The independent Ubuntu and
Windows jobs run *in parallel* after Preflight, so the configured runner
execution upper bound is **17 minutes**, below the 20-minute fast feedback
budget (GitHub queue times and artifact service availability are external).
Actual Windows cold-install duration must be assessed from the first
successful run; if it consistently approaches the limit, prefer a cache or
reduce the Windows quick graph rather than extending its timeout. The full
Windows Release/NSIS/installer acceptance is separate and much more expensive.

The Windows fast contracts are **not** substitutes for the full Windows build:
they do not compile `screenshottoolpalette.cpp`, the static Qt release
configuration, every `Q_OBJECT` class, every Windows-only branch, or every
third-party library. The production Release build remains required. Do not
count compiler self-check fixtures as product features or treat uncovered
files as passing.
