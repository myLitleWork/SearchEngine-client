[CmdletBinding()]
param(
    [string]$BuildDirectory,
    [string]$OutputDirectory,
    [switch]$AllowDirty,
    [string]$CloudRoot,
    [string]$CloudReleaseId,
    [string]$ZipName,
    [switch]$SkipCloudPublish
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
$projectRoot = (Resolve-Path -LiteralPath (Join-Path $PSScriptRoot '..')).Path
$productName = 'SearchClient'
$getNames = @(
    (Join-Path $projectRoot '..\..\TOOLS\scripts\release\Get-VclAppVersionNames.ps1'),
    (Join-Path $projectRoot '..\..\..\TOOLS\scripts\release\Get-VclAppVersionNames.ps1')
) | ForEach-Object { [IO.Path]::GetFullPath($_) } |
    Where-Object { Test-Path -LiteralPath $_ -PathType Leaf } |
    Select-Object -First 1
if ($null -eq $getNames) { throw 'Get-VclAppVersionNames.ps1 not found.' }
$versionInfo = & $getNames -ProjectRoot $projectRoot -ProductName $productName
if ([string]::IsNullOrWhiteSpace($BuildDirectory)) { $BuildDirectory = Join-Path $projectRoot 'Win32\Release' }
if ([string]::IsNullOrWhiteSpace($OutputDirectory)) {
    $OutputDirectory = Join-Path $projectRoot (
        Join-Path 'out\package' (Join-Path $versionInfo.ReleaseId $versionInfo.OutputLeaf))
}
if ([string]::IsNullOrWhiteSpace($ZipName)) { $ZipName = $versionInfo.ZipFileName }
if ([string]::IsNullOrWhiteSpace($CloudReleaseId)) { $CloudReleaseId = $versionInfo.ReleaseId }
$BuildDirectory = [IO.Path]::GetFullPath($BuildDirectory)
$OutputDirectory = [IO.Path]::GetFullPath($OutputDirectory)
$outputParent = Split-Path -Parent $OutputDirectory
$markerName = '.searchclient-release-package'

function Get-Sha256([string]$Path) {
    $sha = [Security.Cryptography.SHA256]::Create()
    $inputStream = [IO.File]::OpenRead($Path)
    try {
        return ([BitConverter]::ToString($sha.ComputeHash($inputStream))).Replace('-', '')
    } finally {
        $inputStream.Dispose()
        $sha.Dispose()
    }
}

function Resolve-GitExe {
    $fromPath = Get-Command git -ErrorAction SilentlyContinue |
        Select-Object -ExpandProperty Source -First 1
    if (-not [string]::IsNullOrWhiteSpace($fromPath) -and
        (Test-Path -LiteralPath $fromPath -PathType Leaf)) {
        return $fromPath
    }

    foreach ($candidate in @(
        (Join-Path ${env:ProgramFiles} 'Git\cmd\git.exe'),
        (Join-Path ${env:ProgramFiles(x86)} 'Git\cmd\git.exe'),
        'C:\Program Files\Git\cmd\git.exe'
    )) {
        if (-not [string]::IsNullOrWhiteSpace($candidate) -and
            (Test-Path -LiteralPath $candidate -PathType Leaf)) {
            return $candidate
        }
    }

    throw (
        'git.exe was not found. Install Git for Windows or add it to PATH ' +
        'for the RAD Studio / MSBuild process.'
    )
}

function Test-BuildArtifact([IO.FileInfo]$File) {
    $ext = $File.Extension.ToLowerInvariant()
    $buildExt = @(
        '.obj', '.dcu', '.tds', '.pch', '.ilc', '.ild', '.ilf', '.ils',
        '.map', '.res', '.rsm', '.lib', '.bpi', '.dcp'
    )
    if ($buildExt -contains $ext) { return $true }
    if ($File.Name -like '*.obj-*') { return $true }
    if ($File.Name -like '*-journal') { return $true }
    if ($File.Name -like '*.PCH1.*') { return $true }
    if ($File.Name -like '*PCH1.pch') { return $true }
    return $false
}

$exe = Join-Path $BuildDirectory 'SearchClient_asio.exe'
if (-not (Test-Path -LiteralPath $exe -PathType Leaf)) { throw "Release executable is missing: $exe" }
$stream = [IO.File]::OpenRead($exe)
try {
    $reader = [IO.BinaryReader]::new($stream)
    if ($reader.ReadUInt16() -ne 0x5A4D) { throw 'SearchClient_asio.exe is not a PE file.' }
    $stream.Position = 0x3c
    $stream.Position = $reader.ReadInt32() + 4
    if ($reader.ReadUInt16() -ne 0x014c) { throw 'SearchClient_asio.exe is not Win32 x86.' }
} finally { $stream.Dispose() }

$gitExe = Resolve-GitExe
$gitStatus = @(& $gitExe -C $projectRoot status --porcelain --untracked-files=normal)
$dirty = $gitStatus.Count -gt 0
if ($dirty -and -not $AllowDirty) { throw 'Worktree is dirty. Use -AllowDirty for a local candidate package.' }

$payloadFiles = @(
    Get-ChildItem -LiteralPath $BuildDirectory -File |
        Where-Object { -not (Test-BuildArtifact $_) } |
        Sort-Object Name
)
if ($payloadFiles.Count -lt 1) { throw "No package payload files found in: $BuildDirectory" }

$templateRoot = Join-Path $projectRoot 'deployment\SearchClientPortable'
$tutorialRoot = Join-Path $projectRoot 'tutorials'
foreach ($required in @('Verify-Package.cmd', 'Verify-Package.ps1')) {
    $path = Join-Path $templateRoot $required
    if (-not (Test-Path -LiteralPath $path -PathType Leaf)) {
        throw "Package template is missing: $path"
    }
}

New-Item -ItemType Directory -Force -Path $outputParent | Out-Null
$staging = Join-Path $outputParent ('.SearchClient-Release.staging-' + [Guid]::NewGuid().ToString('N'))
$previous = $null
New-Item -ItemType Directory -Path (Join-Path $staging 'app') -Force | Out-Null
try {
    foreach ($file in $payloadFiles) {
        Copy-Item -LiteralPath $file.FullName -Destination (Join-Path (Join-Path $staging 'app') $file.Name) -Force
    }
    if (Test-Path -LiteralPath $tutorialRoot -PathType Container) {
        Copy-Item -LiteralPath $tutorialRoot -Destination (Join-Path $staging 'tutorials') -Recurse
    }
    Copy-Item -LiteralPath (Join-Path $templateRoot 'Verify-Package.cmd') -Destination $staging
    Copy-Item -LiteralPath (Join-Path $templateRoot 'Verify-Package.ps1') -Destination $staging

    $files = @(Get-ChildItem -LiteralPath $staging -Recurse -File | Sort-Object FullName)
    $manifest = [ordered]@{
        formatVersion = 1; product = 'SearchClient'; architecture = 'Win32';
        commit = (& $gitExe -C $projectRoot rev-parse HEAD).Trim(); dirty = $dirty;
        createdUtc = (Get-Date).ToUniversalTime().ToString('o');
        entryPoint = 'app/SearchClient_asio.exe';
        excluded = @('compiler intermediates (.obj/.tds/.pch/.il*)', '*-journal');
        files = @($files | ForEach-Object {
            [ordered]@{ path = $_.FullName.Substring($staging.Length + 1).Replace('\','/'); size = $_.Length; sha256 = Get-Sha256 $_.FullName }
        })
    }
    [IO.File]::WriteAllText((Join-Path $staging 'package-manifest.json'), ($manifest | ConvertTo-Json -Depth 6), [Text.UTF8Encoding]::new($false))
    $hashFiles = @(Get-ChildItem -LiteralPath $staging -Recurse -File | Where-Object Name -ne 'package-checksums.sha256' | Sort-Object FullName)
    $lines = @($hashFiles | ForEach-Object { '{0}  {1}' -f (Get-Sha256 $_.FullName).ToLowerInvariant(), $_.FullName.Substring($staging.Length + 1) })
    [IO.File]::WriteAllLines((Join-Path $staging 'package-checksums.sha256'), $lines, [Text.UTF8Encoding]::new($false))
    [IO.File]::WriteAllText((Join-Path $staging $markerName), "Generated by New-SearchClientPackage.ps1`r`n", [Text.UTF8Encoding]::new($false))

    if (Test-Path -LiteralPath $OutputDirectory) {
        if (-not (Test-Path -LiteralPath (Join-Path $OutputDirectory $markerName) -PathType Leaf)) { throw "Existing output was not generated by this packager: $OutputDirectory" }
        $previous = Join-Path $outputParent ('.SearchClient-Release.previous-' + [Guid]::NewGuid().ToString('N'))
        Move-Item -LiteralPath $OutputDirectory -Destination $previous
    }
    Move-Item -LiteralPath $staging -Destination $OutputDirectory
    if ($previous) { Remove-Item -LiteralPath $previous -Recurse -Force }
} finally {
    if (Test-Path -LiteralPath $staging) { Remove-Item -LiteralPath $staging -Recurse -Force }
}

try {
    & (Join-Path $OutputDirectory 'Verify-Package.ps1') -Quiet
}
catch {
    throw "Packaged verifier failed: $($_.Exception.Message)"
}

$cloudHelper = @(
    (Join-Path $projectRoot '..\..\TOOLS\scripts\release\Publish-ReleasePackageIfConfigured.ps1'),
    (Join-Path $projectRoot '..\..\..\TOOLS\scripts\release\Publish-ReleasePackageIfConfigured.ps1')
) | ForEach-Object { [IO.Path]::GetFullPath($_) } |
    Where-Object { Test-Path -LiteralPath $_ -PathType Leaf } |
    Select-Object -First 1

$cloudPackage = $null
if ($null -ne $cloudHelper) {
    $cloudArgs = @{
        PackageDirectory = $OutputDirectory
        ProductName = $productName
        ReleaseId = $CloudReleaseId
        ZipName = $ZipName
        StartPath = $projectRoot
        SkipCloudPublish = $SkipCloudPublish
        # Stage policy: include local Settings/DB/INI snapshot in Drive ZIP.
        AllowSensitive = $true
    }
    if (-not [string]::IsNullOrWhiteSpace($CloudRoot)) {
        $cloudArgs.CloudRoot = $CloudRoot
    }
    $cloudPackage = & $cloudHelper @cloudArgs
}
else {
    Write-Host 'Cloud publish helper not found; local package only.'
}

[pscustomobject]@{
    Product = 'SearchClient'
    OutputDirectory = $OutputDirectory
    Dirty = $dirty
    PayloadFileCount = $payloadFiles.Count
    CloudZipPath = $(if ($null -ne $cloudPackage) { $cloudPackage.ZipPath } else { $null })
}
