param(
  [switch]$SkipFrontend,
  [switch]$SkipBackend,
  [switch]$SkipTypecheck
)

$ErrorActionPreference = "Stop"

$root = [System.IO.Path]::GetFullPath((Join-Path $PSScriptRoot ".."))
$frontendDir = Join-Path $root "frontend"
$backendDir = Join-Path $root "backend"

if (-not $SkipFrontend) {
  & (Join-Path $PSScriptRoot "ensure-frontend-deps.ps1") -FrontendDir $frontendDir

  if (-not $SkipTypecheck) {
    Write-Host "Type-checking frontend..."
    Push-Location $frontendDir
    try {
      npm run typecheck
    }
    finally {
      Pop-Location
    }
  }
}

if (-not $SkipBackend) {
  & (Join-Path $PSScriptRoot "setup-vcpkg.ps1")

  Write-Host "Configuring backend..."
  Push-Location $backendDir
  try {
    cmake --preset windows-msvc-vcpkg
    cmake --build --preset windows-msvc-vcpkg-debug
  }
  finally {
    Pop-Location
  }

  $debugExe = Join-Path $backendDir "out\build\windows-msvc-vcpkg\Debug\ReactMUIOpenCV.exe"
  if (-not (Test-Path $debugExe)) {
    throw "Debug backend executable was not found at $debugExe"
  }

  Write-Host "Debug backend executable ready: $debugExe"

  $debugAppExe = Join-Path $backendDir "out\build\windows-msvc-vcpkg\Debug\ReactMUIOpenCVApp.exe"
  if (Test-Path $debugAppExe) {
    Write-Host "Debug desktop app executable ready: $debugAppExe"
  } else {
    Write-Warning "Debug desktop app executable was not found at $debugAppExe. Check WebView2 SDK availability if app-mode debugging is needed."
  }
}
