param(
  [string]$Version = "1.0.3800.47",
  [string]$PackageRoot = "$env:USERPROFILE\.nuget\packages"
)

$ErrorActionPreference = "Stop"

$resolvedPackageRoot = [System.IO.Path]::GetFullPath($PackageRoot)
$sdkRoot = Join-Path $resolvedPackageRoot "microsoft.web.webview2\$Version"
$nativeRoot = Join-Path $sdkRoot "build\native"
$header = Join-Path $nativeRoot "include\WebView2.h"
$loader = Join-Path $nativeRoot "x64\WebView2LoaderStatic.lib"

if ((Test-Path $header) -and (Test-Path $loader)) {
  Write-Host "WebView2 SDK already exists at $nativeRoot"
  exit 0
}

if (-not (Test-Path $sdkRoot)) {
  New-Item -ItemType Directory -Path $sdkRoot | Out-Null
}

$downloadDir = Join-Path ([System.IO.Path]::GetTempPath()) "ReactMUIOpenCV-WebView2Sdk-$Version"
$nupkg = Join-Path $downloadDir "Microsoft.Web.WebView2.$Version.nupkg"
$zip = Join-Path $downloadDir "Microsoft.Web.WebView2.$Version.zip"
$url = "https://www.nuget.org/api/v2/package/Microsoft.Web.WebView2/$Version"

if (Test-Path $downloadDir) {
  Remove-Item -LiteralPath $downloadDir -Recurse -Force
}
New-Item -ItemType Directory -Path $downloadDir | Out-Null

Write-Host "Downloading WebView2 SDK $Version..."
Invoke-WebRequest -Uri $url -OutFile $nupkg
Copy-Item -Path $nupkg -Destination $zip -Force

Write-Host "Extracting WebView2 SDK to $sdkRoot"
Expand-Archive -Path $zip -DestinationPath $sdkRoot -Force

if (-not ((Test-Path $header) -and (Test-Path $loader))) {
  throw "WebView2 SDK extraction finished, but expected native files were not found under $nativeRoot"
}

Write-Host "WebView2 SDK ready at $nativeRoot"
