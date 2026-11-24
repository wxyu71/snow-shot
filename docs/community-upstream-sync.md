# Upstream synchronization

The scheduled workflow fetches official upstream and maintains at most one rolling Community review PR. It merges actual Git history rather than copying files or creating report-only branches.

- A clean integration is committed to the review branch; an unresolved merge is reported without pretending it succeeded.
- Review protected Community paths, runtime behavior, OCR and update origins before marking the PR ready.
- Ready PRs are frozen for acceptance. Merge only the tested head, preserving merge ancestry.
- The sync job never pushes to main. Automatic merging is not the default.
- GitHub must permit Actions to create PRs, or a maintainer must open the reported review branch manually. A separately authorized GitHub App can supply permissions where needed.

After merging, delete integrated review branches. Subsequent synchronization starts from main and its actual upstream ancestor.

See [Windows release policy](community-windows.md) for production acceptance.

Keep the Community documentation index concise. Do not reintroduce retired release guides, historical reports or duplicate changelogs during synchronization; retain required build inputs, protocol references and legal notices.
