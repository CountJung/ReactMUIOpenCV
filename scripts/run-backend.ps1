param(
  [switch]$Lan
)

$ErrorActionPreference = "Stop"

$root = Resolve-Path "$PSScriptRoot\.."
$backendDir = Join-Path $root "backend"
$candidate = Get-ChildItem -Path (Join-Path $backendDir "out") -Recurse -Filter "ReactMUIOpenCV.exe" -ErrorAction SilentlyContinue |
  Sort-Object LastWriteTime -Descending |
  Select-Object -First 1

if (-not $candidate) {
  throw "ReactMUIOpenCV.exe was not found under backend/out. Build the backend first."
}

$arguments = @()
if ($Lan) {
  $arguments += "--lan"
}

Write-Host "Running $($candidate.FullName) $($arguments -join ' ')"
& $candidate.FullName @arguments
