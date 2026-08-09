#Requires -Version 5.1
[CmdletBinding()]
param()

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
$project = (Resolve-Path -LiteralPath (Join-Path $PSScriptRoot '..')).Path
$syncScript = Join-Path $PSScriptRoot 'Sync-SearchClientVersion.ps1'
$packageScript = Join-Path $PSScriptRoot 'New-SearchClientPackage.ps1'
$resolveScript = @(
    (Join-Path $project '..\..\TOOLS\scripts\release\Resolve-WorkspaceReleaseScript.ps1'),
    (Join-Path $project '..\..\..\TOOLS\scripts\release\Resolve-WorkspaceReleaseScript.ps1')
) | ForEach-Object { [IO.Path]::GetFullPath($_) } |
    Where-Object { Test-Path -LiteralPath $_ -PathType Leaf } |
    Select-Object -First 1
if ($null -eq $resolveScript) { throw 'Resolve-WorkspaceReleaseScript.ps1 not found.' }
$preflight = & $resolveScript -ScriptName 'Test-VclReleasePrerequisites.ps1' -StartPath $project
$null = & $preflight `
    -ProjectRoot $project `
    -ProjectFilePath (Join-Path $project 'SearchClient_asio.cbproj') `
    -SyncScriptPath $syncScript `
    -PackageScriptPath $packageScript `
    -RequiredPackageInputs @('deployment\SearchClientPortable\Verify-Package.cmd','deployment\SearchClientPortable\Verify-Package.ps1','deployment\SearchClientPortable\app\sht.docm','deployment\SearchClientPortable\app\Settings.db3','deployment\SearchClientPortable\app\admin984.bin') `
    -SkipCloudPublish:($env:SEARCHCLIENT_SKIP_PORTABLE_PACKAGE -eq '1')

if ($env:SEARCHCLIENT_SKIP_VERSION_BUMP -eq '1') {
    Write-Host 'SearchClient version bump skipped (SEARCHCLIENT_SKIP_VERSION_BUMP=1).'
    $null = & $syncScript
} else {
    $null = & $syncScript -BumpPatch
}

