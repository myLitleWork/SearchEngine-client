[CmdletBinding()]
param()

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

$syncPath = Join-Path $PSScriptRoot '..\Sync-SearchClientVersion.ps1'
$parseTokens = $null
$parseErrors = $null
$syncAst = [Management.Automation.Language.Parser]::ParseFile($syncPath, [ref]$parseTokens, [ref]$parseErrors)
if ($parseErrors.Count -gt 0) { throw "Cannot parse $syncPath`: $($parseErrors[0].Message)" }
$functionAsts = @($syncAst.FindAll({
    param($node)
    $node -is [Management.Automation.Language.FunctionDefinitionAst] -and
        $node.Name -eq 'Get-ReleaseInputSha256'
}, $true))
if ($functionAsts.Count -ne 1) { throw 'Expected one Get-ReleaseInputSha256 function.' }
# Load only the real fingerprint function; executing the complete sync script would mutate version files.
. ([scriptblock]::Create($functionAsts[0].Extent.Text))

$tempRoot = [IO.Path]::GetFullPath([IO.Path]::GetTempPath())
$tempPrefix = $tempRoot.TrimEnd([IO.Path]::DirectorySeparatorChar, [IO.Path]::AltDirectorySeparatorChar) + [IO.Path]::DirectorySeparatorChar
$fixtureRoot = [IO.Path]::GetFullPath((Join-Path $tempRoot ('SearchClient-fingerprint-test-' + [Guid]::NewGuid().ToString('N'))))
$utf8 = [Text.UTF8Encoding]::new($false)
try {
    $tutorialRoot = Join-Path $fixtureRoot 'tutorials'
    New-Item -ItemType Directory -Path $tutorialRoot | Out-Null
    $cssPath = Join-Path $tutorialRoot 'guide.css'
    [IO.File]::WriteAllText($cssPath, 'body { color: red; }', $utf8)
    $originalHash = Get-ReleaseInputSha256 -ProjectRoot $fixtureRoot

    # The edit keeps the byte length constant, so the hash must account for CSS content.
    [IO.File]::WriteAllText($cssPath, 'body { color: tan; }', $utf8)
    $editedHash = Get-ReleaseInputSha256 -ProjectRoot $fixtureRoot
    if ($originalHash -eq $editedHash) { throw 'Changing tutorial CSS content did not change the release fingerprint.' }

    $nestedRoot = Join-Path $tutorialRoot 'вложенные'
    New-Item -ItemType Directory -Path $nestedRoot | Out-Null
    [IO.File]::WriteAllText((Join-Path $nestedRoot 'оформление.css'), 'p { margin: 0; }', $utf8)
    $nestedHash = Get-ReleaseInputSha256 -ProjectRoot $fixtureRoot
    if ($editedHash -eq $nestedHash) { throw 'Adding nested tutorial CSS with a Cyrillic path did not change the release fingerprint.' }

    [pscustomobject]@{
        CssContentChangesFingerprint = $true
        NestedCyrillicCssChangesFingerprint = $true
        OriginalHash = $originalHash
        EditedHash = $editedHash
        NestedHash = $nestedHash
    }
}
finally {
    if (Test-Path -LiteralPath $fixtureRoot -PathType Container) {
        $resolvedFixture = (Resolve-Path -LiteralPath $fixtureRoot).ProviderPath
        if (-not $resolvedFixture.Equals($fixtureRoot, [StringComparison]::OrdinalIgnoreCase) -or
            -not $resolvedFixture.StartsWith($tempPrefix, [StringComparison]::OrdinalIgnoreCase)) {
            throw "Refusing to remove a fixture outside its expected temporary path: $resolvedFixture"
        }
        Remove-Item -LiteralPath $resolvedFixture -Recurse -Force
    }
}
