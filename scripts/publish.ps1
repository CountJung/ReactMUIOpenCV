param(
  [string]$Version = (Get-Date -Format "yyyy.MM.dd.HHmm"),
  [switch]$SkipBuild
)

$ErrorActionPreference = "Stop"

$root = [System.IO.Path]::GetFullPath((Join-Path $PSScriptRoot ".."))
$frontendDist = Join-Path $root "frontend\dist"
$releaseDir = Join-Path $root "backend\out\build\windows-msvc-vcpkg\Release"
$releaseExe = Join-Path $releaseDir "ReactMUIOpenCV.exe"
$publishRoot = Join-Path $root "publish"
$bundleName = "ReactMUIOpenCV"
$stageDir = Join-Path $publishRoot $bundleName
$archivePath = Join-Path $publishRoot "$bundleName-$Version.zip"
$latestArchivePath = Join-Path $publishRoot "$bundleName-latest.zip"

if (-not $SkipBuild) {
  & (Join-Path $root "build.ps1")
}

if (-not (Test-Path $releaseExe)) {
  throw "Release executable was not found at $releaseExe"
}

if (-not (Test-Path $frontendDist)) {
  throw "Frontend dist was not found at $frontendDist"
}

if (-not (Test-Path $publishRoot)) {
  New-Item -ItemType Directory -Path $publishRoot | Out-Null
}

if (Test-Path $stageDir) {
  Remove-Item -LiteralPath $stageDir -Recurse -Force
}

New-Item -ItemType Directory -Path $stageDir | Out-Null
New-Item -ItemType Directory -Path (Join-Path $stageDir "frontend") | Out-Null

Get-ChildItem -Path $releaseDir -File |
  Where-Object { $_.Extension -in @(".exe", ".dll") } |
  Copy-Item -Destination $stageDir

Copy-Item -Path $frontendDist -Destination (Join-Path $stageDir "frontend\dist") -Recurse

$docsDir = Join-Path $root "docs"
if (Test-Path $docsDir) {
  Copy-Item -Path $docsDir -Destination (Join-Path $stageDir "docs") -Recurse
}

$startScript = @'
$ErrorActionPreference = "Stop"
$root = $PSScriptRoot
$exe = Join-Path $root "ReactMUIOpenCV.exe"
if (-not (Test-Path $exe)) {
  throw "ReactMUIOpenCV.exe was not found next to this script."
}

Start-Process -FilePath $exe -WorkingDirectory $root
Start-Sleep -Seconds 2
Start-Process "http://127.0.0.1:18730"
'@

Set-Content -Path (Join-Path $stageDir "Start-ReactMUIOpenCV.ps1") -Value $startScript -Encoding UTF8

$lanScript = @'
$ErrorActionPreference = "Stop"
$root = $PSScriptRoot
$exe = Join-Path $root "ReactMUIOpenCV.exe"
if (-not (Test-Path $exe)) {
  throw "ReactMUIOpenCV.exe was not found next to this script."
}

Write-Warning "LAN mode binds the backend to 0.0.0.0. Use it only on trusted networks."
Start-Process -FilePath $exe -ArgumentList "--lan" -WorkingDirectory $root
Start-Sleep -Seconds 2
Start-Process "http://127.0.0.1:18730/remote-access"
'@

Set-Content -Path (Join-Path $stageDir "Start-ReactMUIOpenCV-LAN.ps1") -Value $lanScript -Encoding UTF8

$installScript = @'
param(
  [string]$InstallDir = "$env:LOCALAPPDATA\Programs\ReactMUIOpenCV"
)

$ErrorActionPreference = "Stop"

$source = $PSScriptRoot
$resolvedInstallDir = [System.IO.Path]::GetFullPath($InstallDir)

if (-not (Test-Path $resolvedInstallDir)) {
  New-Item -ItemType Directory -Path $resolvedInstallDir | Out-Null
}

Copy-Item -Path (Join-Path $source "*") -Destination $resolvedInstallDir -Recurse -Force

$shortcutDir = Join-Path $env:APPDATA "Microsoft\Windows\Start Menu\Programs"
$shortcutPath = Join-Path $shortcutDir "ReactMUIOpenCV.lnk"
$targetScript = Join-Path $resolvedInstallDir "Start-ReactMUIOpenCV.ps1"

$shell = New-Object -ComObject WScript.Shell
$shortcut = $shell.CreateShortcut($shortcutPath)
$shortcut.TargetPath = "powershell.exe"
$shortcut.Arguments = "-ExecutionPolicy Bypass -File `"$targetScript`""
$shortcut.WorkingDirectory = $resolvedInstallDir
$shortcut.Save()

Write-Host "Installed ReactMUIOpenCV to $resolvedInstallDir"
Write-Host "Start menu shortcut: $shortcutPath"
'@

Set-Content -Path (Join-Path $stageDir "Install-ReactMUIOpenCV.ps1") -Value $installScript -Encoding UTF8

$readme = @"
# ReactMUIOpenCV Bundle

Version: $Version

Run:

```powershell
.\Start-ReactMUIOpenCV.ps1
```

Install for the current Windows user:

```powershell
.\Install-ReactMUIOpenCV.ps1
```

LAN mode:

```powershell
.\Start-ReactMUIOpenCV-LAN.ps1
```

Read `docs/USER_GUIDE.md` and `docs/PUBLISHING.md` for details.
"@

Set-Content -Path (Join-Path $stageDir "README.md") -Value $readme -Encoding UTF8

if (Test-Path $archivePath) {
  Remove-Item -LiteralPath $archivePath -Force
}

if (Test-Path $latestArchivePath) {
  Remove-Item -LiteralPath $latestArchivePath -Force
}

Compress-Archive -Path (Join-Path $stageDir "*") -DestinationPath $archivePath -Force
Copy-Item -Path $archivePath -Destination $latestArchivePath -Force

Write-Host "Published bundle directory: $stageDir"
Write-Host "Published archive: $archivePath"
Write-Host "Latest archive: $latestArchivePath"
