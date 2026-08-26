[CmdletBinding()]
param([switch]$Quiet)

$ErrorActionPreference = 'Stop'
$root = $PSScriptRoot
$checksums = Join-Path $root 'package-checksums.sha256'

function Get-Sha256([string]$Path) {
    $sha = [Security.Cryptography.SHA256]::Create()
    $stream = [IO.File]::OpenRead($Path)
    try {
        return ([BitConverter]::ToString($sha.ComputeHash($stream))).Replace('-', '').ToLowerInvariant()
    } finally {
        $stream.Dispose()
        $sha.Dispose()
    }
}

if (-not (Test-Path -LiteralPath $checksums -PathType Leaf)) {
    throw "Checksum file is missing: $checksums"
}

foreach ($required in @(
    'app\SearchClient_asio.exe',
    'app\sht.docm',
    'app\Settings.db3',
    'app\admin984.bin',
    'app\config\server-error-messages.json'
)) {
    $requiredPath = Join-Path $root $required
    if (-not (Test-Path -LiteralPath $requiredPath -PathType Leaf)) {
        throw "Required package file is missing: $required"
    }
}

foreach ($line in Get-Content -LiteralPath $checksums -Encoding UTF8) {
    if ($line -notmatch '^([0-9a-f]{64})  (.+)$') {
        throw "Invalid checksum line: $line"
    }
    $path = Join-Path $root $Matches[2]
    if (-not (Test-Path -LiteralPath $path -PathType Leaf)) {
        throw "Package file is missing: $($Matches[2])"
    }
    $actual = Get-Sha256 $path
    if ($actual -ne $Matches[1]) { throw "Checksum mismatch: $($Matches[2])" }
}

if (-not $Quiet) { Write-Host 'SearchClient package verification passed.' }
