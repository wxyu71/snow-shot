# Snow Shot Community prerelease

Release version: **@VERSION@**

Windows x64 full edition. Select the online or offline installer; the accompanying SHA-256 files and manifests describe the verified release assets. The two transaction update ZIPs and RSA-PSS signed latest-version.json are for the application updater, not portable editions.

## Changes in Beta 2

- Online PaddleOCR: automatic model selection or manual selection, with quota-aware fallback between compatible models and bounded retries for a full provider queue.
- Markdown, plain text, JSONL and locally converted HTML output choices, depending on the model.
- Online screenshot recognition continues after the capture window closes; completion uses in-app feedback and preserves newer clipboard publications.
- Simplified and Traditional Chinese OCR settings, shared Ant Design Qt editors, and a direct link to the official AI Studio token page.

## Upgrade

Close the running application before upgrading and back up important settings and history. Download only from this repository. Automatic unsigned executable updates are disabled. Beta 1 users must first migrate using the matching Inno installer; subsequent updates use the signed transaction feed. Never install an update that cannot be verified.

This personal fork preserves local capture, editing, history and independent PaddleOCR configuration. Upstream source and Community changes are recorded in Git history.
