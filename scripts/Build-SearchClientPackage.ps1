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
$productName = 'SearchClient'

$resolveScript = @(
    (Join-Path $projectRoot '..\..\TOOLS\scripts\release\Resolve-WorkspaceReleaseScript.ps1'),
    (Join-Path $projectRoot '..\..\..\TOOLS\scripts\release\Resolve-WorkspaceReleaseScript.ps1')
) | ForEach-Object { [IO.Path]::GetFullPath($_) } |
    Where-Object { Test-Path -LiteralPath $_ -PathType Leaf } |
    Select-Object -First 1
if ($null -eq $resolveScript) {
    throw 'Resolve-WorkspaceReleaseScript.ps1 not found.'
}
$syncScript = & $resolveScript -ScriptName 'Sync-VclProjectVersion.ps1' -StartPath $projectRoot
$syncArgs = @{
    ProjectRoot = $projectRoot
    ProductName = $productName
    Cbproj      = $projectFile
}
if (-not $SkipVersionBump) { $syncArgs.BumpPatch = $true }
$null = & $syncScript @syncArgs

$before = (Get-FileHash -LiteralPath $projectFile -Algorithm SHA256).Hash
$command = 'call "C:\Program Files (x86)\Embarcadero\Studio\22.0\bin\rsvars.bat" && msbuild "' + $projectFile + '" /t:Make /p:Config=Release /p:Platform=Win32 /p:PostBuildEvent= /v:n /nologo'
& $env:ComSpec /d /s /c $command
if ($LASTEXITCODE -ne 0) { throw "Release build failed: $LASTEXITCODE" }
$after = (Get-FileHash -LiteralPath $projectFile -Algorithm SHA256).Hash
if ($after -ne $before) { throw 'MSBuild changed SearchClient_asio.cbproj.' }
$packageArgs = @{ AllowDirty = $AllowDirty }
if ($SkipCloudPublish) { $packageArgs.SkipCloudPublish = $true }
& (Join-Path $PSScriptRoot 'New-SearchClientPackage.ps1') @packageArgs
