# Snow Shot Community — Privacy

This independent open-source fork has no Snow Shot account, membership, credits,
recharge or payment service. Screenshots and the optional offline PP-OCRv6
engine run on the local PC without remote OCR requests.

**Cloud OCR is optional.** With your own Baidu AI Studio Access Token, the
selected screenshot is uploaded **directly to the configured AI Studio API**.
Result JSONL or Markdown is downloaded from the signed URL returned by the
service. Baidu's terms and data-retention practices apply to that processing.

Currently the API URL, model and token are stored under your user's Qt
settings. **The Access Token is not encrypted at rest.** Protect your profile,
use a scoped token, and never attach your settings file to bug reports.

Optional inherited features may have separate network interactions. Review
their settings before enabling them. This fork is not affiliated with or
supported by the original author.
