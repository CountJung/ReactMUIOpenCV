param(
  [switch]$SkipToolInstall,
  [switch]$SkipWebView2Sdk,
  [switch]$SkipBuild,
  [switch]$RunAfterBuild,
  [ValidateSet("App", "Web", "Lan")]
  [string]$RunMode = "App"
)

$ErrorActionPreference = "Stop"

$root = [System.IO.Path]::GetFullPath((Join-Path $PSScriptRoot ".."))

Write-Host "ReactMUIOpenCV bootstrap"
Write-Host "Workspace: $root"

if (-not $SkipToolInstall) {
  & (Join-Path $PSScriptRoot "install-windows-prereqs.ps1")
}

if (-not $SkipWebView2Sdk) {
  & (Join-Path $PSScriptRoot "setup-webview2-sdk.ps1")
}

& (Join-Path $PSScriptRoot "ensure-frontend-deps.ps1") -FrontendDir (Join-Path $root "frontend")
& (Join-Path $PSScriptRoot "setup-vcpkg.ps1")

if (-not $SkipBuild) {
  & (Join-Path $PSScriptRoot "build.ps1")
}

if ($RunAfterBuild) {
  & (Join-Path $PSScriptRoot "start.ps1") -Mode $RunMode -Release
}

Write-Host "Bootstrap complete."
