#Requires -Version 7.0
<#
.SYNOPSIS
  Install, upgrade and uninstall the actual Community Windows x64 NSIS artifacts.
.DESCRIPTION
  CI-only smoke test. Uses a fresh temporary installation path and does not
  require any OCR credentials or network calls. This is not a substitute for
  the separate manual upgrade test from an official 1.5.7 installation.
#>
[CmdletBinding()]
param(
    [Parameter(Mandatory)][string]$BuildDirectory
)

$ErrorActionPreference = 'Stop'
$build = (Resolve-Path -LiteralPath $BuildDirectory).Path
$root = Join-Path $env:RUNNER_TEMP ('snow-shot-community-installer-smoke-' + [guid]::NewGuid().ToString('N'))
$installed = Join-Path $root 'Community install'
$binary = Join-Path $installed 'bin/snow_shot.exe'
$ocrAssets = Join-Path $installed 'bin/assets/ocr'
$uninstaller = Join-Path $installed 'Uninstall.exe'

function Invoke-Nsis([string]$Executable, [string[]]$Arguments) {
    $process = Start-Process -FilePath $Executable -ArgumentList $Arguments -PassThru -WindowStyle Hidden
    try {
        if (-not $process.WaitForExit(300000)) {
            $process.Kill()
            $process.WaitForExit()
            throw "Installer timed out: $Executable"
        }
        if ($process.ExitCode -ne 0) {
            throw "Installer failed with exit code $($process.ExitCode): $Executable"
        }
    }
    finally {
        $process.Dispose()
    }
}

function Get-ExactlyOne([string]$Variant) {
    $files = @(Get-ChildItem -LiteralPath $build -File -Filter "snow-shot-*-windows-x64-$Variant.exe")
    if ($files.Count -ne 1) {
        throw "Expected one $Variant x64 NSIS installer, got $($files.Count)."
    }
    return $files[0].FullName
}

function Assert-Installed([string]$Variant) {
    if (-not (Test-Path -LiteralPath $binary -PathType Leaf)) {
        throw "$Variant installation did not install bin/snow_shot.exe."
    }
    if (-not (Test-Path -LiteralPath $uninstaller -PathType Leaf)) {
        throw "$Variant installation did not create Uninstall.exe."
    }
    $manifest = Join-Path $ocrAssets 'asset-manifest.json'
    if (-not (Test-Path -LiteralPath $manifest -PathType Leaf)) {
        throw "$Variant installation is missing the trusted OCR asset manifest."
    }
    $assets = @(Get-ChildItem -LiteralPath $ocrAssets -Recurse -File)
    if ($Variant -eq 'online' -and ($assets.Count -ne 1 -or $assets[0].Name -ne 'asset-manifest.json')) {
        throw 'The online installation unexpectedly bundled local OCR runtime/model assets.'
    }
    if ($Variant -eq 'offline' -and $assets.Count -le 1) {
        throw 'The offline installation is missing the bundled OCR runtime/model payload.'
    }
    Write-Host "PASS: installed $Variant full x64 installer and OCR payload."
}

if (-not $env:RUNNER_TEMP) { throw 'Run this destructive installer smoke test only in an isolated CI runner.' }
$online = Get-ExactlyOne 'online'
$offline = Get-ExactlyOne 'offline'
New-Item -ItemType Directory -Force -Path $root | Out-Null
$didUninstall = $false
try {
    # NSIS /D must be the final argument, without surrounding quotes.
    Invoke-Nsis $online @('/S', "/D=$installed")
    Assert-Installed 'online'

    # Upgrade the real online installer in place with the real offline installer.
    # The product's NSIS uninstall-before-install behavior is exercised here.
    Invoke-Nsis $offline @('/S', "/D=$installed")
    Assert-Installed 'offline'

    # The _?= option keeps the uninstaller attached to this test destination.
    Invoke-Nsis $uninstaller @('/S', "_?=$installed")
    $didUninstall = $true
    if (Test-Path -LiteralPath $binary -PathType Leaf) {
        throw 'Silent uninstall did not remove snow_shot.exe.'
    }
    Write-Host 'PASS: online -> offline in-place upgrade and silent uninstall.'
}
finally {
    if (-not $didUninstall -and (Test-Path -LiteralPath $uninstaller -PathType Leaf)) {
        try { Invoke-Nsis $uninstaller @('/S', "_?=$installed") }
        catch { Write-Warning "Best-effort cleanup uninstall failed: $_" }
    }
    if (Test-Path -LiteralPath $root) {
        # Only delete the unique test sandbox, never any external installation.
        $actual = [IO.Path]::GetFullPath($root)
        $allowed = [IO.Path]::GetFullPath($env:RUNNER_TEMP).TrimEnd('\') + '\'
        if (-not $actual.StartsWith($allowed, [StringComparison]::OrdinalIgnoreCase)) {
            throw 'Refusing to delete an installation path outside the CI sandbox.'
        }
        Remove-Item -LiteralPath $root -Recurse -Force
    }
}
