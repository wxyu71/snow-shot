# Community fork: upstream synchronization

The official upstream is `mg-chao/snow-shot:main`. The scheduled and manually
dispatched workflow is `.github/workflows/upstream-sync.yml`. The policy is
`.github/upstream-protection.json` and `scripts/upstream_sync.py`.

## Safety invariants

1. **Community `main` is never force-pushed.** Only documentation-only changes
   that pass the fail-closed policy and deletion guard can be auto-merged.
   All executable code, release/build metadata and workflow changes need review.
2. **An upstream review PR is a real code review, not a report-only PR.**
   With a clean Git merge, its branch contains a two-parent merge commit.
   With merge conflicts, its head starts at the actual upstream commit;
   GitHub detects the conflict against Community `main`. It must be reconciled
   before merging, preserving both sides' ancestry and Community behavior.
3. **Human work wins.** If an upstream review PR is already open, scheduled
   syncs only write a warning and job summary with the latest upstream SHA.
   They never mutate its head or create duplicate review PRs.
4. **No force-pushes on review branches.** New branch names contain the upstream
   SHA and the target `main` SHA. A retry reuses an existing branch unchanged.
   A Git problem, denied push or unexpected API error still fails the job.
5. **Review is not a broken build.** Pending manual resolution is reported
   as a warning and in the job summary. Genuine failures remain red.

## Workflow states

| Condition | Behavior | Outcome |
| --- | --- | --- |
| No incoming changes | Exit without pushing | Success, no PR |
| Only policy-allowed documentation, clean merge | Run guards, merge and push to `main` | Success |
| Changes requiring review, an upstream PR is already open | Preserve its branch; summarize upstream SHA and PR link | Success with warning |
| Review required, merge is clean, no prior PR | Push a real merge commit to a new immutable review branch | Draft PR |
| Review required, merge conflicts, no prior PR | Push the actual upstream head to a new immutable review branch | Draft PR showing real conflicts |
| Previously pushed review branch, PR creation interrupted | Reuse branch, retry opening the PR | Draft PR, no force push |
| Unexpected push, Git or API problem | Do not bypass the error | Failed |

**Important:** A conflict PR based on upstream is *not* safe to merge until
its conflicts are resolved and protected Community behavior is verified.
Do not interpret a green sync workflow as an approval of the pending PR.

## Resolve a conflicting upstream PR

The draft PR tells you the upstream SHA, fork baseline SHA, policy reasons and
detected conflicted files. You may use GitHub's Resolve conflicts UI for
small conflicts; for complex code, resolve locally:

```sh
gh pr checkout <PR_NUMBER>
git fetch origin main
git merge origin/main
# Resolve conflicts; preserve Community OCR, edition, updater and release policy.
git add <resolved-files>
git commit
python3 scripts/test-upstream-sync.py
python3 scripts/upstream_sync.py verify-deletions
git push
```

For a clean review candidate, `git merge origin/main` may report "Already up to
date"; that is expected. The policy and normal Community PR CI must still pass,
and a maintainer must review executable changes before merging.

The fork intentionally removed commercial account, membership and payment
code. If upstream adds additional commercial modules, update `deleted_files`.
Do not weaken the protection configuration merely to make synchronization pass.

## Authentication and CI caveat

Repository **Settings → Actions → General → Workflow permissions** must allow
GitHub Actions to create and approve pull requests; the workflow additionally
uses `contents: write` and `pull-requests: write`. GitHub's default
`GITHUB_TOKEN` can create a PR with this setting, but according to GitHub's
current documentation, `pull_request` CI created by that token can require a
maintainer to explicitly approve the run. If fully unattended PR CI is needed,
prefer a narrowly scoped GitHub App installation token (and protect its keys);
avoid granting a broad personal token by default.

A branch protection rule may still deny an automated push. The sync workflow
must not bypass it.

## Local policy and regression tests

```sh
python3 scripts/test-upstream-sync.py
python3 scripts/upstream_sync.py verify-deletions
git remote add upstream https://github.com/mg-chao/snow-shot.git
git fetch upstream main
python3 scripts/upstream_sync.py classify --upstream upstream/main
```

The regression suite builds local Git fixtures for clean merges and true
conflicts, checks that the review branch carries real upstream history, and
checks that an already-open PR prevents duplicate creation or branch overwrite.
The scheduled workflow runs from **main**, not from an unmerged workflow PR.
