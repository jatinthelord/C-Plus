param(
  [string]$Prefix = (Join-Path ([Environment]::GetFolderPath('LocalApplicationData')) 'CSP')
)

$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path -Parent $PSScriptRoot
$binaryDirectory = Join-Path $Prefix 'bin'
$includeDirectory = Join-Path $Prefix 'include'
$shareDirectory = Join-Path $Prefix 'share\csx'
$tools = @('cspc.exe', 'csx.exe', 'csp-codegen.exe', 'cspweb.exe', 'csp-bench.exe')

New-Item -ItemType Directory -Force -Path $binaryDirectory | Out-Null
New-Item -ItemType Directory -Force -Path $includeDirectory | Out-Null
New-Item -ItemType Directory -Force -Path $shareDirectory | Out-Null

foreach ($tool in $tools) {
  $builtSource = Join-Path (Join-Path $projectRoot 'build') $tool
  $rootSource = Join-Path $projectRoot $tool
  $source = if (Test-Path -LiteralPath $builtSource -PathType Leaf) {
    $builtSource
  } else {
    $rootSource
  }
  if (-not (Test-Path -LiteralPath $source -PathType Leaf)) {
    throw "Missing compiled tool: $source"
  }
  Copy-Item -LiteralPath $source -Destination (Join-Path $binaryDirectory $tool) -Force
}

Copy-Item -Path (Join-Path $projectRoot 'include\*') -Destination $includeDirectory -Recurse -Force
Copy-Item -LiteralPath (Join-Path $projectRoot 'packages\csx\index.json') -Destination (Join-Path $shareDirectory 'index.json') -Force

$userPath = [Environment]::GetEnvironmentVariable('Path', 'User')
$entries = @($userPath -split ';' | Where-Object { $_ })
$alreadyInstalled = $entries | Where-Object {
  $_.TrimEnd('\') -ieq $binaryDirectory.TrimEnd('\')
}
if (-not $alreadyInstalled) {
  $newPath = (($entries + $binaryDirectory) -join ';')
  [Environment]::SetEnvironmentVariable('Path', $newPath, 'User')
}

Write-Host "Installed CSP tools in $binaryDirectory"
Write-Host 'Open a new terminal, then run: cspc --version'
