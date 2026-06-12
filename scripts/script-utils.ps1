$ErrorActionPreference = "Stop"

function Invoke-Checked {
  param(
    [Parameter(Mandatory = $true)]
    [string]$FilePath,
    [string[]]$ArgumentList = @(),
    [string]$WorkingDirectory
  )

  $previousLocation = Get-Location
  try {
    if (-not [string]::IsNullOrWhiteSpace($WorkingDirectory)) {
      Push-Location $WorkingDirectory
    }

    & $FilePath @ArgumentList
    $exitCode = $LASTEXITCODE
    if ($null -ne $exitCode -and $exitCode -ne 0) {
      throw "$FilePath exited with code $exitCode"
    }
  }
  finally {
    if ((Get-Location).Path -ne $previousLocation.Path) {
      Pop-Location
    }
  }
}

function Reset-CMakeBuildDirWhenGeneratorChanges {
  param(
    [Parameter(Mandatory = $true)]
    [string]$BackendDir,
    [string]$PresetName,
    [string]$ExpectedGenerator,
    [string]$BuildDir
  )

  $resolvedBackendDir = [System.IO.Path]::GetFullPath($BackendDir)
  if ([string]::IsNullOrWhiteSpace($ExpectedGenerator) -or [string]::IsNullOrWhiteSpace($BuildDir)) {
    if ([string]::IsNullOrWhiteSpace($PresetName)) {
      throw "Either PresetName or both ExpectedGenerator and BuildDir must be provided."
    }

    $presetsPath = Join-Path $resolvedBackendDir "CMakePresets.json"
    if (-not (Test-Path $presetsPath)) {
      throw "CMakePresets.json was not found at $presetsPath"
    }

    $presets = Get-Content -Raw $presetsPath | ConvertFrom-Json
    $preset = $presets.configurePresets | Where-Object { $_.name -eq $PresetName } | Select-Object -First 1
    if (-not $preset) {
      throw "CMake configure preset '$PresetName' was not found in $presetsPath"
    }

    $BuildDir = $preset.binaryDir.Replace('${sourceDir}', $resolvedBackendDir).Replace('${presetName}', $PresetName)
    $ExpectedGenerator = $preset.generator
  }

  $resolvedBuildDir = [System.IO.Path]::GetFullPath($BuildDir)
  $allowedRoot = [System.IO.Path]::GetFullPath((Join-Path $resolvedBackendDir "out\build"))
  if (-not $resolvedBuildDir.StartsWith($allowedRoot, [System.StringComparison]::OrdinalIgnoreCase)) {
    throw "Refusing to clean CMake files outside backend/out/build: $resolvedBuildDir"
  }

  $cachePath = Join-Path $resolvedBuildDir "CMakeCache.txt"
  if (-not (Test-Path $cachePath)) {
    return
  }

  $existingGenerator = Get-Content $cachePath |
    Where-Object { $_ -like "CMAKE_GENERATOR:INTERNAL=*" } |
    Select-Object -First 1
  $existingGenerator = $existingGenerator -replace "^CMAKE_GENERATOR:INTERNAL=", ""

  if ($existingGenerator -and $existingGenerator -ne $ExpectedGenerator) {
    Write-Warning "CMake generator changed from '$existingGenerator' to '$ExpectedGenerator'. Removing generated CMake cache files in $resolvedBuildDir."
    Remove-Item -LiteralPath $cachePath -Force
    $cmakeFiles = Join-Path $resolvedBuildDir "CMakeFiles"
    if (Test-Path $cmakeFiles) {
      Remove-Item -LiteralPath $cmakeFiles -Recurse -Force
    }
  }
}

function Get-BackendBuildDir {
  param(
    [Parameter(Mandatory = $true)]
    [string]$BackendDir
  )

  return [System.IO.Path]::GetFullPath((Join-Path $BackendDir "out\build\windows-msvc-vcpkg"))
}

function Get-PreferredVisualStudioGenerator {
  $vswhere = Join-Path ${env:ProgramFiles(x86)} "Microsoft Visual Studio\Installer\vswhere.exe"
  if (-not (Test-Path $vswhere)) {
    throw "vswhere.exe was not found. Install Visual Studio Build Tools with the C++ workload."
  }

  $instancesJson = & $vswhere -all -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -format json
  if ($LASTEXITCODE -ne 0) {
    throw "vswhere.exe failed while searching for Visual Studio C++ tools."
  }

  $instances = $instancesJson | ConvertFrom-Json
  if (-not $instances) {
    throw "No Visual Studio installation with C++ tools was found."
  }

  $cmakeHelp = & cmake --help
  if ($LASTEXITCODE -ne 0) {
    throw "cmake --help failed. Check that CMake is installed and available on PATH."
  }
  $cmakeHelpText = $cmakeHelp -join "`n"

  $orderedInstances = @($instances) | Sort-Object { [version]$_.installationVersion } -Descending
  foreach ($instance in $orderedInstances) {
    $major = ([version]$instance.installationVersion).Major
    if ($major -ge 18 -and $cmakeHelpText -match "Visual Studio 18 2026") {
      return "Visual Studio 18 2026"
    }
    if ($major -eq 17 -and $cmakeHelpText -match "Visual Studio 17 2022") {
      return "Visual Studio 17 2022"
    }
  }

  throw "No supported Visual Studio CMake generator was found. Install Visual Studio 2022 Build Tools or newer CMake support for your Visual Studio version."
}

function Invoke-BackendConfigure {
  param(
    [Parameter(Mandatory = $true)]
    [string]$BackendDir
  )

  $resolvedBackendDir = [System.IO.Path]::GetFullPath($BackendDir)
  $buildDir = Get-BackendBuildDir -BackendDir $resolvedBackendDir
  $generator = Get-PreferredVisualStudioGenerator
  $toolchainFile = [System.IO.Path]::GetFullPath((Join-Path $resolvedBackendDir "..\.tools\vcpkg\scripts\buildsystems\vcpkg.cmake"))

  Reset-CMakeBuildDirWhenGeneratorChanges -BackendDir $resolvedBackendDir -ExpectedGenerator $generator -BuildDir $buildDir

  Write-Host "Using CMake generator: $generator"
  Invoke-Checked -FilePath "cmake" -ArgumentList @(
    "-S", $resolvedBackendDir,
    "-B", $buildDir,
    "-G", $generator,
    "-A", "x64",
    "-DCMAKE_TOOLCHAIN_FILE=$toolchainFile",
    "-DCMAKE_CXX_STANDARD=20",
    "-DCMAKE_CXX_STANDARD_REQUIRED=ON"
  )
}

function Invoke-BackendBuild {
  param(
    [Parameter(Mandatory = $true)]
    [string]$BackendDir,
    [ValidateSet("Debug", "Release")]
    [string]$Configuration = "Debug"
  )

  $buildDir = Get-BackendBuildDir -BackendDir $BackendDir
  Invoke-Checked -FilePath "cmake" -ArgumentList @("--build", $buildDir, "--config", $Configuration)
}
