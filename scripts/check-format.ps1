[CmdletBinding()]
param(
  [switch]$SkipFrontend,
  [switch]$SkipBackend
)

$ErrorActionPreference = "Stop"
$Root = Resolve-Path (Join-Path $PSScriptRoot "..")
$FrontendDir = Join-Path $Root "frontend"
$BackendSrcDir = Join-Path $Root "backend/src"

function Invoke-CheckedCommand {
  param(
    [Parameter(Mandatory = $true)]
    [string]$Command,
    [Parameter(Mandatory = $true)]
    [string[]]$Arguments,
    [Parameter(Mandatory = $true)]
    [string]$WorkingDirectory
  )

  Push-Location $WorkingDirectory
  try {
    & $Command @Arguments
    if ($LASTEXITCODE -ne 0) {
      throw "$Command $($Arguments -join ' ') failed with exit code $LASTEXITCODE."
    }
  } finally {
    Pop-Location
  }
}

function Get-CppFormatFiles {
  Get-ChildItem -Path $BackendSrcDir -Recurse -File |
    Where-Object { $_.Extension -in @(".c", ".cc", ".cpp", ".cxx", ".h", ".hpp") }
}

function Resolve-ClangFormat {
  $fromPath = Get-Command "clang-format" -ErrorAction SilentlyContinue
  if ($fromPath) {
    return $fromPath.Source
  }

  $candidateRoots = @(
    (Join-Path ${env:ProgramFiles} "LLVM/bin/clang-format.exe"),
    (Join-Path ${env:USERPROFILE} ".vscode/extensions/ms-vscode.cpptools-*/LLVM/bin/clang-format.exe"),
    (Join-Path ${env:USERPROFILE} ".vscode-insiders/extensions/ms-vscode.cpptools-*/LLVM/bin/clang-format.exe")
  )

  foreach ($candidateRoot in $candidateRoots) {
    $candidate = Get-ChildItem -Path $candidateRoot -ErrorAction SilentlyContinue |
      Sort-Object FullName -Descending |
      Select-Object -First 1
    if ($candidate) {
      return $candidate.FullName
    }
  }

  return $null
}

if (-not $SkipFrontend) {
  Invoke-CheckedCommand -Command "npm" -Arguments @("run", "format:check") -WorkingDirectory $FrontendDir
}

if (-not $SkipBackend) {
  $clangFormat = Resolve-ClangFormat
  if (-not $clangFormat) {
    throw "clang-format was not found. Install LLVM or the VS Code C/C++ extension before checking backend sources."
  }

  foreach ($file in Get-CppFormatFiles) {
    & $clangFormat "--dry-run" "--Werror" "--style=file" $file.FullName
    if ($LASTEXITCODE -ne 0) {
      throw "clang-format check failed for $($file.FullName)."
    }
  }
}
