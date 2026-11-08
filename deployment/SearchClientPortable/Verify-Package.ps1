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
    'Инструкция_по_обновлению.txt',
    'app\sht.docm',
    'app\Settings.db3',
    'app\admin984.bin',
    'app\config\server-error-messages.json',
    'app\config\search-templates.defaults.json'
)) {
    $requiredPath = Join-Path $root $required
    if (-not (Test-Path -LiteralPath $requiredPath -PathType Leaf)) {
        throw "Required package file is missing: $required"
    }
}

$defaults = [IO.File]::ReadAllText((Join-Path $root 'app\config\search-templates.defaults.json'), [Text.Encoding]::UTF8) | ConvertFrom-Json
$groups = @($defaults.PSObject.Properties)
if ($groups.Count -ne 2) { throw 'Default templates must contain two direction groups.' }
$directions = @()
foreach ($group in $groups) {
    $templates = @($group.Value.PSObject.Properties)
    if ($templates.Count -ne 5) { throw 'Each default group must contain five periods.' }
    $periods = @()
    $types = @()
    foreach ($template in $templates) {
        $value = $template.Value
        if ($value.template_version -ne 2 -or $value.field -ne 'SQL' -or
            $value.search_text -ne '1=1' -or $value.sql -ne $true -or
            $value.date_field -ne 'DData' -or $value.max_results -ne 300 -or
            $value.newest_first -ne $true -or $value.filters_saved -ne $true -or
            @($value.column_filters.PSObject.Properties).Count -ne 0) {
            throw 'Unexpected default template parameters.'
        }
        $periods += [int]$value.period
        $types += [int]$value.type
    }
    if ((@($periods | Sort-Object -Unique) -join ',') -ne '1,2,3,4,5' -or
        @($types | Sort-Object -Unique).Count -ne 1) { throw 'Invalid default periods or direction.' }
    $directions += $types[0]
}
if ((@($directions | Sort-Object -Unique) -join ',') -ne '0,1') { throw 'Missing default direction.' }

$authArtifacts = @(Get-ChildItem -LiteralPath $root -Recurse -File -Force |
    Where-Object {
        $_.Name -match '(?i)^searchclient-auth-(token|request)\.json$'
    })
if ($authArtifacts.Count -gt 0) {
    throw (
        'Auth token/request must be stored outside the portable package. ' +
        'The PC token belongs in %LOCALAPPDATA%\SearchEngine: ' +
        $authArtifacts[0].FullName
    )
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
