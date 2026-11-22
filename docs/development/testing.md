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
