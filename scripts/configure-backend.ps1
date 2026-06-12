$ErrorActionPreference = "Stop"
. (Join-Path $PSScriptRoot "script-utils.ps1")

$root = [System.IO.Path]::GetFullPath((Join-Path $PSScriptRoot ".."))
$backendDir = Join-Path $root "backend"

& (Join-Path $PSScriptRoot "setup-vcpkg.ps1")
Invoke-BackendConfigure -BackendDir $backendDir
