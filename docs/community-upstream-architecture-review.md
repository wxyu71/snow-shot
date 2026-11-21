# Community upstream architecture and placement review

Status: **review in progress; do not merge or publish**. Updated 2026-11-21.

## Exact Git scope

- Community merge candidate: [PR #2](https://github.com/wxyu71/snow-shot/pull/2), whose original imported upstream ancestor is `637f8caec369e4ebaf4db71ae3d0ffb2336361ea`.
- Official original: `mg-chao/snow-shot@7f34f24696c06200bc7d707eab4f35d0a7fbdae1` (2026-11-21).
- Incoming code: [Draft PR #5](https://github.com/wxyu71/snow-shot/pull/5) (head exactly the official commit; base the Community PR #2 branch). Not a merge. Its conflicts must be resolved on a separate reviewed integration commit before any acceptance.
- Official delta from `637f8cae` to `7f34f246`: **27 commits, 281 changed paths**, including extensive changes overlapping the Community OCR/UI tree.
- Upstream sync workflow hardening: [PR #3](https://github.com/wxyu71/snow-shot/pull/3), incorporated into the current PR #2 candidate for *combined-tree* testing.

These statements describe checked Git state, **not** test/functional approvals.

## Canonical ownership (from official AGENTS.md)

| Ownership | Canonical path | Decision |
| --- | --- | --- |
| App domain / platform / network / presentation / storage implementation | `snow_shot/src/{app,platform,network,presentation,storage}/` | Preserve these subsystem boundaries; do not relocate an official source based only on its name |
| Public app headers | `snow_shot/include/snow_shot/` | Keep matching APIs with their owning module |
| Native C++ tests and fixtures | `snow_shot/tests/` | Add real test target registration in `snow_shot/CMakeLists.txt` |
| Rust runtime / protocol documentation | `snow-crates/crates/snow-ocr-process/` | Keep `PROTOCOL.md` next to the protocol implementation, not in arbitrary root docs |
| Build integration | `cmake/`, `snow_shot/CMakeLists.txt` | Audit source registration, feature flags, package staging and target dependencies together |
| Community-specific product/operations documents | `docs/community-*.md` | Do not add parallel root or `scripts/` copies; validate relative links |
| Official root entry-point build documentation | `docs-macos-build.md`, `docs-windows-arm64-build.md` | These are upstream-established root conventions; moving them without coordinated link changes would itself be harmful |
| Third-party licensing | `licenses/`, appropriate `THIRD_PARTY_NOTICES.md` | Changes need provenance and redistribution review |

## Review findings and required dispositions

### Blocker — OCR runtime, protocol, assets and licensing must migrate as a unit

PR #2's `snow_shot/packaging/snow-shot-ocr-asset-manifest.json` still describes **snow-ocr-runtime 1.2.2**. The new official upstream manifest uses **1.3.0**, protocol **8** (see `snow-crates/crates/snow-ocr-process/PROTOCOL.md`), a new paragraph model artifact, Rust process and asset staging changes, and an updated `snow-crates/crates/simd-paddle-ocr-rs` submodule gitlink. Copying only C++ OCR presentation changes, only Cargo changes, or only the manifest can cause runtime protocol/asset incompatibility. Treat this as an atomic change set with SHA-256 and external source provenance verification, full Windows x64 native build, offline installer asset checks and recognition regression tests. The model artifact URL is on the original author's ModelScope namespace; this is an external binary trust boundary, not a Community-hosted dependency. The existing checked Community gitlink for `snow-crates/crates/simd-paddle-ocr-rs` is `43aba513b661e48eacb04d19f1ff071649116ddf`, whereas incoming upstream pins `a2fc7abfec5913618c78a0d2330ae520872c5abf`; audit the exact submodule diff and license/provenance before changing it. A new automated regression checks consistency between the Rust/C++ wire versions, `PROTOCOL.md`, both Windows manifests, pinned hashes and approved model download origin.

### Blocker — Community identity and feature contracts

Upstream `CMakeLists.txt` has advanced to **1.6.1**. Do not replace Community `1.5.8-beta.1`, the independent release URLs, manual-update policy, account/paid-service deletion list, direct PaddleOCR settings or disablement of unsupported official document recognition. Audit all account, membership, payment, telemetry, endpoint, update and signing differences; a clean Git merge is not evidence of policy approval. Keep macOS Sparkle signing/feed updates disabled as distributable Community behavior until separately configured, tested and approved.

### High — New presentation files and build target ownership

Incoming upstream adds `snow_shot/src/presentation/services/imageexporteditor.cpp`, `imagecompressionwindow.cpp` and their private helpers, plus `screenshotexporttasks.cpp`. The first two are widget/window-heavy despite living in the broader `services/` folder. This is an **architecture-review candidate**, not a proven file-placement bug: existing upstream save/export UI has related files in `services/`. Review whether to retain that original layout for safe syncing or refactor all dependent headers, private includes, CMake target source lists, translations and tests together into a dedicated presentation submodule. Do **not** move a single .cpp in isolation. New `snow_shot/tests/imagecompression_tests.cpp` and `export_dialog_test_support.h` belong under tests; corresponding registration must be checked.

### Verified placement and CMake coverage

The new C++ paths introduced by the existing PR #2 (recognized-document OCR, export service and private secret protection) were checked against their owning module and `snow_shot/CMakeLists.txt` registrations. The new Sparkle driver and native fixture are registered in the **included** `cmake/SnowShotUpdates.cmake`, not the root app source list; this is intentional. The 21 newly added upstream files seen in Draft PR #5 also use the established Rust/C++, `include/`, `tests/`, `resources/` and `presentation/` subtrees, with their new C++ sources/test names present in official `snow_shot/CMakeLists.txt`. **No confirmed mislocated added file is asserted from this path audit.** The large UI components under `src/presentation/services/` remain architectural review candidates because they contain QWidget/editor logic alongside services; moving them without reviewing all their dependent code would make compatibility worse.

A confirmed Community code-quality inconsistency was fixed: `editionActionToolAvailable` had unreachable branches that returned availability for action IDs already blocked earlier in the same function (barcode, document, table, LaTeX, HTML). These branches were removed **without changing their disabled behavior**, and a Mini toolbar-import regression now explicitly checks that those original-author action variants remain unavailable. Do not confuse this UI action suppression with disabling independent offline QR/local OCR.

### High — Release scope and optional features

New upstream screenshot follow-up/export task configuration, translation layout, pinned-history operations, and compression/clipboard behavior are product changes, not merely maintenance. Check their default settings and interactions with Community offline OCR, direct Cloud OCR, history, permissions and update behavior. PR #2 already introduces an optional user-supplied Vision engine beyond the initial PaddleOCR-only scope; explicitly approve it or separate it. PR #2 changes `capture_history/enabled` and `pinned_history/enabled` defaults to `false`; confirm new-install and migrated-profile expectations.

### High — GitHub workflows, source boundaries and deletion protection

The original PR #2 and PR #3 had conflicting sync workflow versions and tests. The reconciled candidate must preserve PR #3's real Git conflict candidates, immutable review branches, token-permission fallback and upstream-ancestry test **and** PR #2's pinned Actions, upstream identity loaded from policy, deleted-file and forbidden-runtime scans. The removed Homebrew workflow is explicitly denied by `.github/upstream-protection.json`. Validate the actual **combined** tree, not prior independent green runs.

### Medium — Documentation placement, references and i18n

Current Community README and `docs/community-*.md` relative references were inspected; no missing relative link was observed in the checked set. This is a path-integrity result only. Audit statements against shipped feature flags, Windows-only release scope, manual updater and OCR provider behavior. Keep official platform build docs at their established root paths. C++ user-facing additions must be registered in `snow_shot/i18n/modules.json` and the corresponding English, Simplified Chinese and Traditional Chinese catalogs; no unfinished messages or inadvertent paid-service UI labels.

## Release acceptance

1. Selectively reconcile Draft PR #5 into PR #2; record **accepted, deferred and rejected** upstream modules/paths and preserve upstream Git ancestry through a real merge after resolution.
2. Run `python3 scripts/test-upstream-sync.py`, `python3 scripts/upstream_sync.py verify-deletions`, Qt/CMake/source registration checks and relevant targeted native tests on the actual merged tree.
3. Run a clean Windows x64 production build and online/offline NSIS installer verification, hash/manifest checks, local PP-OCRv6 and PaddleOCR integration tests; manually test 1.5.7 upgrade and persisted settings.
4. Do not merge to Community `main` or publish a beta until these gates pass and the source/feature decisions are approved.

The review does **not** conclude that new upstream code is malicious or that every highlighted location is wrong. It distinguishes proven mismatches from architecture decisions and unverified integration risk.
