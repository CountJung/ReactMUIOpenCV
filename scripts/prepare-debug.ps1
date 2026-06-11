param(
  [switch]$SkipFrontend,
  [switch]$SkipBackend,
  [switch]$SkipTypecheck,
  [switch]$BuildFrontend,
  [switch]$StopExistingRuntime
)

$ErrorActionPreference = "Stop"

$root = [System.IO.Path]::GetFullPath((Join-Path $PSScriptRoot ".."))
$frontendDir = Join-Path $root "frontend"
$backendDir = Join-Path $root "backend"

function Stop-WorkspaceRuntime {
  $buildRoot = [System.IO.Path]::GetFullPath((Join-Path $backendDir "out\build"))
  $processNames = @("ReactMUIOpenCV.exe", "ReactMUIOpenCVApp.exe")

  foreach ($processName in $processNames) {
    Get-CimInstance Win32_Process -Filter "Name = '$processName'" | ForEach-Object {
      $exePath = $_.ExecutablePath
      if ([string]::IsNullOrWhiteSpace($exePath)) {
        return
      }

      $fullExePath = [System.IO.Path]::GetFullPath($exePath)
      if (-not $fullExePath.StartsWith($buildRoot, [System.StringComparison]::OrdinalIgnoreCase)) {
        return
      }

      Write-Host "Stopping existing workspace runtime: $fullExePath (PID $($_.ProcessId))"
      Stop-Process -Id $_.ProcessId -Force
    }
  }
}

if ($StopExistingRuntime) {
  Stop-WorkspaceRuntime
}

if (-not $SkipFrontend) {
  & (Join-Path $PSScriptRoot "ensure-frontend-deps.ps1") -FrontendDir $frontendDir

  if ($BuildFrontend) {
    Write-Host "Building frontend bundle for debug static hosting..."
    Push-Location $frontendDir
    try {
      npm run build
    }
    finally {
      Pop-Location
    }

    $frontendDist = Join-Path $frontendDir "dist\index.html"
    if (-not (Test-Path $frontendDist)) {
      throw "Frontend debug bundle was not found at $frontendDist"
    }

    Write-Host "Frontend debug bundle ready: $frontendDist"
  } elseif (-not $SkipTypecheck) {
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
