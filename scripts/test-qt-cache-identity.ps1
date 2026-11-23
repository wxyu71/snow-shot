[CmdletBinding()]
param()
$ErrorActionPreference = 'Stop'

# Real build tools, isolated fake ABI records: no Qt download or build needed.
$root = Join-Path ([System.IO.Path]::GetTempPath()) (
    "snow-qt-cache-identity-$([guid]::NewGuid().ToString('N'))")
$previousOutput = $env:GITHUB_OUTPUT
try {
    foreach ($port in @('libpng', 'zlib')) {
        $dir = Join-Path $root "share/$port"
        New-Item -ItemType Directory -Path $dir -Force | Out-Null
        Set-Content -LiteralPath (Join-Path $dir 'vcpkg_abi_info.txt') -Value "$port=initial" -NoNewline
    }

    function Get-Identity {
        $outFile = Join-Path $root ([guid]::NewGuid().ToString('N') + '.txt')
        $env:GITHUB_OUTPUT = $outFile
        & (Join-Path $PSScriptRoot 'qt-cache-identity.ps1') -Architecture x64 -Configuration Release -DevelopmentModules -DependencyPrefix $root | Out-Null
        $entries = @(Get-Content -LiteralPath $outFile | Where-Object { $_ -match '^key=([0-9a-f]{64})$' })
        if ($entries.Count -ne 1) {
            throw "Expected one deterministic SHA256 Qt cache identity, got: $($entries -join ', ')"
        }
        return $entries[0].Substring(4)
    }

    $initial = Get-Identity
    if ((Get-Identity) -cne $initial) {
        throw 'Same inputs produced different Qt cache keys.'
    }

    Set-Content -LiteralPath (Join-Path $root 'share/libpng/vcpkg_abi_info.txt') -Value 'libpng=changed' -NoNewline
    $afterPngChange = Get-Identity
    if ($afterPngChange -ceq $initial) {
        throw 'Changing the installed libpng ABI did not invalidate Qt cache.'
    }

    Set-Content -LiteralPath (Join-Path $root 'share/zlib/vcpkg_abi_info.txt') -Value 'zlib=changed' -NoNewline
    if ((Get-Identity) -ceq $afterPngChange) {
        throw 'Changing the installed zlib ABI did not invalidate Qt cache.'
    }
    Write-Host 'Qt cache identity regression: PASS (stable inputs, libpng ABI, zlib ABI)'
}
finally {
    if ($null -eq $previousOutput) {
        Remove-Item Env:GITHUB_OUTPUT -ErrorAction SilentlyContinue
    } else {
        $env:GITHUB_OUTPUT = $previousOutput
    }
    Remove-Item -LiteralPath $root -Recurse -Force -ErrorAction SilentlyContinue
}
