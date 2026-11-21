# Snow Shot Community — Privacy

This independent open-source fork has no Snow Shot account, membership, credits,
recharge or payment service. It does not contain analytics or telemetry upload
code. Diagnostics and crash capture are local-only; Crashpad uploads are
explicitly disabled.

Screenshots and the optional offline PP-OCRv6 engine run on the local computer
without remote OCR requests. Fresh Community profiles also keep automatic startup,
capture history, and pinned-image history disabled until the user explicitly
enables them. Existing saved preferences are preserved.

**Online OCR is optional and provider-controlled in Settings.**

- **Baidu PaddleOCR (AI Studio):** the selected screenshot is uploaded directly
  to the user-configured HTTPS AI Studio endpoint with the user's Access Token.
  Structured Markdown is returned, with formulas preserved as LaTeX. Signed
  result downloads do not carry the Access Token.
- **OpenAI-compatible vision model:** the inherited large-model OCR pipeline can
  send the selected screenshot directly to a vision-capable endpoint configured
  by the user under Connections & services. Output can be Markdown, HTML or
  LaTeX.

No original-author Snow Shot account or hosted API endpoint is configured by
default. Network requests made by Online OCR therefore go only to a provider the
user explicitly configures or selects. Provider terms and data-retention
practices apply to those requests.

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
