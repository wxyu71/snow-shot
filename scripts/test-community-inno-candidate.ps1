#Requires -Version 7.0
[CmdletBinding()]
param()
$ErrorActionPreference = 'Stop'
if ($env:GITHUB_ACTIONS -ne 'true' -or -not $env:RUNNER_TEMP) { throw 'Requires an isolated CI runner.' }
$base = Join-Path $env:RUNNER_TEMP ("snow-inno-contract-" + [Guid]::NewGuid().ToString('N'))
$stage = Join-Path $base 'stage'
$build = Join-Path $base 'build'
$install = Join-Path $base 'existing'
$output = Join-Path $base 'candidate'
$target = Join-Path $base 'cargo'
$registration = 'HKCU:\Software\Snow Shot Community\SnowShot'
$uninstall = 'HKCU:\Software\Microsoft\Windows\CurrentVersion\Uninstall\SnowShot'
$innoKey = 'HKCU:\Software\Microsoft\Windows\CurrentVersion\Uninstall\SnowShot_is1'
foreach ($key in @($registration,$uninstall,$innoKey)) {
    if (Test-Path -LiteralPath $key) { throw 'Refusing to touch existing installation registry data.' }
}
New-Item -ItemType Directory -Force -Path "$stage\bin","$install\bin",$build | Out-Null
try {
    $manifest = 'snow_shot/rust/snow-shot-updater/Cargo.toml'
    cargo test --locked --manifest-path $manifest --target-dir $target --lib community_transaction_feed_requires_both_exact_variants_and_inventory
    if ($LASTEXITCODE -ne 0) { throw 'Signed update contract failed.' }
    cargo test --locked --manifest-path $manifest --target-dir $target --lib local_installer_manifest_has_no_portable_or_foreign_payload
    if ($LASTEXITCODE -ne 0) { throw 'Local installer manifest contract failed.' }
    cargo build --locked --manifest-path $manifest --target-dir $target --bin snow-shot-updater
    if ($LASTEXITCODE -ne 0) { throw 'Rust updater build failed.' }
    Copy-Item -LiteralPath "$target\debug\snow-shot-updater.exe" -Destination "$stage\bin\snow-shot-updater.exe"
    'fn main() { if std::env::args().nth(1).as_deref() != Some("--update-probe") { std::process::exit(1); } }' | Set-Content -LiteralPath "$base\probe.rs"
    rustc "$base\probe.rs" -o "$stage\bin\snow_shot.exe"
    if ($LASTEXITCODE -ne 0) { throw 'Fixture executable build failed.' }
    $version = (& python scripts/community_release.py version).Trim()
    if ($LASTEXITCODE -ne 0 -or $version -notmatch '^\d+\.\d+\.\d+-beta\.\d+$') { throw 'Invalid fixture version' }
    $env:SNOW_INNO_FIXTURE = $base
    $env:SNOW_INNO_VERSION = $version
    @'
import hashlib, json, os, pathlib, zipfile
root = pathlib.Path(os.environ["SNOW_INNO_FIXTURE"])
stage, old, build = (root / n for n in ("stage", "existing", "build"))
version = os.environ["SNOW_INNO_VERSION"]
record = "snow-shot-installation.json"
def inventory(directory):
    return [{"path": p.relative_to(directory).as_posix(), "size": p.stat().st_size,
             "sha256": hashlib.sha256(p.read_bytes()).hexdigest()}
            for p in sorted(directory.rglob("*")) if p.is_file() and p.name != record]
def save(directory, v):
    (directory / record).write_text(json.dumps({"schema": 1, "product": "snow-shot",
        "platform": "windows-x64", "version": v, "variant": "online", "files": inventory(directory)}), encoding="utf-8")
(old / "bin/snow_shot.exe").write_bytes(b"old app")
(old / "bin/snow-shot-updater.exe").write_bytes(b"old helper")
(old / "bin/obsolete.dll").write_bytes(b"old owned file")
save(old, "0.0.1-beta.1")
(old / "bin/portable").mkdir()
(old / "bin/portable/user-data.txt").write_text("keep me", encoding="utf-8")
(old / "Uninstall.exe").write_bytes(b"must never execute or remove this sentinel")
save(stage, version)
stem = f"snow-shot-{version}-windows-x64-online-update"
archive = build / (stem + ".zip")
with zipfile.ZipFile(archive, "w", zipfile.ZIP_DEFLATED) as z:
    for p in stage.rglob("*"):
        if p.is_file(): z.write(p, p.relative_to(stage).as_posix())
entries = [{"Path": p.relative_to(stage).as_posix(), "Bytes": p.stat().st_size,
            "Sha256": hashlib.sha256(p.read_bytes()).hexdigest()} for p in stage.rglob("*") if p.is_file()]
(build / (stem + ".manifest.json")).write_text(json.dumps({
    "PackageVersion": version, "Variant": "online", "Architecture": "x64",
    "Platform": "windows-x64", "NativeValidation": {"Passed": True},
    "Archive": {"Sha256": hashlib.sha256(archive.read_bytes()).hexdigest()}, "InstallFiles": entries}), encoding="utf-8")
'@ | python -
    if ($LASTEXITCODE -ne 0) { throw 'Fixture staging failed.' }
    & ./scripts/build-community-inno-candidate.ps1 -Compiler $env:SNOW_INNO_COMPILER -BuildDirectory $build -StageDirectory $stage -Variant online -OutputDirectory $output
    New-Item -Path $registration -Force | Out-Null
    Set-Item -LiteralPath $registration -Value $install
    New-Item -Path $uninstall -Force | Out-Null
    New-ItemProperty -LiteralPath $uninstall -Name DisplayVersion -Value '0.0.1-beta.1' | Out-Null
    New-ItemProperty -LiteralPath $uninstall -Name UninstallString -Value ('"' + "$install\Uninstall.exe" + '"') | Out-Null
    $exe = Join-Path $output "snow-shot-$version-windows-x64-online.exe"
    $p = Start-Process -FilePath $exe -ArgumentList @('/VERYSILENT','/SUPPRESSMSGBOXES','/NORESTART','/CURRENTUSER',('/LOG="' + "$base\upgrade.log" + '"')) -Wait -PassThru
    if ($p.ExitCode -ne 0) { Get-Content "$base\upgrade.log" -ErrorAction SilentlyContinue; throw "Candidate upgrade failed: $($p.ExitCode)" }
    $record = Get-Content "$install\snow-shot-installation.json" -Raw | ConvertFrom-Json
    if ($record.version -ne $version -or (Get-ItemProperty $uninstall).DisplayVersion -ne $version -or
        (Test-Path "$install\bin\obsolete.dll") -or (Test-Path $innoKey) -or
        (Get-Content "$install\bin\portable\user-data.txt" -Raw) -ne 'keep me' -or
        (Get-Content "$install\Uninstall.exe" -Raw) -ne 'must never execute or remove this sentinel' -or
        (Get-FileHash "$install\bin\snow_shot.exe").Hash -ne (Get-FileHash "$stage\bin\snow_shot.exe").Hash) {
        throw 'In-place upgrade violated file, user-data or registry ownership contract.'
    }
    Write-Output 'Synthetic legacy registration upgraded without running its uninstaller.'
} finally {
    Remove-Item -LiteralPath $registration,$uninstall,$innoKey -Recurse -Force -ErrorAction SilentlyContinue
}
