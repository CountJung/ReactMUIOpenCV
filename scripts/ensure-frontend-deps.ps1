param(
  [string]$FrontendDir = "$PSScriptRoot\..\frontend"
)

$ErrorActionPreference = "Stop"
. (Join-Path $PSScriptRoot "script-utils.ps1")

$resolvedFrontendDir = [System.IO.Path]::GetFullPath($FrontendDir)
$packageJson = Join-Path $resolvedFrontendDir "package.json"
$packageLock = Join-Path $resolvedFrontendDir "package-lock.json"
$nodeModules = Join-Path $resolvedFrontendDir "node_modules"
$typescript = Join-Path $nodeModules "typescript\lib\typescript.js"

if (-not (Test-Path $packageJson)) {
  throw "Frontend package.json was not found at $packageJson"
}

$needsInstall = -not (Test-Path $nodeModules) -or -not (Test-Path $typescript)

if (-not $needsInstall -and (Test-Path $packageLock)) {
  $installMarker = Join-Path $nodeModules ".package-lock.json"
  if (-not (Test-Path $installMarker)) {
    $needsInstall = $true
  } elseif ((Get-Item $packageLock).LastWriteTimeUtc -gt (Get-Item $installMarker).LastWriteTimeUtc) {
    $needsInstall = $true
  }
}

if ($needsInstall) {
  Write-Host "Installing frontend dependencies..."
  Invoke-Checked -FilePath "npm" -ArgumentList @("install") -WorkingDirectory $resolvedFrontendDir
} else {
  Write-Host "Frontend dependencies are ready."
}
