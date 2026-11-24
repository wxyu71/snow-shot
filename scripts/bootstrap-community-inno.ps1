#Requires -Version 7.0
[CmdletBinding()]
param()
$ErrorActionPreference = 'Stop'
if (-not $env:RUNNER_TEMP -or $env:GITHUB_ACTIONS -ne 'true') { throw 'This bootstrap is for isolated CI runners.' }
$version = '7.1.1'
$hash = '8a5a6711c3766a6e5fb74f280ba0e3551e719ce94538d901fdbd1f8f9920b3f0'
$installer = Join-Path $env:RUNNER_TEMP "innosetup-$version-x64.exe"
$directory = Join-Path $env:RUNNER_TEMP "snow-inno-$version"
# Official release digest independently matches WinGet JRSoftware.InnoSetup.7 7.1.1.
& curl.exe --fail --location --silent --show-error --retry 2 --connect-timeout 15 --max-time 120 --output $installer "https://github.com/jrsoftware/issrc/releases/download/is-7_1_1/innosetup-$version-x64.exe"
if ($LASTEXITCODE -ne 0 -or (Get-Item -LiteralPath $installer).Length -ne 14336568 -or
    (Get-FileHash -LiteralPath $installer -Algorithm SHA256).Hash -ne $hash) {
    throw 'Inno compiler installer failed pinned byte verification.'
}
$p = Start-Process -FilePath $installer -ArgumentList @('/VERYSILENT','/SUPPRESSMSGBOXES','/NORESTART','/CURRENTUSER',('/DIR="' + $directory + '"')) -Wait -PassThru
if ($p.ExitCode -ne 0) { throw "Inno installation failed: $($p.ExitCode)" }
$compiler = Join-Path $directory 'ISCC.exe'
if (-not (Test-Path -LiteralPath $compiler -PathType Leaf)) { throw 'Missing pinned ISCC.exe' }
"SNOW_INNO_COMPILER=$compiler" | Out-File -FilePath $env:GITHUB_ENV -Encoding utf8 -Append
