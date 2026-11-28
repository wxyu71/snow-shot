#Requires -Version 7.0
[CmdletBinding()]
param([Parameter(Mandatory)][string]$BuildDirectory)
$ErrorActionPreference = 'Stop'
if ($env:GITHUB_ACTIONS -ne 'true' -or -not $env:RUNNER_TEMP) {
    throw 'Run only on an isolated GitHub Actions Windows runner.'
}
$build = (Resolve-Path -LiteralPath $BuildDirectory).Path
$root = Join-Path $env:RUNNER_TEMP ('snow-inno-production-' + [Guid]::NewGuid().ToString('N'))
$version = (& python (Join-Path $PSScriptRoot 'community_release.py') version)
if ($LASTEXITCODE -ne 0 -or $version -notmatch '^\d+\.\d+\.\d+-beta\.\d+$') { throw 'Invalid release version' }
$registration = 'Software\Snow Shot Community\SnowShot'
$upstreamRegistration = 'Software\Snow Apps\SnowShot'
$uninstall = 'Software\Microsoft\Windows\CurrentVersion\Uninstall\SnowShot'
$keys = @($registration, $upstreamRegistration, $uninstall, ($uninstall + '_is1'))
function Get-RegistryValues([string]$SubKey, [string]$Name) {
    foreach ($hive in @('CurrentUser','LocalMachine')) {
        foreach ($view in @('Registry64','Registry32')) {
            $baseKey = [Microsoft.Win32.RegistryKey]::OpenBaseKey(
                [Microsoft.Win32.RegistryHive]$hive, [Microsoft.Win32.RegistryView]$view)
            try {
                $key = $baseKey.OpenSubKey($SubKey)
                if ($null -ne $key) {
                    try { [pscustomobject]@{ Hive = $hive; View = $view; Value = $key.GetValue($Name) } }
                    finally { $key.Dispose() }
                }
            } finally { $baseKey.Dispose() }
        }
    }
}
function Assert-CleanRegistration {
    foreach ($subKey in $keys) {
        if (@(Get-RegistryValues $subKey '').Count -ne 0) {
            throw "Refusing to test over an existing Snow Shot registration: $subKey"
        }
    }
}
function Invoke-Installer([string]$Executable, [string[]]$Arguments) {
    $process = Start-Process -FilePath $Executable -ArgumentList $Arguments -PassThru -Wait
    try {
        if ($process.ExitCode -ne 0) { throw "Installer exit $($process.ExitCode): $Executable" }
    } finally { $process.Dispose() }
}
function Get-Installer([string]$Variant) {
    $file = Join-Path $build "snow-shot-$version-windows-x64-$Variant.exe"
    if (-not (Test-Path -LiteralPath $file -PathType Leaf)) { throw "Missing $file" }
    return $file
}
function Assert-Installed([string]$Directory, [string]$Variant) {
    $record = Get-Content -LiteralPath (Join-Path $Directory 'snow-shot-installation.json') -Raw | ConvertFrom-Json
    if ($record.version -cne $version -or $record.variant -cne $Variant) { throw 'Wrong installed identity' }
    $manifest = Get-Content -LiteralPath (Join-Path $build "snow-shot-$version-windows-x64-$Variant.manifest.json") -Raw | ConvertFrom-Json
    foreach ($entry in $manifest.InstallFiles) {
        $file = Join-Path $Directory $entry.Path
        if (-not (Test-Path -LiteralPath $file -PathType Leaf) -or
            (Get-Item -LiteralPath $file).Length -ne $entry.Bytes -or
            (Get-FileHash -LiteralPath $file -Algorithm SHA256).Hash -ne $entry.Sha256) {
            throw "Installed bytes differ from approved stage: $($entry.Path)"
        }
    }
    $ocr = @(Get-ChildItem -LiteralPath (Join-Path $Directory 'bin/assets/ocr') -Recurse -File)
    if (($Variant -eq 'online' -and ($ocr.Count -ne 1 -or $ocr[0].Name -ne 'asset-manifest.json')) -or
        ($Variant -eq 'offline' -and $ocr.Count -le 1)) {
        throw 'Installed OCR payload does not match the selected variant'
    }
    $versions = @((Get-RegistryValues $uninstall 'DisplayVersion'), (Get-RegistryValues ($uninstall + '_is1') 'DisplayVersion')) |
        Where-Object { $null -ne $_ }
    if (@($versions).Count -eq 0 -or @($versions | Where-Object { $_.Value -cne $version }).Count -ne 0) {
        throw 'Installed registry version did not match release'
    }
}
function Invoke-Inno([string]$Executable, [string]$Directory, [string]$Scope, [string]$Language) {
    Invoke-Installer $Executable @('/VERYSILENT','/SUPPRESSMSGBOXES','/NORESTART',
        $Scope, "/LANG=$Language", ('/DIR="' + $Directory + '"'))
}
# These are the immutable Beta 1 assets verified against the published GitHub
# Release API digest, not reconstructed fixtures or untrusted downloaded hashes.
$legacy = @{
    online = @{ Bytes = 25455054; Sha256 = '860225feb19d02e073f3821c661378ffaeb3da2ad0a7eaf5d65bdd5c327bd7ec' }
    offline = @{ Bytes = 62440134; Sha256 = '43cfd60a38d00e1faa27961439a289f542af2284fb6eb72d2f3fc83543e9fb86' }
}
Assert-CleanRegistration
New-Item -ItemType Directory -Path $root | Out-Null
try {
    foreach ($variant in @('online','offline')) {
        $language = if ($variant -eq 'online') { 'zhCN' } else { 'zhTW' }
        $installer = Get-Installer $variant
        $fresh = Join-Path $root "fresh-$variant"
        Invoke-Inno $installer $fresh '/CURRENTUSER' $language
        Assert-Installed $fresh $variant
        $uninstallers = @(Get-ChildItem -LiteralPath $fresh -File -Filter 'unins*.exe')
        if ($uninstallers.Count -ne 1) { throw 'Expected one Inno uninstaller' }
        Invoke-Installer $uninstallers[0].FullName @('/VERYSILENT','/SUPPRESSMSGBOXES','/NORESTART')
        if (Test-Path -LiteralPath (Join-Path $fresh 'bin/snow_shot.exe')) { throw 'Fresh Inno uninstall failed' }
        Assert-CleanRegistration

        $name = "snow-shot-1.6.3-beta.1-windows-x64-$variant.exe"
        $oldInstaller = Join-Path $root $name
        $url = "https://github.com/wxyu71/snow-shot/releases/download/v1.6.3-beta.1_snow-shot/$name"
        & curl.exe --fail --location --silent --show-error --retry 2 --connect-timeout 15 --max-time 300 --output $oldInstaller $url
        if ($LASTEXITCODE -ne 0 -or (Get-Item -LiteralPath $oldInstaller).Length -ne $legacy[$variant].Bytes -or
            (Get-FileHash -LiteralPath $oldInstaller -Algorithm SHA256).Hash -ne $legacy[$variant].Sha256) {
            throw 'Published Beta 1 asset failed pinned verification'
        }
        $installed = Join-Path $root "legacy-$variant"
        # NSIS /D must be last and must not be surrounded by quotes.
        Invoke-Installer $oldInstaller @('/S', "/D=$installed")
        $recordPath = Join-Path $installed 'snow-shot-installation.json'
        $record = Get-Content -LiteralPath $recordPath -Raw | ConvertFrom-Json
        if ($record.version -cne '1.6.3-beta.1' -or $record.variant -cne $variant) {
            throw 'Published Beta 1 install does not match the requested variant'
        }
        $oldUninstaller = Join-Path $installed 'Uninstall Snow Shot.exe'
        $oldHash = (Get-FileHash -LiteralPath $oldUninstaller -Algorithm SHA256).Hash
        $sentinel = Join-Path $installed 'community-user-data-sentinel.txt'
        'must survive upgrade' | Set-Content -LiteralPath $sentinel
        $registrations = @(Get-RegistryValues $registration '' | Where-Object { $_.Value -eq $installed })
        $hives = @($registrations.Hive | Sort-Object -Unique)
        if ($hives.Count -ne 1) { throw 'Cannot determine legacy installation scope' }
        $scope = if ($hives[0] -eq 'LocalMachine') { '/ALLUSERS' } else { '/CURRENTUSER' }
        Invoke-Inno $installer $installed $scope 'en'
        Assert-Installed $installed $variant
        if ((Get-Content -LiteralPath $sentinel -Raw).Trim() -cne 'must survive upgrade' -or
            (Get-FileHash -LiteralPath $oldUninstaller -Algorithm SHA256).Hash -ne $oldHash) {
            throw 'Upgrade changed unowned user data or the legacy uninstaller'
        }
        # The legacy uninstaller is retained deliberately; exercise its new owned-file cleanup path.
        Invoke-Installer $oldUninstaller @('/S', "_?=$installed")
        if (Test-Path -LiteralPath (Join-Path $installed 'bin/snow_shot.exe')) { throw 'Migrated uninstall failed' }
        Assert-CleanRegistration
        Write-Output "Accepted fresh Inno install/uninstall and real Beta 1 $variant upgrade/uninstall"
    }
} finally {
    # Runner teardown removes the isolated sandbox if an installer is still registered.
    # Never forcibly remove registry entries or installation files to disguise failure.
    $remaining = @($keys | ForEach-Object { Get-RegistryValues $_ '' })
    if ($remaining.Count -eq 0 -and (Test-Path -LiteralPath $root)) {
        Remove-Item -LiteralPath $root -Recurse -Force
    }
}
