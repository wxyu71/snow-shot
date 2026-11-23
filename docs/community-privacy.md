# Snow Shot Community — Privacy

This independent open-source fork has no Snow Shot account, membership, credits,
recharge or payment service. It does not contain analytics or telemetry upload
code. Diagnostics and crash capture are local-only; Crashpad uploads are
explicitly disabled.

Screenshots, capture history, pinned-image history and optional offline PP-OCRv6
recognition stay on the user's computer. On a fresh installation, **both local
histories are enabled by default**, while startup registration remains disabled.
Users may disable histories or change retention limits; previously saved choices
are respected. Local histories are not uploaded, but someone with access to the
computer or its backups may be able to read them.

**Online OCR is optional, PaddleOCR-only and explicitly user-initiated.**

- **Baidu PaddleOCR (AI Studio):** selecting Online OCR sends the selected image
  directly to the user-configured AI Studio HTTPS endpoint with an Access Token.
  Supported models are PP-OCRv6, PP-StructureV3 and PaddleOCR-VL-1.6.
  Output is Markdown with formulas preserved as LaTeX. Result downloads
  never carry the Access Token.
- Obsolete Vision/HTML/LaTeX Online OCR preferences from older builds are
  ignored, not treated as permission to contact another provider.
- Separate user-configured cloud upload actions require an explicit user
  action and a configured destination. Merely capturing an image or retaining
  local histories never invokes a cloud upload.

No original-author account or hosted service endpoint is configured by default.
Provider terms and retention practices apply only when the user intentionally
submits an image to PaddleOCR or another independently configured cloud action.

On Windows, the PaddleOCR Access Token is stored with user-scoped DPAPI
protection. Custom AI / translation API keys and S3 access credentials are also
DPAPI-protected at the `config.json` disk boundary; legacy plaintext Windows
profiles are rewritten to the protected form after loading. Configuration
archives remain credential-redacted by default. On other platforms these
credentials are currently stored in user settings/configuration and should be
treated as sensitive. Never attach settings files or credentials to bug reports.

Community update/release links point to this fork's public GitHub/Gitee release
infrastructure. Required upstream copyright and license notices are retained;
they are attribution, not service endpoints.
