[CmdletBinding()]
param(
    [switch]$BumpPatch,
    [switch]$ForceBumpPatch
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

$project = (Resolve-Path -LiteralPath (Join-Path $PSScriptRoot '..')).Path
$versionFile = Join-Path $project 'app-version.json'
$versionHeader = Join-Path $project 'Version.h'
$resourceFile = Join-Path $project 'SearchClientVersion.rc'
$productName = 'SearchClient'

$resolveScript = @(
    (Join-Path $project '..\..\TOOLS\scripts\release\Resolve-WorkspaceReleaseScript.ps1'),
    (Join-Path $project '..\..\..\TOOLS\scripts\release\Resolve-WorkspaceReleaseScript.ps1')
) | ForEach-Object { [IO.Path]::GetFullPath($_) } |
    Where-Object { Test-Path -LiteralPath $_ -PathType Leaf } |
    Select-Object -First 1
if ($null -eq $resolveScript) { throw 'Resolve-WorkspaceReleaseScript.ps1 not found.' }
$namesScript = & $resolveScript -ScriptName 'Get-VclAppVersionNames.ps1' -StartPath $project

function Read-TextFilePreservingEncoding {
    param([Parameter(Mandatory)][string]$Path)
    $bytes = [IO.File]::ReadAllBytes($Path)
    $hasBom = $bytes.Length -ge 3 -and $bytes[0] -eq 0xEF -and $bytes[1] -eq 0xBB -and $bytes[2] -eq 0xBF
    $encoding = [Text.UTF8Encoding]::new($hasBom)
    $text = $encoding.GetString($bytes)
    if ($hasBom -and $text.Length -gt 0 -and [int][char]$text[0] -eq 0xFEFF) { $text = $text.Substring(1) }
    $newline = if ($text -match "`r`n") { "`r`n" } elseif ($text -match "`r") { "`r" } else { "`n" }
    [pscustomobject]@{ Text = $text; Encoding = $encoding; Newline = $newline }
}

function Get-ReleaseInputSha256 {
    param([Parameter(Mandatory)][string]$ProjectRoot)
    # The raw .cbproj is deliberately excluded: the IDE may rewrite equivalent XML.
    # Release-only project setting changes therefore require -ForceBumpPatch.
    $sourceExtensions = @('.c', '.cc', '.cpp', '.cxx', '.h', '.hpp', '.pas', '.dfm', '.res', '.rc', '.ico')
    $scriptExtensions = @('.bat', '.cmd', '.ps1')
    $tutorialExtensions = @('.htm', '.html', '.jpg', '.jpeg', '.md', '.pdf', '.png', '.txt')
    $sourceDirectories = @('BinaryFileLoad', 'Database', 'Frames', 'Message', 'MyStrings', 'my_json', 'Net', 'WordServer')
    $templateFiles = @(
        'deployment/SearchClientPortable/Verify-Package.cmd',
        'deployment/SearchClientPortable/Verify-Package.ps1',
        'deployment/SearchClientPortable/app/sht.docm',
        'deployment/SearchClientPortable/app/Settings.db3',
        'deployment/SearchClientPortable/app/admin984.bin'
    )
    $inputs = @()
    $inputs += Get-ChildItem -LiteralPath $ProjectRoot -File | Where-Object {
        $_.Name -notin @('Version.h', 'SearchClientVersion.rc') -and
        $sourceExtensions -contains $_.Extension.ToLowerInvariant()
    }
    foreach ($directory in $sourceDirectories) {
        $root = Join-Path $ProjectRoot $directory
        if (Test-Path -LiteralPath $root -PathType Container) {
            $inputs += Get-ChildItem -LiteralPath $root -File -Recurse | Where-Object {
                $sourceExtensions -contains $_.Extension.ToLowerInvariant()
            }
        }
    }
    foreach ($directory in @('scripts', 'tools')) {
        $root = Join-Path $ProjectRoot $directory
        if (Test-Path -LiteralPath $root -PathType Container) {
            $inputs += Get-ChildItem -LiteralPath $root -File -Recurse | Where-Object {
                $scriptExtensions -contains $_.Extension.ToLowerInvariant()
            }
        }
    }
    $tutorialRoot = Join-Path $ProjectRoot 'tutorials'
    if (Test-Path -LiteralPath $tutorialRoot -PathType Container) {
        $inputs += Get-ChildItem -LiteralPath $tutorialRoot -File -Recurse | Where-Object {
            $tutorialExtensions -contains $_.Extension.ToLowerInvariant()
        }
    }
    foreach ($relative in $templateFiles) {
        $path = Join-Path $ProjectRoot $relative
        if (Test-Path -LiteralPath $path -PathType Leaf) {
            $inputs += Get-Item -LiteralPath $path
        }
    }
    $inputs = @($inputs | Sort-Object { $_.FullName.Substring($ProjectRoot.Length).Replace('\', '/').ToLowerInvariant() } -Unique)
    $sha256 = [Security.Cryptography.SHA256]::Create()
    try {
        foreach ($input in $inputs) {
            $relative = $input.FullName.Substring($ProjectRoot.Length).TrimStart([IO.Path]::DirectorySeparatorChar, [IO.Path]::AltDirectorySeparatorChar).Replace('\', '/')
            $pathBytes = [Text.Encoding]::UTF8.GetBytes($relative.ToLowerInvariant())
            $separator = [byte[]]@(0)
            $lengthBytes = [BitConverter]::GetBytes([Int64]$input.Length)
            $null = $sha256.TransformBlock($pathBytes, 0, $pathBytes.Length, $null, 0)
            $null = $sha256.TransformBlock($separator, 0, $separator.Length, $null, 0)
            $null = $sha256.TransformBlock($lengthBytes, 0, $lengthBytes.Length, $null, 0)
            $content = [IO.File]::ReadAllBytes($input.FullName)
            $null = $sha256.TransformBlock($content, 0, $content.Length, $null, 0)
        }
        $null = $sha256.TransformFinalBlock([byte[]]@(), 0, 0)
        return ([BitConverter]::ToString($sha256.Hash).Replace('-', '')).ToLowerInvariant()
    } finally { $sha256.Dispose() }
}

if ($BumpPatch -and $ForceBumpPatch) { throw 'Use either -BumpPatch or -ForceBumpPatch, not both.' }
$releaseInputSha256 = Get-ReleaseInputSha256 -ProjectRoot $project
$file = Read-TextFilePreservingEncoding -Path $versionFile
$formatMatch = [regex]::Match($file.Text, '(?m)"releaseFingerprintFormat"\s*:\s*(\d+)')
$shaMatch = [regex]::Match($file.Text, '(?m)"releaseInputSha256"\s*:\s*"([0-9A-Fa-f]{64})"')
$fingerprintChanged = -not ($formatMatch.Success -and $formatMatch.Groups[1].Value -eq '2' -and $shaMatch.Success -and $shaMatch.Groups[1].Value.ToLowerInvariant() -eq $releaseInputSha256)
$bumped = $ForceBumpPatch -or ($BumpPatch -and $fingerprintChanged)
$updated = $file.Text
if ($bumped) {
    $patchMatch = [regex]::Match($updated, '(?m)("patch"\s*:\s*)(\d+)')
    if (-not $patchMatch.Success) { throw "app-version.json has no numeric patch: $versionFile" }
    $updated = [regex]::Replace($updated, '(?m)("patch"\s*:\s*)\d+', ('${1}' + ([int]$patchMatch.Groups[2].Value + 1)), 1)
}
if ($formatMatch.Success) { $updated = [regex]::Replace($updated, '(?m)("releaseFingerprintFormat"\s*:\s*)\d+', '${1}2', 1) }
else { $updated = [regex]::Replace($updated, '(?m)("formatVersion"\s*:\s*\d+\s*,)', ('$1' + $file.Newline + '  "releaseFingerprintFormat": 2,'), 1) }
if ($shaMatch.Success) { $updated = [regex]::Replace($updated, '(?m)("releaseInputSha256"\s*:\s*")[^"]*(")', ('${1}' + $releaseInputSha256 + '${2}'), 1) }
else { $updated = [regex]::Replace($updated, '(?m)("releaseFingerprintFormat"\s*:\s*2\s*,)', ('$1' + $file.Newline + '  "releaseInputSha256": "' + $releaseInputSha256 + '",'), 1) }
if ($updated -ne $file.Text) {
    $normalized = $updated -replace "`r`n", "`n" -replace "`r", "`n"
    [IO.File]::WriteAllText($versionFile, ($normalized -replace "`n", $file.Newline), $file.Encoding)
}

$versionInfo = & $namesScript -ProjectRoot $project -ProductName $productName
if ($versionInfo.Version -notmatch '^(?<major>\d+)\.(?<minor>\d+)\.(?<patch>\d+)$') { throw "Invalid version: $($versionInfo.Version)" }
$versionDocument = $file.Text | ConvertFrom-Json
$titleTemplate = [string]$versionDocument.titleTemplate
$resourceText = @"
// Generated by scripts\Sync-SearchClientVersion.ps1. Do not edit manually.
#include <windows.h>

1 VERSIONINFO
FILEVERSION $($Matches.major),$($Matches.minor),$($Matches.patch),0
PRODUCTVERSION $($Matches.major),$($Matches.minor),$($Matches.patch),0
FILEFLAGSMASK 0x3fL
FILEFLAGS 0x0L
FILEOS 0x00040004L
FILETYPE 0x1L
FILESUBTYPE 0x0L
BEGIN
    BLOCK "StringFileInfo"
    BEGIN
        BLOCK "040904E4"
        BEGIN
            VALUE "FileVersion", "$($versionInfo.FileVersion)\0"
            VALUE "ProductVersion", "$($versionInfo.Version)\0"
        END
    END
    BLOCK "VarFileInfo"
    BEGIN
        VALUE "Translation", 0x0409, 1200
    END
END
"@
$resourceBytes = [Text.UTF8Encoding]::new($false).GetBytes($resourceText + [Environment]::NewLine)
$writeResource = -not (Test-Path -LiteralPath $resourceFile -PathType Leaf) -or -not [Linq.Enumerable]::SequenceEqual([IO.File]::ReadAllBytes($resourceFile), $resourceBytes)
if ($writeResource) { [IO.File]::WriteAllBytes($resourceFile, $resourceBytes) }

# Version.h remains a compatibility projection for code outside the main form.
# Keep its existing encoding and line endings, and avoid a write when it is current.
if (Test-Path -LiteralPath $versionHeader -PathType Leaf) {
    $header = Read-TextFilePreservingEncoding -Path $versionHeader
    $headerText = $header.Text
    $headerText = [regex]::Replace($headerText, '(?m)(const int Major\s*=\s*)\d+;', ('${1}' + $Matches.major + ';'), 1)
    $headerText = [regex]::Replace($headerText, '(?m)(const int Minor\s*=\s*)\d+;', ('${1}' + $Matches.minor + ';'), 1)
    $headerText = [regex]::Replace($headerText, '(?m)(const int Patch\s*=\s*)\d+;', ('${1}' + $Matches.patch + ';'), 1)
    $headerText = [regex]::Replace($headerText, '(?m)(return L")[^"]*(";)', ('${1}' + $titleTemplate + '${2}'), 1)
    if ($headerText -ne $header.Text) {
        $normalized = $headerText -replace "`r`n", "`n" -replace "`r", "`n"
        [IO.File]::WriteAllText($versionHeader, ($normalized -replace "`n", $header.Newline), $header.Encoding)
    }
}
Write-Host "Synced SearchClient VERSIONINFO resource: $resourceFile ($($versionInfo.FileVersion))."
[pscustomobject]@{ ProjectRoot=$project; ProductName=$productName; Version=$versionInfo.Version; FileVersion=$versionInfo.FileVersion; ReleaseId=$versionInfo.ReleaseId; ZipFileName=$versionInfo.ZipFileName; OutputLeaf=$versionInfo.OutputLeaf; ResourceFile=$resourceFile; ReleaseFingerprintFormat=2; ReleaseInputSha256=$releaseInputSha256; Bumped=[bool]$bumped }
