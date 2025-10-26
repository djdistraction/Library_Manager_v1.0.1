@echo off
REM Library Manager - Uninstaller for Windows
REM This batch file launches the PowerShell uninstallation script

echo.
echo ========================================================
echo   Library Manager - Uninstaller
echo ========================================================
echo.

REM Check if PowerShell is available
where powershell >nul 2>nul
if %errorlevel% neq 0 (
    echo ERROR: PowerShell is not available on this system.
    pause
    exit /b 1
)

REM Execute the PowerShell uninstallation script
powershell.exe -ExecutionPolicy Bypass -File "%~dp0uninstall.ps1"

if %errorlevel% neq 0 (
    echo.
    echo Uninstallation encountered an error.
    pause
    exit /b %errorlevel%
)
