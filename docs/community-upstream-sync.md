# Community upstream integration (solo-maintainer workflow)

Official upstream: `mg-chao/snow-shot`, source of truth: `.github/upstream-protection.json`.
The scheduled job never pushes `main`, never force-pushes review branches, and never
creates a report-only PR. All updates are proposed as real Git history in one
rolling integration PR per batch. No upstream release is a Community release.

## Migration from existing PRs

The existing **Draft PR #2** is a large, manually resolved upstream/Beta
integration. It is *not* disposable. The new automation refuses to create a
parallel sync PR while a `review/upstream-main-*` PR against `main` is open.
First complete, test, and merge PR #2, or explicitly close it after preserving
its work elsewhere. PR #7 is a legacy report-only PR; close it once the new
mechanism is enabled. Do not merge PR #7's report file.

## Normal behavior

1. Every day (or on `workflow_dispatch`) fetch `origin/main` and official
   `upstream/main`. If the official tip is already in `main`, exit.
2. Check for an open `automation/upstream-sync-*` PR. Only **one** is allowed.
   A Ready PR is frozen; an open Draft PR can be updated in place.
3. If there is no PR, try a real, two-parent Git merge of upstream into current
   Community `main`. Create a PR with a branch `automation/upstream-sync-<baseSHA>`.
4. A clean but policy-sensitive merge is a Draft PR. A clean low-risk merge
   is also Draft by default (solo-maintainer safety default).
5. If opted in using `UPSTREAM_SYNC_AUTO_MERGE=true`, a clean low-risk new PR
   is created Ready and GitHub's `--auto --merge` is requested. This is only
   safe after configuring branch protection, required Windows checks, and a
   token that actually triggers those checks. Never enable it just because Git
   reports no textual conflicts.
6. Existing Draft PRs are updated by merging latest `main`, then latest upstream,
   onto the **existing** branch history. Non-fast-forward updates are rejected.
   Existing manual commits are retained. If either merge conflicts, the remote
   branch is untouched and the managed PR description reports the blocker.
7. Overlap with other open feature PRs is advisory. It does not merge their
   unreviewed work into the integration candidate.
8. Mark the PR **Ready for review** to freeze automated changes. Merge only
   after the actual candidate is validated; merge commits preserve upstream
   ancestry (do not squash an upstream sync PR).

## When the first merge conflicts

Git cannot push an unresolved merge index. The branch begins at the **actual
upstream commit** (not an empty/report commit), and the Draft PR's GitHub
conflict UI indicates that it is not yet a Community-integrated candidate.
The mandatory ancestry gate refuses to consider it safe until the initial
Community `main` commit is a Git ancestor of the PR branch.

On a local machine, using the PR's exact branch name:

```bash
git remote add upstream https://github.com/mg-chao/snow-shot.git  # only if absent
git fetch origin main automation/upstream-sync-<baseSHA>
git fetch upstream main
git switch -c resolve-upstream --track origin/automation/upstream-sync-<baseSHA>
git merge --no-ff origin/main
# Resolve all Git conflicts while retaining Community identity, OCR, removals,
# updater origins, release settings, license notices and build/test registration.
git add -A
git commit
python3 scripts/test-upstream-sync.py
python3 scripts/upstream_sync.py verify-deletions
# Run Community Windows x64 CI / CTest; check Git history before Ready.
git push origin HEAD:automation/upstream-sync-<baseSHA>
```

If later upstream updates conflict, fetch the *same* PR branch and merge
`upstream/main` into it. Resolve and push; the bot never discards earlier
manual resolutions. The PR body has a versioned marker, and text outside the
managed section remains untouched by the bot.

## Required setup

- Enable **Allow GitHub Actions to create and approve pull requests** under
  Settings → Actions → General, otherwise the branch may be pushed but PR
  creation will fail. No report-only fallback is performed.
- Prefer a tightly-scoped GitHub App with Contents, Pull requests and
  Workflows write permissions. Store its `UPSTREAM_SYNC_APP_ID` and
  `UPSTREAM_SYNC_APP_PRIVATE_KEY` as repository secrets; the workflow mints
  a short-lived installation token on each run. Alternatively, set an
  appropriately scoped fine-grained PAT as `UPSTREAM_SYNC_TOKEN`. Without
  either, the workflow falls back to `GITHUB_TOKEN`; GitHub may reject pushes of
  upstream `.github/workflows` files, and its PR/push events may not run
  the downstream PR CI. The workflow fails explicitly rather than claiming
  that the code was integrated.
- Protect `main` through a ruleset requiring PRs and successful Community
  preflight/Windows tests. Do **not** require another reviewer's approval in
  a single-maintainer repository. Allow merge commits; use merge commits for
  sync PRs, not squash. Require the sync PR ancestry check to pass.
- **Do not** turn on `UPSTREAM_SYNC_AUTO_MERGE` until the check requirements
  are working, GitHub's repository auto-merge setting is enabled, and
  a complete Windows acceptance gate covers your supported distribution.
  Repo auto-merge is currently disabled, so low-risk PRs stay Draft by default.
- If you also maintain other feature PRs, their overlap is only a warning;
  they should remain based on Community `main` and not on the sync PR.

## Key invariants

- Incoming official code is never represented by a fake report-only commit.
- A candidate must contain both the declared upstream commit and the
  original Community baseline in Git ancestry, as well as current `main`.
- No banned/deleted Community commercial file is restored.
- Changing files on both sides is **not** synonymous with a Git textual conflict.
- Source-only non-overlapping changes can be classified as low-risk but must
  still pass CI; updated workflows, Community protected code and account/payment
  surface require manual review.
- The bot never overwrites a branch with `--force`; a race fails safe.
- Risk review status stays sticky within a batch; a subsequently clean update
  doesn't retroactively declare an earlier risky sync safe.

The sync script can be tested without GitHub credentials:

```bash
python3 scripts/test-upstream-sync.py
python3 scripts/upstream_sync.py verify-deletions
```
