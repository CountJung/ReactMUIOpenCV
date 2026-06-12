param(
  [ValidateSet("App", "Web", "Lan")]
  [string]$Mode = "App",
  [switch]$Release,
  [switch]$NoBrowser
)

$ErrorActionPreference = "Stop"

$root = [System.IO.Path]::GetFullPath((Join-Path $PSScriptRoot ".."))
$config = if ($Release) { "Release" } else { "Debug" }
$buildDir = Join-Path $root "backend\out\build\windows-msvc-vcpkg\$config"
$backendExe = Join-Path $buildDir "ReactMUIOpenCV.exe"
$appExe = Join-Path $buildDir "ReactMUIOpenCVApp.exe"

if ($Mode -eq "App") {
  if (-not (Test-Path $appExe)) {
    throw "Desktop app executable was not found at $appExe. Run scripts\bootstrap.ps1 or scripts\build.ps1 first."
  }

  Start-Process -FilePath $appExe -WorkingDirectory $buildDir
  Write-Host "Started desktop app: $appExe"
  exit 0
}

if (-not (Test-Path $backendExe)) {
  throw "Backend executable was not found at $backendExe. Run scripts\bootstrap.ps1 or scripts\build.ps1 first."
}

$arguments = @()
if ($Mode -eq "Lan") {
  $arguments += "--lan"
  Write-Warning "LAN mode binds to 0.0.0.0. Use it only on trusted networks."
}

Start-Process -FilePath $backendExe -ArgumentList $arguments -WorkingDirectory $root
Write-Host "Started backend: $backendExe $($arguments -join ' ')"

if (-not $NoBrowser) {
  Start-Sleep -Seconds 2
  $url = if ($Mode -eq "Lan") { "http://127.0.0.1:18730/remote-access" } else { "http://127.0.0.1:18730" }
  Start-Process $url
  Write-Host "Opened $url"
}
