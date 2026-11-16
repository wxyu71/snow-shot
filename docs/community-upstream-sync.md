# Community fork: upstream synchronization

The official upstream is `mg-chao/snow-shot` (`main`).
`.github/workflows/upstream-sync.yml` checks upstream daily or on demand.

A change is auto-merged **only** when the incoming paths do not overlap the
fork's changes since the shared merge base, do not touch protected or deleted
files, and do not contain newly added known commercial account/payment code.
The available sync runner currently runs the policy and source-contract tests,
not the full Qt/Windows test suite. Therefore, it **auto-merges documentation-only
changes** when those checks pass; executable code, workflows, resource manifests,
build files, and installers are always routed to a draft review PR until native
regression tests can run as a mandatory gate. This conservative restriction
prevents silently merging untested binaries or reintroducing commercial code.
After a clean documentation merge, the deletion guard and policy tests run again.
Any conflict or failed guard aborts the merge.

Protected changes produce a **draft review PR containing a pending report**,
not a speculative merge that may overwrite community-specific functionality.
Maintainers must review and manually apply the relevant upstream changes.
The workflow never uses `--force` to push to `main`.

The protected file and deletion policy lives in `.github/upstream-protection.json`.
When removing new commercial modules, add each path to `deleted_files`.
Do not weaken protection to make an upstream sync pass.

## Local checks

```sh
python3 scripts/test-upstream-sync.py
python3 scripts/upstream_sync.py verify-deletions
git remote add upstream https://github.com/mg-chao/snow-shot.git
git fetch upstream main
python3 scripts/upstream_sync.py classify --upstream upstream/main
```

Required repository settings: allow GitHub Actions to create PRs and write
contents, and permit the workflow's non-force merge push if auto-sync is desired.
A protected branch that blocks the action's push will cause the job to fail,
rather than bypass protection.
