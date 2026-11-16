# Upstream sync review pending — 2026-11-16

> **Status: merge blocked; no upstream source has been applied.**
> This file is the conflict/report-only candidate required by the fork's
> fail-closed [upstream sync policy](../docs/community-upstream-sync.md).
> Do not merge this PR as an upstream code update. Resolve conflicts on this
> branch, apply and validate upstream changes, and remove this file first.

## Compared refs

| Ref | SHA |
| --- | --- |
| Fork `wxyu71/snow-shot:main` (base for this PR) | `5f3bd76282fc660968266bd957734b8b2fa05a06` |
| Upstream `mg-chao/snow-shot:main` | `4cc4dfe9824abc85bbb9e629ccea08de0bb94ae3` |
| Shared merge base | `4b075c1977529558f6036fb56ad37e12d9f706c4` |

The refs diverged: 13 upstream-only commits and 74 fork-only commits.
Comparing each branch against the shared merge base shows **181 upstream-changed
files**, **37 paths also changed by the fork**, and **20 upstream paths
matching protected/deleted rules** in `.github/upstream-protection.json`.

Compare links:
- [Upstream since merge base](https://github.com/mg-chao/snow-shot/compare/4b075c1977529558f6036fb56ad37e12d9f706c4...4cc4dfe9824abc85bbb9e629ccea08de0bb94ae3)
- [Community fork since merge base](https://github.com/wxyu71/snow-shot/compare/4b075c1977529558f6036fb56ad37e12d9f706c4...5f3bd76282fc660968266bd957734b8b2fa05a06)

## Blocking conflict and review candidates

**Definite Git modify/delete conflict:** upstream modifies
`.github/workflows/snow-shot-homebrew-checks.yml` while the community fork
deleted it. The fork intentionally removed official Homebrew/release
automation; retain the deletion unless the independent community distribution
policy explicitly approves a replacement.

The compare patches additionally show overlapping changed base-line ranges
for these paths. These are **potential content conflicts** (diff-hunk
screening, not a completed `git merge` or a verified list of conflict markers):

- `CMakeLists.txt`, `README.md`, `README.zh-CN.md`, `snow_shot/CMakeLists.txt`
- `snow_shot/include/snow_shot/presentation/screenshotimageconversioncontroller.h`
- `snow_shot/include/snow_shot/presentation/screenshottoolbarlayoutmodel.h`
- `snow_shot/src/network/snowshotapiclient.cpp`
- `snow_shot/src/presentation/ocr/screenshotimageconversioncontroller.cpp`
- `snow_shot/src/presentation/ocr/screenshotrecognitionsessioncontroller.cpp`
- `snow_shot/src/presentation/tools/screenshottoolpalette.cpp`
- `snow_shot/tests/image_conversion_tests.cpp`
- `snow_shot/tests/screenshot_recognition_session_controller_tests.cpp`
- `snow_shot/tests/snowshot_api_client_tests.cpp`

Other overlapping changes (e.g. localization, edition feature flags, OCR
controllers, settings and application lifecycle) also need semantic review
even when their diff hunks do not overlap.

## Upstream features worth integrating

The 13 incoming commits include: rich document recognition/export, collage
editor handoff, vision model reasoning-display improvements, serial-number
shift-drag arrows, fullscreen input suppression controls, screenshot guide
color adaptation, notification/shortcut fixes, macOS release signing changes,
and an upstream `1.5.8` version bump.

Keep the community `1.5.8-beta.1` branding/version rules, independent updater,
direct PaddleOCR client, removed account/membership/billing flows, and
Windows x64 community release configuration unless deliberately reviewed.
In particular, avoid reintroducing `AccountSessionService`,
`AccountPageWidget`, account credentials, billing, or official
release/update endpoints.

## Resolution procedure (maintainer action required)

1. Re-fetch both heads and ensure this PR's branch is rebased/updated from the
   current `origin/main`; redo comparisons if either branch moved.
2. On a machine with the repository and full Git history:
   ```sh
   git remote add upstream https://github.com/mg-chao/snow-shot.git  # only if absent
   git fetch origin main
   git fetch upstream main
   git switch -c integrate/upstream-4cc4dfe origin/main
   python3 scripts/test-upstream-sync.py
   python3 scripts/upstream_sync.py classify --upstream upstream/main
   git merge --no-ff --no-commit upstream/main
   # Resolve modify/delete and content conflicts; audit all protected paths.
   # Complete the merge only after verification succeeds.
   ```
3. Preserve deleted commercial modules and community distribution boundaries.
   Run `python3 scripts/upstream_sync.py verify-deletions`,
   `python3 scripts/test-upstream-sync.py`, other community policy tests,
   and the full native Qt/Windows build and CTest CI.
4. Review upstream functional changes against the fork's OCR/editor features,
   remove **this pending report**, and push the verified integrated merge
   candidate to this PR branch. Only then mark this draft ready for review.

**Verification:** GitHub ref/commit/file comparisons and fork policy inspected.
No local Git merge, compile, CTest, or release validation was executed.
