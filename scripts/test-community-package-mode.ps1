[CmdletBinding()]
param()

$ErrorActionPreference = 'Stop'
$root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$packagePath = Join-Path $PSScriptRoot 'package-snow-shot.ps1'
$package = Get-Content -LiteralPath $packagePath -Raw

# Evaluate expressions extracted from the actual packaging script, not a
# duplicated expression in the test fixture.
$modeMatch = [regex]::Match($package, '(?m)^\$miniEnabled = (?<expression>.+)$')
if (-not $modeMatch.Success) {
    throw 'Packaging script is missing the central Mini mode decision.'
}
$cacheCode = '"SNOW_APPS_BUILD_SNOW_SHOT_MINI:BOOL=$(if ($miniEnabled) { ''ON'' } else { ''OFF'' })"'
if (-not $package.Contains($cacheCode)) {
    throw 'The release cache check does not use the shared $miniEnabled decision.'
}
$targetsMatch = [regex]::Match(
    $package,
    '(?s)\[string\[\]\]\$releaseTargets\s*=\s*(?<expression>if \(\$miniEnabled\) \{.*?\}\s*else \{.*?\})'
)
if (-not $targetsMatch.Success) {
    throw 'The full build target selection does not use the shared $miniEnabled decision.'
}

$modeExpression = [scriptblock]::Create($modeMatch.Groups['expression'].Value)
$cacheExpression = [scriptblock]::Create($cacheCode)
$targetsExpression = [scriptblock]::Create($targetsMatch.Groups['expression'].Value)

$cases = @(
    @{ Name = 'standalone Full+Mini legacy'; Store = $false; FullOnly = $false; Mini = $true; Targets = 'snow_shot,snow_shot_mini' },
    @{ Name = 'standalone FullOnly community beta'; Store = $false; FullOnly = $true; Mini = $false; Targets = 'snow_shot' },
    @{ Name = 'Store channel'; Store = $true; FullOnly = $false; Mini = $false; Targets = 'snow_shot' },
    @{ Name = 'Store channel FullOnly'; Store = $true; FullOnly = $true; Mini = $false; Targets = 'snow_shot' }
)
foreach ($case in $cases) {
    $store = [bool]$case.Store
    $FullOnly = [bool]$case.FullOnly
    $miniEnabled = [bool](& $modeExpression)
    $actualCacheEntry = [string](& $cacheExpression)
    [string[]]$actualTargets = @(& $targetsExpression)
    $expectedCacheEntry = if ($case.Mini) {
        'SNOW_APPS_BUILD_SNOW_SHOT_MINI:BOOL=ON'
    } else {
        'SNOW_APPS_BUILD_SNOW_SHOT_MINI:BOOL=OFF'
    }
    if ($miniEnabled -ne $case.Mini -or $actualCacheEntry -cne $expectedCacheEntry -or
        ($actualTargets -join ',') -cne $case.Targets) {
        throw "Packaging mode '$($case.Name)' mismatched. Mini=$miniEnabled; cache=$actualCacheEntry; targets=$($actualTargets -join ',')"
    }
    Write-Host "Package mode passed: $($case.Name) -> $actualCacheEntry; targets $($actualTargets -join ',')"
}

$presets = Get-Content -LiteralPath (Join-Path $root 'CMakePresets.json') -Raw | ConvertFrom-Json
$releasePreset = @($presets.configurePresets | Where-Object { $_.name -ceq 'snow-shot-msvc-release' })
if ($releasePreset.Count -ne 1 -or
    $releasePreset[0].cacheVariables.SNOW_APPS_BUILD_SNOW_SHOT_MINI -cne 'OFF') {
    throw 'The audited community release preset must disable Mini.'
}
if (-not $package.Contains('if (-not $FullOnly)')) {
    throw 'FullOnly must continue to suppress Mini artifact packaging.'
}
Write-Host 'PowerShell FullOnly/Store/Mini cache and target preflight passed.'
