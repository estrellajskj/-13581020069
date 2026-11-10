param([string]$Compiler = 'C:\mingw64\bin\gcc.exe')
$ErrorActionPreference = 'Stop'
if (!(Test-Path -LiteralPath $Compiler)) { throw 'Set -Compiler to a working host GCC.' }
$env:PATH = (Split-Path $Compiler) + ';' + $env:PATH
$testExe = Join-Path ([IO.Path]::GetTempPath()) ('environment-monitor-test-' + [guid]::NewGuid().ToString('N') + '.exe')
try {
    & $Compiler -std=c99 -Wall -Wextra -Werror -Wno-unused-function -Wno-misleading-indentation (Join-Path $PSScriptRoot 'test_core.c') -o $testExe
    if ($LASTEXITCODE -ne 0) { throw 'Host compilation failed' }
    & $testExe
    if ($LASTEXITCODE -ne 0) { throw 'Host test failed' }
} finally {
    if (Test-Path -LiteralPath $testExe) { Remove-Item -LiteralPath $testExe }
}
