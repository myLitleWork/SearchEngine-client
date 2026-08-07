[CmdletBinding()]
param([Parameter(Mandatory)][string]$BuildDirectory)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
& (Join-Path $PSScriptRoot 'New-SearchClientPackage.ps1') `
    -BuildDirectory $BuildDirectory `
    -AllowDirty
