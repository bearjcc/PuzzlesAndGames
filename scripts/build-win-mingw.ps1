$ErrorActionPreference = "Stop"

$root = Split-Path -Parent $PSScriptRoot
$mingwBin = Join-Path $root "tools\winlibs\mingw64\bin"
if (-not (Test-Path (Join-Path $mingwBin "gcc.exe"))) {
  Write-Error "Missing MinGW toolchain at:`n  $mingwBin`nDownload WinLibs (UCRT, POSIX, SEH) and extract to tools\winlibs\mingw64, or run your own winget install."
}

$kitwareCmake = "C:\Program Files\CMake\bin"
if (-not (Test-Path (Join-Path $kitwareCmake "cmake.exe"))) {
  Write-Error "Missing Kitware CMake at:`n  $kitwareCmake`nInstall CMake from https://cmake.org/download/ or winget install Kitware.CMake"
}

$ninjaDir = "C:\Program Files\Ninja"
if (-not (Test-Path (Join-Path $ninjaDir "ninja.exe"))) {
  $ninjaDir = ""
}

$env:Path = "$mingwBin;$kitwareCmake;$ninjaDir;" + $env:Path

& (Join-Path $PSScriptRoot "fetch-deps.ps1")

Set-Location $root
if (Test-Path build) {
  Remove-Item -Recurse -Force build
}

cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
ctest --test-dir build --output-on-failure

Write-Host ""
Write-Host "Run:"
Write-Host "  .\build\puzzlesgames.exe"
