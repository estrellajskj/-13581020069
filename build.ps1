param([string]$Keil = 'D:\mdk\UV4\UV4.exe')
$ErrorActionPreference = 'Stop'
if (!(Test-Path -LiteralPath $Keil)) { throw "Set -Keil to the installed UV4.exe path." }
$project = Join-Path $PSScriptRoot 'firmware\MDK\Template.uvprojx'
$log = Join-Path $PSScriptRoot 'verification\keil-build.log'
New-Item -ItemType Directory -Path (Split-Path $log) -Force | Out-Null
$process = Start-Process -FilePath $Keil -ArgumentList '-r',('"'+$project+'"'),'-o',('"'+$log+'"') -WindowStyle Hidden -PassThru
$process.WaitForExit()
$text = [IO.File]::ReadAllText($log)
Write-Output $text
if ($process.ExitCode -ge 2 -or $text -notmatch '0 Error\(s\)' -or $text -match 'Target not created') { throw 'Keil build failed.' }
Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'firmware\MDK\OBJ\Template.hex') -Destination (Join-Path $PSScriptRoot 'environment-monitor.hex')
Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'firmware\MDK\Template.map') -Destination (Join-Path $PSScriptRoot 'verification\firmware.map')
Write-Output 'Build passed. Hardware configuration and validation are still required.'
