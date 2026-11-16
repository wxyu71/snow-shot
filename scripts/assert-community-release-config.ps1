[CmdletBinding()]
param(
    [string]$BuildDirectory = 'build/snow-shot-msvc-release',
    [ValidateSet('ON', 'OFF')][string]$ExpectedMini = 'OFF'
)

$ErrorActionPreference = 'Stop'
$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$build = if ([IO.Path]::IsPathRooted($BuildDirectory)) {
    [IO.Path]::GetFullPath($BuildDirectory)
} else {
    [IO.Path]::GetFullPath((Join-Path $repoRoot $BuildDirectory))
}
$cachePath = Join-Path $build 'CMakeCache.txt'
$cpackPath = Join-Path $build 'CPackConfig.cmake'
foreach ($file in @($cachePath, $cpackPath)) {
    if (-not (Test-Path -LiteralPath $file -PathType Leaf)) {
        throw "Release configure was not completed: $file"
    }
}
$cache = Get-Content -LiteralPath $cachePath -Raw
foreach ($entry in @(
    'SNOW_APPS_BUILD_SNOW_SHOT:BOOL=ON',
    'SNOW_APPS_PACKAGE_SNOW_SHOT:BOOL=ON',
    "SNOW_APPS_BUILD_SNOW_SHOT_MINI:BOOL=$ExpectedMini",
    'SNOW_SHOT_DISTRIBUTION_CHANNEL:STRING=standalone'
)) {
    if (-not ([regex]::IsMatch($cache, "(?m)^$([regex]::Escape($entry))\r?$"))) {
        throw "Release configuration is missing '$entry'."
    }
}

# Both CI configure-time preflight and actual packaging use this contract.
# The project, not an old official-domain constant, owns the installer URL.
$cmakeSource = Get-Content -LiteralPath (Join-Path $repoRoot 'CMakeLists.txt') -Raw
$homepageMatch = [regex]::Match($cmakeSource,
    '(?m)^set\(SNOW_SHOT_HOMEPAGE_URL\s+"(?<url>[^"]+)"\)')
if (-not $homepageMatch.Success) {
    throw 'SNOW_SHOT_HOMEPAGE_URL was not defined by the project.'
}
$homepage = $homepageMatch.Groups['url'].Value
if ($homepage -cne 'https://github.com/wxyu71/snow-shot') {
    throw "Community installer homepage does not identify this fork: $homepage"
}

$cpack = Get-Content -LiteralPath $cpackPath -Raw
$requiredCpackSettings = [ordered]@{
    CPACK_CREATE_DESKTOP_LINKS = 'snow_shot'
    CPACK_PACKAGE_EXECUTABLES = 'snow_shot;Snow Shot'
    CPACK_PACKAGE_HOMEPAGE_URL = $homepage
    CPACK_PACKAGE_VENDOR = 'Snow Shot Community'
    CPACK_PACKAGE_INSTALL_DIRECTORY = 'SnowShot'
    CPACK_PACKAGE_INSTALL_REGISTRY_KEY = 'SnowShot'
    CPACK_NSIS_INSTALLED_ICON_NAME = 'bin\\snow_shot.exe'
}
foreach ($name in $requiredCpackSettings.Keys) {
    $settingPresent = if ($name -eq 'CPACK_NSIS_INSTALLED_ICON_NAME') {
        $cpack -match 'set\(CPACK_NSIS_INSTALLED_ICON_NAME "bin\\+snow_shot\.exe"\)'
    } else {
        $pattern = 'set\(' + [regex]::Escape($name) + ' "' +
            [regex]::Escape($requiredCpackSettings[$name]) + '"\)'
        $cpack -match $pattern
    }
    if (-not $settingPresent) {
        throw "CPack configuration is missing '$name=$($requiredCpackSettings[$name])'."
    }
}
if ($cpack -notmatch 'set\(CPACK_PACKAGE_VERSION "[^"]+"\)') {
    throw 'CPack package version is missing.'
}
Write-Host "Community release CPack contract verified (Mini=$ExpectedMini; homepage=$homepage)."

