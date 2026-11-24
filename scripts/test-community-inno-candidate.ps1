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
$badStage = Join-Path $base 'bad-stage'
$badBuild = Join-Path $base 'bad-build'
$badOutput = Join-Path $base 'bad-candidate'
$registration = 'HKCU:\Software\Snow Shot Community\SnowShot'
$uninstall = 'HKCU:\Software\Microsoft\Windows\CurrentVersion\Uninstall\SnowShot'
$innoKey = 'HKCU:\Software\Microsoft\Windows\CurrentVersion\Uninstall\SnowShot_is1'
foreach ($key in @($registration,$uninstall,$innoKey)) {
    if (Test-Path -LiteralPath $key) { throw 'Refusing to touch existing installation registry data.' }
}
New-Item -ItemType Directory -Force -Path "$stage\bin","$install\bin","$badStage\bin",$build,$badBuild | Out-Null
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
    Copy-Item -LiteralPath "$stage\bin\snow-shot-updater.exe" -Destination "$badStage\bin\snow-shot-updater.exe"
    'fn main() { std::process::exit(42); }' | Set-Content -LiteralPath "$base\bad-probe.rs"
    rustc "$base\bad-probe.rs" -o "$badStage\bin\snow_shot.exe"
    if ($LASTEXITCODE -ne 0) { throw 'Failing startup probe fixture build failed.' }
    'fn main() { std::thread::sleep(std::time::Duration::from_secs(180)); }' | Set-Content -LiteralPath "$base\old-app.rs"
    rustc "$base\old-app.rs" -o "$install\bin\snow_shot.exe"
    if ($LASTEXITCODE -ne 0) { throw 'Running legacy app fixture build failed.' }
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
(old / "bin/snow-shot-updater.exe").write_bytes(b"old helper")
(old / "bin/obsolete.dll").write_bytes(b"old owned file")
save(old, "0.0.1-beta.1")
(old / "bin/portable").mkdir()
(old / "bin/portable/user-data.txt").write_text("keep me", encoding="utf-8")
(old / "Uninstall.exe").write_bytes(b"must never execute or remove this sentinel")
def package(stage, build):
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
package(stage, build)
package(root / "bad-stage", root / "bad-build")
'@ | python -
    if ($LASTEXITCODE -ne 0) { throw 'Fixture staging failed.' }
    & ./scripts/build-community-inno-candidate.ps1 -Compiler $env:SNOW_INNO_COMPILER -BuildDirectory $build -StageDirectory $stage -Variant online -OutputDirectory $output
    New-Item -Path $registration -Force | Out-Null
    Set-Item -LiteralPath $registration -Value $install
    New-Item -Path $uninstall -Force | Out-Null
    New-ItemProperty -LiteralPath $uninstall -Name DisplayVersion -Value '0.0.1-beta.1' | Out-Null
    New-ItemProperty -LiteralPath $uninstall -Name UninstallString -Value ('"' + "$install\Uninstall.exe" + '"') | Out-Null
    # A real setup EXE must roll back after replacing files and updating the registry,
    # not merely reject a malformed archive before the transaction starts.
    & ./scripts/build-community-inno-candidate.ps1 -Compiler $env:SNOW_INNO_COMPILER -BuildDirectory $badBuild -StageDirectory $badStage -Variant online -OutputDirectory $badOutput
    $oldHashes = @{}
    foreach ($file in (Get-ChildItem -LiteralPath $install -Recurse -File)) {
        $oldHashes[$file.FullName] = (Get-FileHash -LiteralPath $file.FullName -Algorithm SHA256).Hash
    }
    $exe = Join-Path $output "snow-shot-$version-windows-x64-online.exe"
    $running = Start-Process -FilePath "$install\bin\snow_shot.exe" -WindowStyle Hidden -PassThru
    try {
        Start-Sleep -Milliseconds 500
        if ($running.HasExited) { throw 'Legacy app fixture exited before the test.' }
        $blocked = Start-Process -FilePath $exe -ArgumentList @('/VERYSILENT','/SUPPRESSMSGBOXES','/NORESTART','/CURRENTUSER',('/LOG="' + "$base\running.log" + '"')) -Wait -PassThru
        if ($blocked.ExitCode -eq 0 -or $running.HasExited -or
            (Get-ItemProperty $uninstall).DisplayVersion -ne '0.0.1-beta.1' -or
            (Test-Path -LiteralPath "$install\.snow-shot-update\journal.json")) {
            Get-Content "$base\running.log" -ErrorAction SilentlyContinue
            throw 'Setup must reject a running application before changing files or stopping it.'
        }
        foreach ($file in $oldHashes.Keys) {
            if (-not (Test-Path -LiteralPath $file -PathType Leaf) -or
                (Get-FileHash -LiteralPath $file -Algorithm SHA256).Hash -ne $oldHashes[$file]) {
                throw "Running application check changed original bytes: $file"
            }
        }
    } finally {
        # Only stop the test process created above; never terminate a real user app.
        if (-not $running.HasExited) { $running.Kill(); $running.WaitForExit() }
        $running.Dispose()
    }
    Write-Output 'Running legacy application blocked upgrade without being terminated.'
    $badExe = Join-Path $badOutput "snow-shot-$version-windows-x64-online.exe"
    $failed = Start-Process -FilePath $badExe -ArgumentList @('/VERYSILENT','/SUPPRESSMSGBOXES','/NORESTART','/CURRENTUSER',('/LOG="' + "$base\rollback.log" + '"')) -Wait -PassThru
    if ($failed.ExitCode -eq 0) { throw 'Setup unexpectedly accepted a failing startup probe.' }
    $failedVersion = Join-Path $install '.snow-shot-update\failed-version.txt'
    if (-not (Test-Path -LiteralPath $failedVersion) -or
        (Get-Content -LiteralPath $failedVersion -Raw) -ne $version -or
        (Get-ItemProperty $uninstall).DisplayVersion -ne '0.0.1-beta.1' -or
        (Test-Path $innoKey) -or
        (Test-Path -LiteralPath "$install\.snow-shot-update\journal.json")) {
        Get-Content "$base\rollback.log" -ErrorAction SilentlyContinue
        throw 'The failed startup probe did not finish a rollback of the legacy installation.'
    }
    foreach ($file in $oldHashes.Keys) {
        if (-not (Test-Path -LiteralPath $file -PathType Leaf) -or
            (Get-FileHash -LiteralPath $file -Algorithm SHA256).Hash -ne $oldHashes[$file]) {
            throw "Rollback failed to restore original bytes: $file"
        }
    }
    Write-Output 'Failed startup probe rolled back the original files and registry version.'
    $p = Start-Process -FilePath $exe -ArgumentList @('/VERYSILENT','/SUPPRESSMSGBOXES','/NORESTART','/CURRENTUSER',('/LOG="' + "$base\upgrade.log" + '"')) -Wait -PassThru
    if ($p.ExitCode -ne 0) { Get-Content "$base\upgrade.log" -ErrorAction SilentlyContinue; throw "Candidate upgrade failed: $($p.ExitCode)" }
    $record = Get-Content "$install\snow-shot-installation.json" -Raw | ConvertFrom-Json
    if ($record.version -ne $version -or (Get-ItemProperty $uninstall).DisplayVersion -ne $version -or
        (Test-Path "$install\bin\obsolete.dll") -or (Test-Path $innoKey) -or
        (Test-Path -LiteralPath $failedVersion) -or
        (Test-Path -LiteralPath "$install\.snow-shot-update\journal.json") -or
        (Get-Content "$install\bin\portable\user-data.txt" -Raw) -ne 'keep me' -or
        (Get-Content "$install\Uninstall.exe" -Raw) -ne 'must never execute or remove this sentinel' -or
        (Get-FileHash "$install\bin\snow_shot.exe").Hash -ne (Get-FileHash "$stage\bin\snow_shot.exe").Hash) {
        throw 'In-place upgrade violated file, user-data or registry ownership contract.'
    }
    Write-Output 'Synthetic legacy registration upgraded without running its uninstaller.'
} finally {
    Remove-Item -LiteralPath $registration,$uninstall,$innoKey -Recurse -Force -ErrorAction SilentlyContinue
}
