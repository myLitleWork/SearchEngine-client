[CmdletBinding()]
param([Parameter(Mandatory)][string]$BuildDirectory)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
if ($env:SEARCHCLIENT_SKIP_PORTABLE_PACKAGE -eq '1') {
    Write-Host 'SearchClient portable package skipped (SEARCHCLIENT_SKIP_PORTABLE_PACKAGE=1).'
    return
}
& (Join-Path $PSScriptRoot 'New-SearchClientPackage.ps1') `
    -BuildDirectory $BuildDirectory `
    -AllowDirty
