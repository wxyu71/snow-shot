[CmdletBinding()]
param()

$ErrorActionPreference = 'Stop'
# CPack's NSIS generator requires NSIS >= 3.03. Keep the CI packaging
# toolchain reproducible, rather than relying on hosted runner image contents.
$version = '3.12.0'
$choco = Get-Command choco.exe -ErrorAction Stop
& $choco.Source install nsis "--version=$version" --yes --no-progress --limit-output
if ($LASTEXITCODE -ne 0) {
    throw "Chocolatey failed to install pinned NSIS $version (exit code $LASTEXITCODE)."
}

$nsisCandidates = @(
    "${env:ProgramFiles(x86)}\NSIS\makensis.exe",
    "${env:ProgramFiles}\NSIS\makensis.exe"
)
$makensis = $nsisCandidates |
    Where-Object { Test-Path -LiteralPath $_ -PathType Leaf } |
    Select-Object -First 1
if (-not $makensis) {
    throw "Pinned NSIS installation succeeded but makensis.exe was not found."
}
$actual = (& $makensis -VERSION 2>&1 | Out-String).Trim()
if ($LASTEXITCODE -ne 0 -or $actual -notmatch '(?i)\bv3\.12(?:\.0)?\b') {
    throw "Expected pinned NSIS $version, found '$actual' at $makensis."
}

$nsisDirectory = Split-Path -Parent $makensis
$env:PATH = "$nsisDirectory;$env:PATH"
if ($env:GITHUB_PATH) {
    $nsisDirectory | Out-File -FilePath $env:GITHUB_PATH -Append -Encoding utf8
}
Write-Host "Validated pinned NSIS compiler $actual at $makensis"

# A real CPack -> makensis smoke test catches missing plugins / PATH /
# generator incompatibility *before* vcpkg and the expensive native build.
$work = Join-Path $env:RUNNER_TEMP 'snow-shot-nsis-smoke'
$source = Join-Path $work 'src'
$build = Join-Path $work 'build'
$output = Join-Path $work 'dist'
New-Item -ItemType Directory -Force -Path $source, $output | Out-Null
@'
cmake_minimum_required(VERSION 3.25)
project(SnowShotCommunityNsisSmoke LANGUAGES NONE)
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/payload.txt" "NSIS smoke payload\n")
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/payload.txt" DESTINATION ".")
set(CPACK_PACKAGE_NAME "SnowShotCommunityNsisSmoke")
set(CPACK_PACKAGE_VENDOR "Snow Shot Community")
set(CPACK_PACKAGE_VERSION "0.0.1")
set(CPACK_PACKAGE_FILE_NAME "snow-shot-community-nsis-smoke")
include(CPack)
'@ | Set-Content -LiteralPath (Join-Path $source 'CMakeLists.txt') -Encoding utf8

& cmake -S $source -B $build -G Ninja
if ($LASTEXITCODE -ne 0) { throw 'CMake NSIS smoke configure failed.' }
& cpack --config (Join-Path $build 'CPackConfig.cmake') -G NSIS -B $output
if ($LASTEXITCODE -ne 0) { throw 'CPack NSIS smoke packaging failed.' }
$installers = @(Get-ChildItem -LiteralPath $output -Filter '*.exe' -File)
if ($installers.Count -ne 1 -or $installers[0].Length -le 0) {
    throw "CPack NSIS smoke produced $($installers.Count) nonempty installers; expected exactly one."
}
Write-Host "CPack NSIS smoke passed: $($installers[0].FullName)"
