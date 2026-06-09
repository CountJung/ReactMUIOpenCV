param(
  [string]$InstallDir = "$PSScriptRoot\..\.tools\vcpkg"
)

$ErrorActionPreference = "Stop"

$resolvedInstallDir = [System.IO.Path]::GetFullPath($InstallDir)
$vcpkgExe = Join-Path $resolvedInstallDir "vcpkg.exe"

if (Test-Path $vcpkgExe) {
  Write-Host "vcpkg already exists at $vcpkgExe"
  exit 0
}

$parent = Split-Path $resolvedInstallDir -Parent
if (-not (Test-Path $parent)) {
  New-Item -ItemType Directory -Path $parent | Out-Null
}

if (-not (Test-Path $resolvedInstallDir)) {
  git clone https://github.com/microsoft/vcpkg.git $resolvedInstallDir
}

Push-Location $resolvedInstallDir
try {
  .\bootstrap-vcpkg.bat -disableMetrics
}
finally {
  Pop-Location
}

Write-Host "vcpkg ready at $vcpkgExe"
