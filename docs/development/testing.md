# Testing

The Community Windows workflow runs static analysis and policy/contract checks, native C++ smoke and focused Windows/MSVC tests before the expensive production stage. Draft PRs use the fast gates; ready code PRs and relevant main pushes run full Windows acceptance.

Runtime invariants belong in executable tests, not source-text assertions. Use CTest labels/presets and fail when an expected test is missing. Preserve upstream fixtures unless a reviewed behavior change requires migration. The release stage verifies packages and exercises actual installer installation and upgrade.

See [build and release](../community-windows.md).
