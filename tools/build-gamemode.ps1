# Сборка мода под Windows: перекодирует UTF-8 -> CP1251 и вызывает pawncc.exe
# Пример: powershell -ExecutionPolicy Bypass -File tools\build-gamemode.ps1 -Pawncc C:\pawno\pawncc.exe
param([string]$Pawncc = "pawncc.exe", [string]$Out = "")
$ErrorActionPreference = "Stop"
$root = Split-Path -Parent $PSScriptRoot
if (-not $Out) { $Out = Join-Path $root "build\server\gamemodes\godjo.amx" }
$tmp = Join-Path $env:TEMP ("godjo_build_" + [guid]::NewGuid().ToString("N"))
Copy-Item (Join-Path $root "gamemode") $tmp -Recurse
$utf8 = New-Object System.Text.UTF8Encoding($false)
$cp = [System.Text.Encoding]::GetEncoding(1251)
Get-ChildItem $tmp -Recurse -Include *.pwn,*.inc | ForEach-Object {
  $text = [System.IO.File]::ReadAllText($_.FullName, $utf8)
  [System.IO.File]::WriteAllText($_.FullName, $text, $cp)
}
New-Item -ItemType Directory -Force -Path (Split-Path $Out) | Out-Null
& $Pawncc (Join-Path $tmp "godjo.pwn") "-i$tmp\include" "-i$tmp" "-o$Out" "-d0" "-O1" "-;+"
$code = $LASTEXITCODE
Remove-Item $tmp -Recurse -Force
if ($code -ne 0) { throw "pawncc завершился с кодом $code" }
Write-Host "OK: $Out"
