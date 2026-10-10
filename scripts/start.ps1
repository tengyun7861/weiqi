param(
    [ValidateSet('Debug', 'Release')][string]$Configuration = 'Release',
    [ValidateRange(1, 65535)][int]$Port = 8081,
    [string]$DatabasePath = 'data\weiqi.sqlite3'
)
$ErrorActionPreference = 'Stop'
$projectRoot = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$serverPath = Join-Path $projectRoot "build-msvc\server-cpp\$Configuration\weiqi_server.exe"
$webRoot = Join-Path $projectRoot 'client-web\dist'
if (-not (Test-Path -LiteralPath $serverPath)) { throw 'Build first with scripts\build.ps1.' }
if (-not (Test-Path -LiteralPath (Join-Path $webRoot 'index.html'))) { throw 'Build the frontend first.' }
Push-Location $projectRoot
try { & $serverPath $DatabasePath $Port $webRoot }
finally { Pop-Location }
