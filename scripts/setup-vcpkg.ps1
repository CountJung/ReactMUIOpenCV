param(
  [string]$InstallDir = "$PSScriptRoot\..\.tools\vcpkg"
)

$ErrorActionPreference = "Stop"
. (Join-Path $PSScriptRoot "script-utils.ps1")

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
  Invoke-Checked -FilePath "git" -ArgumentList @("clone", "https://github.com/microsoft/vcpkg.git", $resolvedInstallDir)
}

Invoke-Checked -FilePath (Join-Path $resolvedInstallDir "bootstrap-vcpkg.bat") -ArgumentList @("-disableMetrics") -WorkingDirectory $resolvedInstallDir

Write-Host "vcpkg ready at $vcpkgExe"
