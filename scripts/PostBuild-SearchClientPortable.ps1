[CmdletBinding()]
param([Parameter(Mandatory)][string]$BuildDirectory)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
if ($env:SEARCHCLIENT_SKIP_PORTABLE_PACKAGE -eq '1') {
    Write-Host 'SearchClient portable package skipped (SEARCHCLIENT_SKIP_PORTABLE_PACKAGE=1).'
    return
}
$project = (Resolve-Path -LiteralPath (Join-Path $PSScriptRoot '..')).Path
& (Join-Path $PSScriptRoot 'New-SearchClientPackage.ps1') `
    -BuildDirectory $BuildDirectory `
    -AllowDirty
$commit = & (Join-Path $PSScriptRoot 'Find-WorkspaceReleaseRoot.ps1') `
    -Name 'Commit-VclReleaseVersion.ps1' -StartPath $project
$null = & $commit -ProjectRoot $project -ProductName 'SearchClient'
