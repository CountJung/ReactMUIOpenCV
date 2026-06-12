param(
  [ValidateSet("Debug", "Release")]
  [string]$Configuration = "Debug",
  [switch]$SkipConfigure
)

$ErrorActionPreference = "Stop"
. (Join-Path $PSScriptRoot "script-utils.ps1")

$root = [System.IO.Path]::GetFullPath((Join-Path $PSScriptRoot ".."))
$backendDir = Join-Path $root "backend"

if (-not $SkipConfigure) {
  & (Join-Path $PSScriptRoot "configure-backend.ps1")
}

Invoke-BackendBuild -BackendDir $backendDir -Configuration $Configuration
