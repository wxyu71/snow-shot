[CmdletBinding()]
param(
    [Parameter(Mandatory)][string]$SigningKeyPath,
    [string]$BuildDirectory = 'build/snow-shot-msvc-release'
)
$ErrorActionPreference = 'Stop'
$root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$build = if ([IO.Path]::IsPathRooted($BuildDirectory)) {
    [IO.Path]::GetFullPath($BuildDirectory)
} else { [IO.Path]::GetFullPath((Join-Path $root $BuildDirectory)) }
if (-not (Test-Path -LiteralPath $SigningKeyPath -PathType Leaf)) {
    throw 'Community update signing key was not provided.'
}
$version = [regex]::Match(
    (Get-Content -LiteralPath (Join-Path $root 'CMakeLists.txt') -Raw),
    '(?m)^set\(SNOW_SHOT_VERSION\s+"(?<version>[^"]+)"\)').Groups['version'].Value
if ($version -cnotmatch '^\d+\.\d+\.\d+-beta\.\d+$') {
    throw "Expected a prerelease version, got '$version'."
}
$items = @()
foreach ($variant in @('online', 'offline')) {
    $base = "snow-shot-$version-windows-x64-$variant"
    foreach ($kind in @('installer', 'update')) {
        $suffix = if ($kind -eq 'installer') { '.exe' } else { '-update.zip' }
        $name = "$base$suffix"
        $file = Join-Path $build $name
        $manifestName = if ($kind -eq 'installer') { "$base.manifest.json" }
                        else { "$base-update.manifest.json" }
        $manifest = Get-Content -LiteralPath (Join-Path $build $manifestName) -Raw |
            ConvertFrom-Json
        if ($manifest.PackageVersion -cne $version -or
            $manifest.Variant -cne $variant -or
            $manifest.Architecture -cne 'x64') {
            throw "Mixed package versions or architecture: $manifestName"
        }
        $descriptor = if ($kind -eq 'installer') { $manifest.Installer }
                      else { $manifest.Archive }
        $bytes = (Get-Item -LiteralPath $file).Length
        $hash = (Get-FileHash -LiteralPath $file -Algorithm SHA256).Hash.ToLowerInvariant()
        if ($bytes -ne $descriptor.Bytes -or $hash -cne $descriptor.Sha256) {
            throw "Native package does not match its audit manifest: $name"
        }
        $sidecar = ((Get-Content -LiteralPath "$file.sha256" -Raw).Trim() -split '\s+')[0]
        if ($sidecar -cne $hash) { throw "SHA256 sidecar mismatch: $name" }
        $path = if ($kind -eq 'installer') { "setup/snow-shot_windows-x64-$variant.exe" }
                else { "setup/snow-shot_windows-x64-$variant-update.zip" }
        $item = [ordered]@{
            variant = $variant; kind = $kind; path = $path
            size = $bytes; sha256 = $hash
        }
        if ($kind -eq 'update') {
            $item.files = @($manifest.InstallFiles | ForEach-Object {
                [ordered]@{
                    path = $_.Path.Replace('\', '/')
                    size = $_.Bytes
                    sha256 = $_.Sha256
                }
            })
        }
        $items += $item
    }
}
$payload = [Text.Encoding]::UTF8.GetBytes((
    [ordered]@{
        schema = 1; product = 'snow-shot'; version = $version
        publishedAt = [DateTime]::UtcNow.ToString('yyyy-MM-ddTHH:mm:ssZ')
        platform = 'windows-x64'; packages = $items
    } | ConvertTo-Json -Compress -Depth 12))
$keysFile = Join-Path $root 'snow_shot/resources/update-trusted-keys.json'
$keys = (Get-Content -LiteralPath $keysFile -Raw | ConvertFrom-Json).keys
$rsa = [Security.Cryptography.RSA]::Create()
try {
    $rsa.ImportFromPem((Get-Content -LiteralPath $SigningKeyPath -Raw))
    if ($rsa.KeySize -ne 3072) { throw 'Community signing requires a 3072-bit RSA key.' }
    $public = $rsa.ExportParameters($false)
    $modulus = [Convert]::ToBase64String($public.Modulus)
    $exponent = [Convert]::ToBase64String($public.Exponent)
    $found = @($keys | Where-Object {
        $_.id -like 'snow-shot-community-*' -and
        $_.modulus -ceq $modulus -and $_.exponent -ceq $exponent
    })
    if ($found.Count -ne 1) {
        throw 'Signing key is not a pinned community update public key.'
    }
    $signature = $rsa.SignData(
        $payload, [Security.Cryptography.HashAlgorithmName]::SHA256,
        [Security.Cryptography.RSASignaturePadding]::Pss)
    if (-not $rsa.VerifyData($payload, $signature,
        [Security.Cryptography.HashAlgorithmName]::SHA256,
        [Security.Cryptography.RSASignaturePadding]::Pss)) {
        throw 'Community update signature self-verification failed.'
    }
    $envelope = [ordered]@{
        schema = 1; keyId = $found[0].id
        payload = [Convert]::ToBase64String($payload)
        signature = [Convert]::ToBase64String($signature)
    }
    $out = Join-Path $build 'latest-version.json'
    [IO.File]::WriteAllText($out,
        ($envelope | ConvertTo-Json -Compress -Depth 6) + [Environment]::NewLine,
        [Text.UTF8Encoding]::new($false))
} finally {
    $rsa.Dispose()
}
$auditor = Join-Path $build 'snow_shot/Release/snow-shot-updater.exe'
if (-not (Test-Path -LiteralPath $auditor)) {
    throw 'The native update verifier was not compiled.'
}
& $auditor --verify-release --platform windows-x64 --manifest (Join-Path $build 'latest-version.json')
if ($LASTEXITCODE -ne 0) {
    throw 'The compiled updater refused the signed community feed.'
}
Write-Host "Verified signed Community Full-only update feed for $version."

