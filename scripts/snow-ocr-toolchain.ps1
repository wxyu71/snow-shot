function Get-SnowOcrToolchain {
    $pin = Join-Path $PSScriptRoot '../snow-crates/crates/simd-paddle-ocr-rs/rust-toolchain.toml'
    if (-not (Test-Path -LiteralPath $pin)) { throw 'Initialize Git submodules before building OCR.' }
    $source = Get-Content -LiteralPath $pin -Raw
    # -cnotmatch does not guarantee a fresh $Matches capture when false.
    # Use an explicit Match object so a clean PowerShell process never returns
    # an empty toolchain name and accidentally invokes cargo +.
    $match = [regex]::Match($source, '(?m)^channel\s*=\s*"(nightly-\d{4}-\d{2}-\d{2})"')
    if (-not $match.Success) {
        throw 'Invalid OCR toolchain pin.'
    }
    return $match.Groups[1].Value
}
