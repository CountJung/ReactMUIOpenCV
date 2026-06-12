param(
  [switch]$SkipFrontendInstall,
  [switch]$SkipFrontendBuild,
  [switch]$SkipBackendBuild
)

$ErrorActionPreference = "Stop"
. (Join-Path $PSScriptRoot "script-utils.ps1")

$root = [System.IO.Path]::GetFullPath((Join-Path $PSScriptRoot ".."))
$frontendDir = Join-Path $root "frontend"
$backendDir = Join-Path $root "backend"

Write-Host "ReactMUIOpenCV Release build"
Write-Host "Workspace: $root"

if (-not $SkipFrontendBuild) {
  if (-not $SkipFrontendInstall) {
    & (Join-Path $PSScriptRoot "ensure-frontend-deps.ps1") -FrontendDir $frontendDir
  }

  Write-Host "Building frontend release bundle..."
  Invoke-Checked -FilePath "npm" -ArgumentList @("run", "build") -WorkingDirectory $frontendDir
}

if (-not $SkipBackendBuild) {
  & (Join-Path $PSScriptRoot "setup-vcpkg.ps1")

  Write-Host "Configuring backend release build..."
  Invoke-BackendConfigure -BackendDir $backendDir
  Invoke-BackendBuild -BackendDir $backendDir -Configuration "Release"
}

$releaseExe = Join-Path $backendDir "out\build\windows-msvc-vcpkg\Release\ReactMUIOpenCV.exe"
$releaseAppExe = Join-Path $backendDir "out\build\windows-msvc-vcpkg\Release\ReactMUIOpenCVApp.exe"
if (Test-Path $releaseExe) {
  Write-Host "Release backend app built: $releaseExe"
} elseif (-not $SkipBackendBuild) {
  throw "Release backend executable was not found at $releaseExe"
}

if (Test-Path $releaseAppExe) {
  Write-Host "Release desktop app built: $releaseAppExe"
} elseif (-not $SkipBackendBuild) {
  Write-Warning "Release desktop app executable was not found at $releaseAppExe. Check WebView2 SDK availability if app mode is required."
}
