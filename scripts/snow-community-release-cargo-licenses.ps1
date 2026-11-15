[CmdletBinding()]
param()

# One audited manifest/feature plan for both the early Windows preflight and
# the production NSIS license collector. Keep the full dependency inventory:
# never turn metadata collection into --no-deps or an incomplete feature scan.
function Get-SnowShotReleaseCargoLicensePlan {
    param(
        [Parameter(Mandatory)][string]$RepoRoot,
        [switch]$Store
    )
    $ffi = Join-Path $RepoRoot 'snow_rust_ffi/Cargo.toml'
    $ocr = Join-Path $RepoRoot 'snow-crates/crates/snow-ocr-process/Cargo.toml'
    $mcp = Join-Path $RepoRoot 'snow_shot/rust/snow-shot-mcp/Cargo.toml'
    $updater = Join-Path $RepoRoot 'snow_shot/rust/snow-shot-updater/Cargo.toml'
    $manifests = @($ffi, $ocr, $mcp)
    if (-not $Store) { $manifests += $updater }
    return [pscustomobject]@{
        Manifests = [string[]]$manifests
        Options = @{
            $ffi = @('--features', 'selected-text,language-detection')
            $ocr = @('--no-default-features', '--features', 'crash-diagnostics,gpu')
        }
    }
}
