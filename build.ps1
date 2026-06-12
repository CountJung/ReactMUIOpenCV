$ErrorActionPreference = "Stop"

$script = Join-Path $PSScriptRoot "scripts\build.ps1"
& $script @args
exit $LASTEXITCODE
