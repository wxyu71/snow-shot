[CmdletBinding()]
param(
    [ValidateSet('x64', 'arm64')][string]$Architecture = 'x64',
    [ValidateSet('Debug', 'Release')][string]$Configuration = 'Release',
    [switch]$DevelopmentModules
)

# Content-addressed identity for a compiled Qt kit. All inputs are observed
# AFTER vcpkg bootstrap, so changes in resolved ABI cannot reuse an old kit.
$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot 'snow-build-environment.ps1')
Add-SnowMsvcToolsToPath -Architecture $Architecture | Out-Null
$target = Get-SnowWindowsTarget -Architecture $Architecture
$dependencyPrefix = Join-Path $target.InstalledRoot $target.Triplet

function Get-RequiredFileHash {
    param([Parameter(Mandatory = $true)][string]$Path)
    if (-not (Test-Path -LiteralPath $Path -PathType Leaf)) {
        throw "Missing Qt cache identity input: $Path"
    }
    return (Get-FileHash -LiteralPath $Path -Algorithm SHA256).Hash.ToLowerInvariant()
}

# Keep the declared Qt source patches in the identity, not a duplicated list.
$recipePaths = @(
    'scripts/build-static-qt.ps1',
    'scripts/snow-build-environment.ps1',
    'scripts/qt-cache-identity.ps1',
    'scripts/build-toolchain.json',
    'scripts/qt-toolchain.json',
    'scripts/static-qt-features.json'
) + @($script:SnowStaticQtSourcePatches | ForEach-Object { "scripts/$($_.File)" })
$recipes = [ordered]@{}
foreach ($relative in ($recipePaths | Sort-Object -Unique)) {
    $recipes[$relative] = Get-RequiredFileHash (Join-Path $script:SnowRepoRoot $relative)
}

# These are the installed package ABI records read by build-static-qt.ps1
# when it validates its static-qt-build.json stamp.
$dependencies = [ordered]@{}
foreach ($port in @('libpng', 'zlib')) {
    $dependencies[$port] = Get-RequiredFileHash (
        Join-Path $dependencyPrefix "share/$port/vcpkg_abi_info.txt")
}

# A compiled Qt SDK is ABI-sensitive. A rolling runner image label is not a
# compiler identity: hash the actual build tools selected on this runner.
$tools = [ordered]@{}
foreach ($name in @('cl.exe', 'link.exe', 'lib.exe', 'cmake.exe', 'rc.exe', 'mt.exe', 'ninja.exe')) {
    $command = Get-Command $name -CommandType Application -ErrorAction SilentlyContinue |
        Select-Object -First 1
    if ($null -eq $command) {
        if ($name -in @('cl.exe', 'link.exe', 'lib.exe', 'cmake.exe')) {
            throw "Cannot identify required Qt build tool: $name"
        }
        $tools[$name] = 'absent'
    } else {
        $tools[$name] = Get-RequiredFileHash $command.Source
    }
}

$identity = [ordered]@{
    QtVersion = $script:SnowQtVersion
    QtSourceArchiveSha256 = $script:SnowQtToolchain.sourceArchiveSha256
    QtStampSchema = $script:SnowStaticQtSchemaVersion
    FeatureFingerprint = $script:SnowStaticQtFeatureFingerprint
    Architecture = $Architecture
    HostArchitecture = $target.HostArchitecture
    Configuration = $Configuration
    DevelopmentModules = [bool]($DevelopmentModules -or $Configuration -eq 'Debug')
    MsvcToolset = "$env:VCToolsVersion"
    WindowsSdk = "$env:WindowsSDKVersion"
    PowerShell = $PSVersionTable.PSVersion.ToString()
    Recipes = $recipes
    InstalledDependencyAbi = $dependencies
    Tools = $tools
}
$json = $identity | ConvertTo-Json -Depth 10 -Compress
$digest = [Convert]::ToHexString(
    [System.Security.Cryptography.SHA256]::HashData(
        [System.Text.Encoding]::UTF8.GetBytes($json))).ToLowerInvariant()
$prefix = Join-Path $script:SnowRepoRoot (
    ".tools/qt/$($script:SnowQtVersion)/$Architecture-static-$($Configuration.ToLowerInvariant())")

if ($env:GITHUB_OUTPUT) {
    Add-Content -LiteralPath $env:GITHUB_OUTPUT -Value "key=$digest"
    Add-Content -LiteralPath $env:GITHUB_OUTPUT -Value "prefix=$prefix"
}
Write-Host "Qt cache: source $($script:SnowQtVersion), arch $Architecture, policy $($script:SnowStaticQtSchemaVersion)"
Write-Host "Qt cache identity: $digest"
Write-Host "Qt install prefix: $prefix"
