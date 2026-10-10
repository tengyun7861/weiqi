param(
    [ValidateSet('Debug', 'Release')][string]$Configuration = 'Release'
)
$ErrorActionPreference = 'Stop'
$projectRoot = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
function Invoke-Checked([string]$Program, [string[]]$CommandArgs) {
    & $Program @CommandArgs
    if ($LASTEXITCODE -ne 0) { throw "$Program failed with exit code $LASTEXITCODE" }
}
Push-Location $projectRoot
try {
    Invoke-Checked 'npm.cmd' @('--prefix', 'client-web', 'ci')
    Invoke-Checked 'npm.cmd' @('--prefix', 'client-web', 'run', 'lint')
    Invoke-Checked 'npm.cmd' @('--prefix', 'client-web', 'run', 'format:check')
    Invoke-Checked 'npm.cmd' @('--prefix', 'client-web', 'run', 'build')
    Invoke-Checked 'cmake' @('--preset', 'windows-msvc')
    Invoke-Checked 'cmake' @('--build', 'build-msvc', '--config', $Configuration, '--parallel', '4')
    Invoke-Checked 'ctest' @('--test-dir', 'build-msvc', '-C', $Configuration, '--output-on-failure')
    Invoke-Checked 'cmake' @('--install', 'build-msvc', '--config', $Configuration, '--prefix', 'build-msvc/site-package')
    Write-Host "Website package: $projectRoot\build-msvc\site-package\bin"
} finally { Pop-Location }
