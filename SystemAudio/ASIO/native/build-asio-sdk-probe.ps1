param(
  [Parameter(Mandatory=$true)][string]$SdkIncludeDir,
  [string]$OutputDir = "."
)
$ErrorActionPreference = "Stop"
$header = Join-Path $SdkIncludeDir "asio.h"
if (-not (Test-Path $header)) { throw "Official ASIO SDK header not found: $header" }
$compiler = Get-Command cl.exe -ErrorAction SilentlyContinue
if (-not $compiler) { throw "Run this from an x64 Native Tools Command Prompt for Visual Studio (MSVC cl.exe required)." }
$source = Join-Path $PSScriptRoot "asio_sdk_capability_probe_windows.cpp"
New-Item -ItemType Directory -Force -Path $OutputDir | Out-Null
$exe = Join-Path (Resolve-Path $OutputDir) "asio_sdk_capability_probe.exe"
& $compiler.Source /nologo /std:c++17 /EHsc /W4 /WX "/I$SdkIncludeDir" $source "/Fe:$exe" ole32.lib
if ($LASTEXITCODE -ne 0) { throw "MSVC build failed with exit code $LASTEXITCODE" }
Write-Host "Built: $exe"
Write-Host "No ASIO driver was initialized or started."
Write-Host "Do not run the executable without explicit acceptance of driver risk."
