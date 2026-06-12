param(
  [switch]$SkipWinget,
  [switch]$SkipVisualStudioBuildTools,
  [switch]$SkipNode,
  [switch]$SkipCMake,
  [switch]$SkipWebView2Runtime
)

$ErrorActionPreference = "Stop"

function Test-Command {
  param([string]$Name)
  return [bool](Get-Command $Name -ErrorAction SilentlyContinue)
}

function Install-WingetPackage {
  param(
    [string]$Id,
    [string]$Name,
    [string[]]$ExtraArgs = @()
  )

  if ($SkipWinget) {
    Write-Warning "$Name is missing, but winget installation is disabled."
    return
  }

  if (-not (Test-Command "winget")) {
    throw "winget was not found. Install App Installer from Microsoft Store or install $Name manually."
  }

  Write-Host "Installing $Name with winget..."
  $args = @(
    "install",
    "--id", $Id,
    "--source", "winget",
    "--accept-source-agreements",
    "--accept-package-agreements"
  ) + $ExtraArgs

  & winget @args
  if ($LASTEXITCODE -ne 0) {
    throw "winget failed while installing $Name"
  }
}

function Update-ProcessPath {
  $machinePath = [Environment]::GetEnvironmentVariable("Path", "Machine")
  $userPath = [Environment]::GetEnvironmentVariable("Path", "User")
  $env:Path = "$machinePath;$userPath"
}

if (-not $SkipNode -and -not (Test-Command "node")) {
  Install-WingetPackage -Id "OpenJS.NodeJS.LTS" -Name "Node.js LTS"
  Update-ProcessPath
}

if (-not $SkipCMake -and -not (Test-Command "cmake")) {
  Install-WingetPackage -Id "Kitware.CMake" -Name "CMake"
  Update-ProcessPath
}

if (-not $SkipWebView2Runtime) {
  Install-WingetPackage -Id "Microsoft.EdgeWebView2Runtime" -Name "Microsoft Edge WebView2 Runtime"
}

if (-not $SkipVisualStudioBuildTools) {
  $vswhere = Join-Path ${env:ProgramFiles(x86)} "Microsoft Visual Studio\Installer\vswhere.exe"
  $hasVctools = $false
  if (Test-Path $vswhere) {
    $installPath = & $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
    $hasVctools = -not [string]::IsNullOrWhiteSpace($installPath)
  }

  if (-not $hasVctools) {
    Install-WingetPackage `
      -Id "Microsoft.VisualStudio.2022.BuildTools" `
      -Name "Visual Studio 2022 Build Tools" `
      -ExtraArgs @(
        "--override",
        "--wait --passive --norestart --add Microsoft.VisualStudio.Workload.VCTools --add Microsoft.VisualStudio.Component.VC.CMake.Project --add Microsoft.VisualStudio.Component.Windows11SDK.22621 --includeRecommended"
      )
    Update-ProcessPath
  } else {
    Write-Host "Visual Studio C++ Build Tools are ready."
  }
}

Write-Host "Windows prerequisites checked."
