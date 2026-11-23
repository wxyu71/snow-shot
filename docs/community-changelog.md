# Snow Shot Community — Beta changes

## v1.5.8-beta.1

- Remove account/login/payment UI; no Snow Shot account is needed.
- Use one user-triggered PaddleOCR-only Online OCR action; configure AI Studio credentials and model and receive Markdown with formulas as LaTeX.
- Enable local screenshot and pinned-image histories by default; no automatic cloud upload.
- Default Windows updating to manual GitHub Releases installs; no separate trust infrastructure for this personal testing prerelease.
- Build and distribute one Community Windows x64 edition, without an extra Mini installer.
- Remove analytics/telemetry upload paths and upstream hosted-service defaults; keep diagnostics local-only.
- Keep local PP-OCRv6, screenshots, pinning and shortcuts.
- Add guarded upstream sync and a Windows x64 beta build pipeline.
- Replace unrelated original website, QQ support and purchase links.

This is an independent test fork, **not an official Snow Shot release**.
Source originally forked from [mg-chao/snow-shot](https://github.com/mg-chao/snow-shot).
Required original copyright, GPL and third-party license notices are retained.

Before publication the testing beta must pass a real Windows production build, NSIS packaging and automated installer smoke. Local-model OCR and an installed upgrade from 1.5.7 remain hands-on acceptance tests for the personal prerelease.
