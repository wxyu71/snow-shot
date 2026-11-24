#Requires -Version 7.0
[CmdletBinding()]
param(
    [Parameter(Mandatory)][string]$Compiler,
    [Parameter(Mandatory)][string]$BuildDirectory,
    [Parameter(Mandatory)][string]$StageDirectory,
    [Parameter(Mandatory)][ValidateSet('online','offline')][string]$Variant,
    [Parameter(Mandatory)][string]$OutputDirectory
)
$ErrorActionPreference = 'Stop'
$root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$compilerPath = (Resolve-Path -LiteralPath $Compiler).Path
if ((Get-Item -LiteralPath $compilerPath).VersionInfo.ProductVersion -notmatch '^7\.1\.1(?:\.0)?$') {
    throw 'Use the independently verified Inno Setup 7.1.1 compiler.'
}
$build = (Resolve-Path -LiteralPath $BuildDirectory).Path
$stage = (Resolve-Path -LiteralPath $StageDirectory).Path
$output = [IO.Path]::GetFullPath($OutputDirectory)
if ($output -eq $build -or $output -eq $stage) {
    throw 'Candidate installer output must be separate from accepted build/stage files.'
}
New-Item -ItemType Directory -Force -Path $output | Out-Null
$version = (& python (Join-Path $PSScriptRoot 'community_release.py') version)
if ($LASTEXITCODE -ne 0 -or $version -notmatch '^\d+\.\d+\.\d+-beta\.\d+$') { throw 'Invalid source version' }
$archive = Join-Path $build "snow-shot-$version-windows-x64-$Variant-update.zip"
$manifest = Join-Path $output "local-upgrade-$Variant.json"
if (Test-Path -LiteralPath $manifest) { throw 'Use a fresh candidate output directory.' }
& python (Join-Path $PSScriptRoot 'community_installer_signing.py') --local-variant $Variant --directory $build --output $manifest
if ($LASTEXITCODE -ne 0) { throw 'Local update payload validation failed.' }
$expected = @((Get-Content -LiteralPath $manifest -Raw | ConvertFrom-Json).files)
$actual = @(Get-ChildItem -LiteralPath $stage -File -Recurse -Force)
if ($actual.Count -ne $expected.Count) { throw 'Candidate stage inventory count mismatch.' }
foreach ($entry in $expected) {
    $file = Join-Path $stage $entry.path
    if (-not (Test-Path -LiteralPath $file -PathType Leaf) -or
        (Get-Item -LiteralPath $file).Length -ne $entry.size -or
        (Get-FileHash -LiteralPath $file -Algorithm SHA256).Hash -ne $entry.sha256) {
        throw "Candidate stage differs from verified ZIP: $($entry.path)"
    }
}
$hash = (Get-FileHash -LiteralPath $manifest -Algorithm SHA256).Hash.ToLowerInvariant()
$iss = Join-Path $root 'snow_shot/packaging/community/SnowShot.iss'
$icon = Join-Path $root 'snow_shot/resources/app-icon.ico'
& $compilerPath "/DStageDir=$stage" "/DUpdateZip=$archive" "/DLocalManifest=$manifest" "/DManifestSha=$hash" "/DAppVersion=$version" "/DVariant=$Variant" "/DOutputDir=$output" "/DAppIcon=$icon" $iss
if ($LASTEXITCODE -ne 0) { throw 'Inno candidate compilation failed.' }
$exe = Join-Path $output "snow-shot-$version-windows-x64-$Variant.exe"
if (-not (Test-Path -LiteralPath $exe -PathType Leaf)) { throw 'Missing candidate installer' }
Write-Output "Unaccepted Inno installer candidate: $exe"
