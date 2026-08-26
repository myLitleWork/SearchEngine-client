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
$getNames = & (Join-Path $PSScriptRoot 'Find-WorkspaceReleaseRoot.ps1') `
    -Name 'Get-VclAppVersionNames.ps1' -StartPath $projectRoot
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

$expectedFileVersion = $versionInfo.FileVersion
$expectedProductVersion = $versionInfo.Version
$peVersion = [Diagnostics.FileVersionInfo]::GetVersionInfo($exe)
if ($peVersion.FileVersion -ne $expectedFileVersion) {
    throw "Release executable FileVersion mismatch: expected $expectedFileVersion, got $($peVersion.FileVersion)."
}
if ($peVersion.ProductVersion -ne $expectedProductVersion) {
    throw "Release executable ProductVersion mismatch: expected $expectedProductVersion, got $($peVersion.ProductVersion)."
}

$gitExe = Resolve-GitExe
$gitStatus = @(& $gitExe -C $projectRoot status --porcelain --untracked-files=normal)
$dirty = $gitStatus.Count -gt 0
if ($dirty -and -not $AllowDirty) { throw 'Worktree is dirty. Use -AllowDirty for a local candidate package.' }

$templateRoot = Join-Path $projectRoot 'deployment\SearchClientPortable'
$templateAppRoot = Join-Path $templateRoot 'app'
$tutorialRoot = Join-Path $projectRoot 'tutorials'
foreach ($required in @('Verify-Package.cmd', 'Verify-Package.ps1')) {
    $path = Join-Path $templateRoot $required
    if (-not (Test-Path -LiteralPath $path -PathType Leaf)) {
        throw "Package template is missing: $path"
    }
}

# Release contents are intentionally explicit. Volatile runtime databases, local
# server addresses, credentials and compiler output must never enter a package just
# because they happen to exist beside the executable. The sanitized Settings.db3
# seed and the non-secret admin marker are copied from the package template, not
# from a developer's runtime folder.
$requiredBuildPayloadNames = @(
    'SearchClient_asio.exe'
)
$payloadFiles = @(
    foreach ($name in $requiredBuildPayloadNames) {
        $path = Join-Path $BuildDirectory $name
        if (-not (Test-Path -LiteralPath $path -PathType Leaf)) {
            throw "Required release payload is missing: $path"
        }
        Get-Item -LiteralPath $path
    }
)

$requiredTemplatePayloadNames = @(
    'sht.docm',
    'Settings.db3',
    'admin984.bin'
)
$errorMessagesPath = Join-Path $projectRoot 'config\server-error-messages.json'
if (-not (Test-Path -LiteralPath $errorMessagesPath -PathType Leaf)) { throw "Required error message map is missing: $errorMessagesPath" }
$requiredTemplatePayloadHashes = @{
    'Settings.db3' = '40A4D999B39B0FE4C18649DB71B7EC60222E8DDF28E2F059E24897B3032F0B13'
    'sht.docm' = '85EB429F9BD08ACE7127729887520EE7EFC34AA348C290ACC6EE818F438B938F'
}
$templatePayloadFiles = @(
    foreach ($name in $requiredTemplatePayloadNames) {
        $path = Join-Path $templateAppRoot $name
        if (-not (Test-Path -LiteralPath $path -PathType Leaf)) {
            throw "Required package template payload is missing: $path"
        }
        if ($requiredTemplatePayloadHashes.ContainsKey($name)) {
            $actualHash = Get-Sha256 $path
            if ($actualHash -ne $requiredTemplatePayloadHashes[$name]) {
                throw "Package template payload has an unexpected SHA-256: $path"
            }
        }
        Get-Item -LiteralPath $path
    }
)

New-Item -ItemType Directory -Force -Path $outputParent | Out-Null
$staging = Join-Path $outputParent ('.SearchClient-Release.staging-' + [Guid]::NewGuid().ToString('N'))
$previous = $null
New-Item -ItemType Directory -Path (Join-Path $staging 'app') -Force | Out-Null
try {
    foreach ($file in $payloadFiles) {
        Copy-Item -LiteralPath $file.FullName -Destination (Join-Path (Join-Path $staging 'app') $file.Name) -Force
    }
    foreach ($file in $templatePayloadFiles) {
        Copy-Item -LiteralPath $file.FullName -Destination (Join-Path (Join-Path $staging 'app') $file.Name) -Force
    }
    New-Item -ItemType Directory -Force -Path (Join-Path $staging 'app\config') | Out-Null
    Copy-Item -LiteralPath $errorMessagesPath -Destination (Join-Path $staging 'app\config\server-error-messages.json') -Force
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
        excluded = @(
            'all files not present in the explicit payload allowlist',
            'compiler intermediates and journals',
            'SearchRes.db3, Templates.json and SERVER_IP.ini',
            'signed tokens, public/private keys and other local credentials'
        );
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

$cloudHelper = & (Join-Path $PSScriptRoot 'Find-WorkspaceReleaseRoot.ps1') `
    -Name 'Publish-ReleasePackageIfConfigured.ps1' -StartPath $projectRoot `
    -Optional

$cloudPackage = $null
$configuredCloudRoot = $CloudRoot
if ([string]::IsNullOrWhiteSpace($configuredCloudRoot)) {
    $configuredCloudRoot = $env:WORKSPACE_RELEASE_CLOUD_ROOT
}
if ([string]::IsNullOrWhiteSpace($configuredCloudRoot)) {
    $configuredCloudRoot = [Environment]::GetEnvironmentVariable('WORKSPACE_RELEASE_CLOUD_ROOT', 'User')
}
$cloudPublishRequired = -not $SkipCloudPublish -and -not [string]::IsNullOrWhiteSpace($configuredCloudRoot)
if ($null -ne $cloudHelper) {
    $cloudArgs = @{
        PackageDirectory = $OutputDirectory
        ProductName = $productName
        ReleaseId = $CloudReleaseId
        ZipName = $ZipName
        StartPath = $projectRoot
        SkipCloudPublish = $SkipCloudPublish
        # admin984.bin intentionally enables the temporary admin mode.
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

if ($cloudPublishRequired) {
    if ($null -eq $cloudPackage) {
        throw 'Cloud publish returned no result for the configured cloud root.'
    }
    if ([string]::IsNullOrWhiteSpace([string]$cloudPackage.ZipPath) -or
        -not (Test-Path -LiteralPath $cloudPackage.ZipPath -PathType Leaf)) {
        throw "Cloud ZIP is missing: $($cloudPackage.ZipPath)"
    }
    if ([string]::IsNullOrWhiteSpace([string]$cloudPackage.Sha256Path) -or
        -not (Test-Path -LiteralPath $cloudPackage.Sha256Path -PathType Leaf)) {
        throw "Cloud SHA-256 sidecar is missing: $($cloudPackage.Sha256Path)"
    }
}

$packagedExe = Join-Path $OutputDirectory 'app\SearchClient_asio.exe'
if (-not (Test-Path -LiteralPath $packagedExe -PathType Leaf)) {
    throw "Packaged executable is missing: $packagedExe"
}
$packagedVersion = [Diagnostics.FileVersionInfo]::GetVersionInfo($packagedExe)
if ($packagedVersion.FileVersion -ne $versionInfo.FileVersion -or
    $packagedVersion.ProductVersion -ne $versionInfo.Version) {
    throw (
        "Packaged executable version mismatch: expected FileVersion=$($versionInfo.FileVersion), " +
        "ProductVersion=$($versionInfo.Version); actual FileVersion=$($packagedVersion.FileVersion), " +
        "ProductVersion=$($packagedVersion.ProductVersion)."
    )
}

[pscustomobject]@{
    Product = $productName
    Version = $versionInfo.Version
    FileVersion = $versionInfo.FileVersion
    PackageDirectory = $OutputDirectory
    OutputDirectory = $OutputDirectory
    PackagedExe = $packagedExe
    Dirty = $dirty
    PayloadFileCount = $payloadFiles.Count + $templatePayloadFiles.Count
    CloudPublished = $null -ne $cloudPackage
    ZipPath = $(if ($null -ne $cloudPackage) { $cloudPackage.ZipPath } else { $null })
    Sha256Path = $(if ($null -ne $cloudPackage) { $cloudPackage.Sha256Path } else { $null })
    Sha256 = $(if ($null -ne $cloudPackage) { $cloudPackage.Sha256 } else { $null })
}
