$ErrorActionPreference = "Stop"

$root = Split-Path -Parent $PSScriptRoot
$third = Join-Path $root "third_party"
New-Item -ItemType Directory -Force -Path $third | Out-Null

$sdl = Join-Path $third "SDL2-2.30.10.tar.gz"
if (-not (Test-Path $sdl)) {
  $url = "https://github.com/libsdl-org/SDL/releases/download/release-2.30.10/SDL2-2.30.10.tar.gz"
  Write-Host "Downloading SDL2 archive to:`n  $sdl"
  Invoke-WebRequest -Uri $url -OutFile $sdl -UseBasicParsing
}

Write-Host "OK: $sdl"
