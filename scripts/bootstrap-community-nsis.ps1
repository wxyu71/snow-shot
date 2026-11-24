[CmdletBinding()]
param()

$ErrorActionPreference = 'Stop'
# Reuse a trusted runner installation only at the exact pinned version. If
# absent, install the official, hash-pinned package without Chocolatey's feed.
# SHA256 and URL: microsoft/winget-pkgs@bc23f96b968c037a79e8929f09654177dabe165a
# manifests/n/NSIS/NSIS/3.12/NSIS.NSIS.installer.yaml
$version = '3.12'
$expectedHash = '3BC2B06253A7E4957111BE152AC6A536E0C7478A706E19DA814038DB5D706495'
$nsisCandidates = @(
    "${env:ProgramFiles(x86)}\NSIS\makensis.exe",
    "${env:ProgramFiles}\NSIS\makensis.exe"
)

function Find-PinnedNsis {
    foreach ($candidate in $nsisCandidates) {
        if (-not (Test-Path -LiteralPath $candidate -PathType Leaf)) { continue }
        $actual = (& $candidate -VERSION 2>&1 | Out-String).Trim()
        if ($LASTEXITCODE -eq 0 -and $actual -match '(?i)^v?3\.12(?:\.0)?$') {
            return $candidate
        }
        Write-Information -InformationAction Continue -MessageData "Ignoring unexpected NSIS version '$actual' at $candidate"
    }
}

$makensis = Find-PinnedNsis
if (-not $makensis) {
    $installer = Join-Path $env:RUNNER_TEMP "nsis-$version-setup.exe"
    $urls = @(
        'https://sourceforge.net/projects/nsis/files/NSIS%203/3.12/nsis-3.12-setup.exe/download',
        'https://downloads.sourceforge.net/project/nsis/NSIS%203/3.12/nsis-3.12-setup.exe'
    )
    $verified = $false
    for ($attempt = 1; $attempt -le 3 -and -not $verified; $attempt++) {
        foreach ($url in $urls) {
            try {
                if (Test-Path -LiteralPath $installer) {
                    Remove-Item -LiteralPath $installer -Force
                }
                Invoke-WebRequest -Uri $url -OutFile $installer -TimeoutSec 90
                $hash = (Get-FileHash -LiteralPath $installer -Algorithm SHA256).Hash
                if ($hash -ne $expectedHash) {
                    throw "NSIS installer SHA-256 mismatch: $hash"
                }
                $verified = $true
                break
            } catch {
                Write-Warning "NSIS download attempt $attempt from $url failed: $($_.Exception.Message)"
            }
        }
        if (-not $verified -and $attempt -lt 3) { Start-Sleep -Seconds (5 * $attempt) }
    }
    if (-not $verified) { throw "Could not download verified NSIS $version." }
    $process = Start-Process -FilePath $installer -ArgumentList '/S' -Wait -PassThru
    if ($process.ExitCode -ne 0) {
        throw "Verified NSIS installer failed with exit code $($process.ExitCode)."
    }
    $makensis = Find-PinnedNsis
    if (-not $makensis) { throw "NSIS $version was not found after installation." }
}
$actual = (& $makensis -VERSION 2>&1 | Out-String).Trim()
if ($LASTEXITCODE -ne 0 -or $actual -notmatch '(?i)^v?3\.12(?:\.0)?$') {
    throw "Expected pinned NSIS $version, found '$actual' at $makensis."
}

$nsisDirectory = Split-Path -Parent $makensis
$env:PATH = "$nsisDirectory;$env:PATH"
if ($env:GITHUB_PATH) {
    $nsisDirectory | Out-File -FilePath $env:GITHUB_PATH -Append -Encoding utf8
}
Write-Information -InformationAction Continue -MessageData "Validated pinned NSIS compiler $actual at $makensis"

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
Write-Information -InformationAction Continue -MessageData "CPack NSIS smoke passed: $($installers[0].FullName)"
