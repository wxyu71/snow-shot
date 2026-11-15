[CmdletBinding()]
param()

$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot 'snow-ocr-toolchain.ps1')
$pinPath = Join-Path $PSScriptRoot '../snow-crates/crates/simd-paddle-ocr-rs/rust-toolchain.toml'
$match = [regex]::Match((Get-Content -LiteralPath $pinPath -Raw),
    '(?m)^channel\s*=\s*"(nightly-\d{4}-\d{2}-\d{2})"')
if (-not $match.Success) { throw 'Pinned OCR Rust toolchain source is invalid.' }
$expected = $match.Groups[1].Value

# No prior successful -match: regression for -cnotmatch followed by $Matches[1].
Remove-Variable Matches -Scope Script -ErrorAction SilentlyContinue
$actual = Get-SnowOcrToolchain
if ($actual -cne $expected) {
    throw "OCR toolchain must not rely on an inherited Matches variable: '$actual' != '$expected'."
}

# Even a misleading capture left by an unrelated caller must not alter the pin.
$Matches = @{ 1 = 'nightly-1900-01-01' }
$actual = Get-SnowOcrToolchain
if ($actual -cne $expected) {
    throw "OCR toolchain used stale Matches capture: '$actual' != '$expected'."
}
Write-Host "Pinned OCR nightly toolchain returns stable value: $actual"
