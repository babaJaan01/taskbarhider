@echo off
REM Double-click launcher for install.ps1 — bypasses PowerShell execution policy.
setlocal
set "SCRIPT_DIR=%~dp0"
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%SCRIPT_DIR%install.ps1" %*
set "RC=%ERRORLEVEL%"
echo.
pause
exit /b %RC%
