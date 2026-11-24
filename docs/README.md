# Snow Shot Community documentation

Community is a personal Windows x64 full-edition distribution. Other platform source and build infrastructure remain for upstream compatibility, not Community releases.

- [Build, testing and release](community-windows.md): local development, CI acceptance, installer verification and publication.
- [Upstream synchronization](community-upstream-sync.md): review and merge policy.
- [Privacy and updates](community-privacy.md): local data, direct OCR and update trust.
- [Repository conventions](../AGENTS.md): coding, translation, testing and licensing.
- [Published release history](https://github.com/wxyu71/snow-shot/releases): the only release changelog. Unpublished changes remain in Git and pull requests.

## Architecture

The fork retains the upstream Qt/C++ UI, Rust components, OCR runtime and platform abstractions. Community-specific boundaries are direct PaddleOCR configuration, local privacy defaults, disabled account/paid features, independent update origin and Windows x64 distribution.

Component API/protocol documentation, fixtures, reproducible benchmarks and legal notices remain beside their code. Retired upstream platform runbooks and historical measurement reports remain available in Git history; the upstream project owns their maintenance.

The release-notes template is a build input, not a separate changelog.
