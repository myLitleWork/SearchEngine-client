[CmdletBinding()]
param(
    [switch]$AllowDirty,
    [switch]$SkipVersionBump,
    [switch]$SkipCloudPublish
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
$projectRoot = (Resolve-Path -LiteralPath (Join-Path $PSScriptRoot '..')).Path
$projectFile = Join-Path $projectRoot 'SearchClient_asio.cbproj'
$rsvarsBat = 'C:\Program Files (x86)\Embarcadero\Studio\22.0\bin\rsvars.bat'
$syncScript = Join-Path $projectRoot 'scripts\Sync-SearchClientVersion.ps1'
$packageScript = Join-Path $PSScriptRoot 'New-SearchClientPackage.ps1'

function Find-RequiredReleaseHelper {
    param([Parameter(Mandatory)][string]$Name)

    $candidates = @(
        (Join-Path $projectRoot "..\..\TOOLS\scripts\release\$Name"),
        (Join-Path $projectRoot "..\..\..\TOOLS\scripts\release\$Name")
    ) | ForEach-Object { [IO.Path]::GetFullPath($_) }

    $helper = $candidates | Where-Object { Test-Path -LiteralPath $_ -PathType Leaf } | Select-Object -First 1
    if ($null -eq $helper) {
        throw "Required local release helper not found: $Name"
    }
    return $helper
}

# Complete every quick, non-mutating environment check before version sync can
# modify app-version.json or its generated projections.
foreach ($required in @(
    @{ Label = 'rsvars.bat'; Path = $rsvarsBat },
    @{ Label = 'SearchClient_asio.cbproj'; Path = $projectFile },
    @{ Label = 'Sync-SearchClientVersion.ps1'; Path = $syncScript },
    @{ Label = 'New-SearchClientPackage.ps1'; Path = $packageScript }
)) {
    if (-not (Test-Path -LiteralPath $required.Path -PathType Leaf)) {
        throw "Required release file not found: $($required.Label): $($required.Path)"
    }
}
foreach ($helperName in @(
    'Resolve-WorkspaceReleaseScript.ps1',
    'Get-VclAppVersionNames.ps1',
    'Publish-ReleasePackageIfConfigured.ps1'
)) {
    $null = Find-RequiredReleaseHelper -Name $helperName
}
$releaseLockHelper = Find-RequiredReleaseHelper -Name 'Enter-WorkspaceReleaseLock.ps1'
foreach ($relative in @(
    'deployment\SearchClientPortable\Verify-Package.cmd',
    'deployment\SearchClientPortable\Verify-Package.ps1',
    'deployment\SearchClientPortable\app\sht.docm',
    'deployment\SearchClientPortable\app\Settings.db3',
    'deployment\SearchClientPortable\app\admin984.bin',
    'config\server-error-messages.json'
)) {
    $path = Join-Path $projectRoot $relative
    if (-not (Test-Path -LiteralPath $path -PathType Leaf)) {
        throw "Required package input not found: $path"
    }
}
$configuredCloudRoot = $env:WORKSPACE_RELEASE_CLOUD_ROOT
if ([string]::IsNullOrWhiteSpace($configuredCloudRoot)) {
    $configuredCloudRoot = [Environment]::GetEnvironmentVariable(
        'WORKSPACE_RELEASE_CLOUD_ROOT', 'User')
}
if (-not $SkipCloudPublish -and
    -not [string]::IsNullOrWhiteSpace($configuredCloudRoot) -and
    -not (Test-Path -LiteralPath $configuredCloudRoot -PathType Container)) {
    throw "Configured CloudRoot is unavailable: $configuredCloudRoot"
}

if ($env:SEARCHCLIENT_SKIP_VERSION_BUMP -eq '1') {
    $SkipVersionBump = $true
}

$releaseLock = & $releaseLockHelper -ProjectRoot $projectRoot
try {
$syncArgs = @{}
if (-not $SkipVersionBump) { $syncArgs.BumpPatch = $true }
$null = & $syncScript @syncArgs

$before = [IO.File]::ReadAllBytes($projectFile)
# Clear Pre/Post build events: version was already synced above; packaging runs below.
# Leaving PreBuild enabled would bump twice and rewrite .cbproj mid-build.
$command = 'call "' + $rsvarsBat + '" && msbuild "' + $projectFile + '" /t:Build /p:Config=Release /p:Platform=Win32 /p:PreBuildEvent= /p:PostBuildEvent= /v:n /nologo'
& $env:ComSpec /d /s /c $command
if ($LASTEXITCODE -ne 0) { throw "Release build failed: $LASTEXITCODE" }
$after = [IO.File]::ReadAllBytes($projectFile)
if (-not [System.Linq.Enumerable]::SequenceEqual($before, $after)) {
    throw 'MSBuild changed SearchClient_asio.cbproj.'
}
$packageArgs = @{ AllowDirty = $AllowDirty }
if ($SkipCloudPublish) { $packageArgs.SkipCloudPublish = $true }
$packageResult = & $packageScript @packageArgs
if ($null -eq $packageResult) {
    throw 'Release package script returned no result.'
}
if (-not (Test-Path -LiteralPath $packageResult.PackageDirectory -PathType Container)) {
    throw "Release package output directory is missing: $($packageResult.PackageDirectory)"
}
if (-not (Test-Path -LiteralPath $packageResult.PackagedExe -PathType Leaf)) {
    throw "Release packaged executable is missing: $($packageResult.PackagedExe)"
}

$appVersionPath = Join-Path $projectRoot 'app-version.json'
$appVersion = Get-Content -LiteralPath $appVersionPath -Raw | ConvertFrom-Json
$expectedVersion = '{0}.{1}.{2}' -f $appVersion.major, $appVersion.minor, $appVersion.patch
$expectedFileVersion = "$expectedVersion.0"
if ([string]$packageResult.Product -ne 'SearchClient' -or
    [string]$packageResult.Version -ne $expectedVersion -or
    [string]$packageResult.FileVersion -ne $expectedFileVersion) {
    throw (
        "Release package metadata mismatch: expected SearchClient " +
        "$expectedVersion ($expectedFileVersion); actual $($packageResult.Product) " +
        "$($packageResult.Version) ($($packageResult.FileVersion))"
    )
}
$packagedVersion = [Diagnostics.FileVersionInfo]::GetVersionInfo(
    $packageResult.PackagedExe
)
if ($packagedVersion.FileVersion -ne $expectedFileVersion -or
    $packagedVersion.ProductVersion -ne $expectedVersion) {
    throw 'Release packaged executable PE version does not match app-version.json.'
}

$cloudRootConfigured = -not [string]::IsNullOrWhiteSpace($configuredCloudRoot)
if ($cloudRootConfigured -and -not $SkipCloudPublish) {
    if (-not $packageResult.CloudPublished) {
        throw 'Release package was not published to the configured cloud root.'
    }
    if (-not (Test-Path -LiteralPath $packageResult.ZipPath -PathType Leaf)) {
        throw "Release cloud ZIP is missing: $($packageResult.ZipPath)"
    }
    if (-not (Test-Path -LiteralPath $packageResult.Sha256Path -PathType Leaf)) {
        throw "Release cloud SHA-256 sidecar is missing: $($packageResult.Sha256Path)"
    }
    if ([string]::IsNullOrWhiteSpace([string]$packageResult.Sha256)) {
        throw 'Release cloud ZIP SHA-256 value is missing.'
    }
}

Write-Host 'Release complete'
}
finally {
    if ($null -ne $releaseLock -and $releaseLock.Acquired) {
        try {
            $releaseLock.Mutex.ReleaseMutex()
        }
        finally {
            $releaseLock.Mutex.Dispose()
        }
    }
}
