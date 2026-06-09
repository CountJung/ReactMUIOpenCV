param(
  [switch]$SkipFrontendInstall,
  [switch]$SkipFrontendBuild,
  [switch]$SkipBackendBuild
)

$ErrorActionPreference = "Stop"

$root = [System.IO.Path]::GetFullPath($PSScriptRoot)
$frontendDir = Join-Path $root "frontend"
$backendDir = Join-Path $root "backend"

Write-Host "ReactMUIOpenCV Release build"
Write-Host "Workspace: $root"

if (-not $SkipFrontendBuild) {
  if (-not $SkipFrontendInstall) {
    & (Join-Path $root "scripts\ensure-frontend-deps.ps1") -FrontendDir $frontendDir
  }

  Write-Host "Building frontend release bundle..."
  Push-Location $frontendDir
  try {
    npm run build
  }
  finally {
    Pop-Location
  }
}

if (-not $SkipBackendBuild) {
  & (Join-Path $root "scripts\setup-vcpkg.ps1")

  Write-Host "Configuring backend release build..."
  Push-Location $backendDir
  try {
    cmake --preset windows-msvc-vcpkg
    cmake --build --preset windows-msvc-vcpkg-release
  }
  finally {
    Pop-Location
  }
}

$releaseExe = Join-Path $backendDir "out\build\windows-msvc-vcpkg\Release\ReactMUIOpenCV.exe"
if (Test-Path $releaseExe) {
  Write-Host "Release backend app built: $releaseExe"
} elseif (-not $SkipBackendBuild) {
  throw "Release backend executable was not found at $releaseExe"
}
