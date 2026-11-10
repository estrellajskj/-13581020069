@echo off
setlocal
set "ENV_MONITOR_UV4=D:\mdk\UV4\UV4.exe"
if exist "%ENV_MONITOR_UV4%" goto open_project
set "ENV_MONITOR_UV4=C:\Keil_v5\UV4\UV4.exe"
if exist "%ENV_MONITOR_UV4%" goto open_project
start "" "%~dp0firmware\MDK\Template.uvprojx"
exit /b
:open_project
start "" "%ENV_MONITOR_UV4%" "%~dp0firmware\MDK\Template.uvprojx"
