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
