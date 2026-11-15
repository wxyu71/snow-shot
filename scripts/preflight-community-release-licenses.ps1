[CmdletBinding()]
param()

$ErrorActionPreference = 'Stop'
$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
. (Join-Path $PSScriptRoot 'snow-community-release-cargo-licenses.ps1')
. (Join-Path $PSScriptRoot 'snow-ocr-toolchain.ps1')
$plan = Get-SnowShotReleaseCargoLicensePlan -RepoRoot $repoRoot
$cargo = (Get-Command cargo -ErrorAction Stop).Source
$target = 'x86_64-pc-windows-msvc'
$ocrManifest = Join-Path $repoRoot 'snow-crates/crates/snow-ocr-process/Cargo.toml'
$ocrToolchain = Get-SnowOcrToolchain

# Cargo metadata --filter-platform still needs the complete locked workspace,
# including dev/other-platform dependencies like anes. Fetch all platforms
# (no --target) before testing strict --offline metadata and tree commands.
# Never use --no-deps or update Cargo.lock.
foreach ($manifest in $plan.Manifests) {
    $compiler = if ($manifest -ieq $ocrManifest) { @("+$ocrToolchain") } else { @() }
    $options = if ($plan.Options.ContainsKey($manifest)) { @($plan.Options[$manifest]) } else { @() }

    Write-Host "Prefetching locked complete Cargo workspace for release licenses: $manifest"
    & $cargo @compiler fetch --locked --manifest-path $manifest
    if ($LASTEXITCODE -ne 0) {
        throw "Locked Cargo workspace fetch failed for $manifest; do not bypass the offline license audit."
    }

    $metadataOutput = @(& $cargo @compiler metadata --locked --offline --filter-platform $target --format-version 1 --manifest-path $manifest @options)
    if ($LASTEXITCODE -ne 0) {
        throw "Full offline Cargo metadata failed after locked fetch: $manifest"
    }
    $metadata = ($metadataOutput -join [Environment]::NewLine) | ConvertFrom-Json
    $roots = @($metadata.packages | Where-Object {
        [IO.Path]::GetFullPath($_.manifest_path) -ieq [IO.Path]::GetFullPath($manifest)
    })
    if ($roots.Count -ne 1) {
        throw "Expected one root package in offline Cargo metadata for $manifest; found $($roots.Count)."
    }
    $rootId = [string]$roots[0].id
    & $cargo @compiler tree --locked --offline --target $target --manifest-path $manifest --package $rootId --edges normal,build --prefix none --format '{p}' @options | Out-Null
    if ($LASTEXITCODE -ne 0) {
        throw "Full offline Cargo tree failed for $manifest"
    }
    Write-Host "Full offline Cargo metadata and dependency tree verified: $manifest"
}

# Run the actual production license collector, including Qt/vcpkg notices,
# before the costly native C++/Rust compilation. Packaging re-runs it later
# with the exact same shared manifest/feature plan, so no gate is bypassed.
if (-not $env:SNOW_QT_STATIC_DIR) {
    throw 'Audited Qt directory is required before the release license preflight.'
}
$buildRoot = Join-Path $repoRoot 'build/snow-shot-msvc-release'
New-Item -ItemType Directory -Force -Path $buildRoot | Out-Null
$qtPrefix = [IO.Path]::GetFullPath((Join-Path $env:SNOW_QT_STATIC_DIR '../../..'))
$destination = Join-Path $buildRoot 'snow_shot/third-party-licenses/third-party'
$vcpkgPrefix = Join-Path $repoRoot '.tools/vcpkg/installed/static/x64-windows-static'
$licenseArgs = @{
    Destination = $destination
    AllowedRoot = $buildRoot
    VcpkgPrefix = $vcpkgPrefix
    QtPrefix = $qtPrefix
    CargoManifest = $plan.Manifests
    CargoOptions = $plan.Options
    AntDesignNotice = (Join-Path $repoRoot 'ant_design_qt/THIRD_PARTY_NOTICES.md')
    FallbackLicenseDirectory = (Join-Path $repoRoot 'licenses')
    CargoTarget = $target
}
& (Join-Path $PSScriptRoot 'collect-third-party-licenses.ps1') @licenseArgs
if ($LASTEXITCODE -ne 0) {
    throw 'Production release license collector failed in the early preflight.'
}
$manifestPath = Join-Path $destination 'manifest.json'
if (-not (Test-Path -LiteralPath $manifestPath -PathType Leaf) -or
    (Get-Item -LiteralPath $manifestPath).Length -le 0) {
    throw 'Release license preflight generated no license manifest.'
}
Write-Host "Full release license collection passed before native build: $manifestPath"
