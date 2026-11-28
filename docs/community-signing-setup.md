# Community update signing: maintainer setup

The Community updater uses a pinned RSA-3072 public key (exponent 65537) to verify signed update feeds. This is independent of Windows Authenticode and does not remove SmartScreen warnings.

Generate the production key on a trusted maintainer machine, outside the checkout. Never send the private key to a chat, commit it, upload it as an artifact, or print it in a workflow log. Keep an encrypted offline backup. Do not use CI's ephemeral self-test keys in production.

Example (PowerShell, from the repository root, with OpenSSL and Python 3.12+ on PATH):

```powershell
$keyDir = Join-Path $env:USERPROFILE ".snow-shot-signing"
New-Item -ItemType Directory -Path $keyDir -Force | Out-Null
$key = Join-Path $keyDir "community-release-v1.pem"
$pub = Join-Path $keyDir "community-release-v1-public.pem"
if ((Test-Path $key) -or (Test-Path $pub)) { throw "Key already exists; do not overwrite" }
openssl genpkey -algorithm RSA -pkeyopt rsa_keygen_bits:3072 -pkeyopt rsa_keygen_pubexp:65537 -aes-256-cbc -out $key
if ($LASTEXITCODE -ne 0) { throw "Key generation failed" }
openssl pkey -in $key -pubout -out $pub
if ($LASTEXITCODE -ne 0) { throw "Public key extraction failed" }
python scripts/community_installer_signing.py --export-public-key $pub --key-id community-release-v1
```

Choose a strong passphrase when prompted. Restrict the key directory's access to the maintainer and back up the private PEM and passphrase separately. Only the exported public JSON is safe to share for independent review and pinning in `snow_shot/resources/update-trusted-keys.json`. A public-key identifier must remain stable across releases. Do not replace or remove an already deployed trusted key without a reviewed rotation strategy.

The encrypted private key stays on the maintainer machine. The maintainer can sign the verified production artifacts locally once the release pipeline is connected; the public JSON alone does not enable publication. Automated signing would require a separately approved secret-management and release-gating design. This setup does not remove any existing release gate.
