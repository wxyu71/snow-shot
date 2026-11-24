# Community privacy and updates

This is a personal fork, not the upstream official account or paid-service distribution. Capture, pinned history and settings are stored locally. Startup, administrator launch and MCP are opt-in; local history is enabled by default and respects explicit user choices.

Online OCR sends selected content to the PaddleOCR endpoint configured by the user. Use a trusted endpoint; its own privacy and retention policy applies. Do not commit credentials or personal capture data.

Updates are obtained from the fixed [Community GitHub Releases](https://github.com/wxyu71/snow-shot/releases) origin. The unsigned automatic updater remains disabled: download the prerelease installer and verify its SHA-256 before installation. HTTPS and checksums help detect corruption but do not independently authenticate a compromised release account.

## User-owned WebDAV settings sync (development)

Community will use an opt-in, user-configured HTTPS WebDAV endpoint instead of the upstream signed-in sync backend. The initial sync core accepts only an explicit set of non-sensitive scalar preferences. It rejects foreign schema versions and unknown fields; capture/history data, local paths, credentials, OCR endpoints, recording-device and privilege settings are excluded. First synchronization with divergent values or concurrent edits to the same key requires explicit conflict resolution. The WebDAV file is JSON, not an encrypted backup; access is controlled by the user's server and HTTPS. Writes require strong ETags (If-Match) or create-only If-None-Match; servers without those semantics cannot be used safely. No automatic overwrite or silent fallback to an official service.

The draft core is not yet an enabled user feature. The remaining integration must add local-only connection configuration and Windows DPAPI-backed credentials, Settings UI, persistent per-endpoint merge baseline, revision-guarded application of schema-validated changes, retry/confirmation behavior and integration tests against a real WebDAV server. Re-read and verify committed content before advancing the local baseline. Do not enable background sync until the integration passes those tests.
