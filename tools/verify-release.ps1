$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path -Parent $PSScriptRoot
$manifestPath = Join-Path $projectRoot 'website\assets\releases.js'
$manifest = Get-Content -Raw -LiteralPath $manifestPath
$pattern = 'sha256:"([0-9a-f]{64})",url:"([^"]+)"'
$matches = [regex]::Matches($manifest, $pattern)

if ($matches.Count -eq 0) {
    throw 'No release artifacts were found in releases.js'
}

$failed = $false
foreach ($match in $matches) {
    $expected = $match.Groups[1].Value
    $relativeUrl = $match.Groups[2].Value -replace '/', '\'
    $artifact = Join-Path (Join-Path $projectRoot 'website') $relativeUrl
    $resolved = [IO.Path]::GetFullPath($artifact)
    if (-not (Test-Path -LiteralPath $resolved -PathType Leaf)) {
        Write-Error "Missing release artifact: $resolved"
        $failed = $true
        continue
    }
    $actual = (Get-FileHash -Algorithm SHA256 -LiteralPath $resolved).Hash.ToLowerInvariant()
    $name = Split-Path -Leaf $resolved
    if ($actual -ne $expected) {
        Write-Error "$name hash mismatch: expected $expected, got $actual"
        $failed = $true
    } else {
        Write-Host "verified $name  $actual"
    }
}

if ($failed) {
    exit 1
}
Write-Host "all $($matches.Count) release artifacts verified"
