@echo off
REM Library Manager - One-Click Installer for Windows
REM This batch file launches the PowerShell installation script

echo.
echo ========================================================
echo   Library Manager - One-Click Install Wizard
echo ========================================================
echo.
echo Starting installation...
echo.

REM Check if PowerShell is available
where powershell >nul 2>nul
if %errorlevel% neq 0 (
    echo ERROR: PowerShell is not available on this system.
    echo Please ensure PowerShell is installed.
    pause
    exit /b 1
)

REM Execute the PowerShell installation script
REM -ExecutionPolicy Bypass allows the script to run without changing system policy
REM -File specifies the script to execute
powershell.exe -ExecutionPolicy Bypass -File "%~dp0install.ps1"

if %errorlevel% neq 0 (
    echo.
    echo Installation encountered an error.
    echo Please check the output above for details.
    pause
    exit /b %errorlevel%
)

echo.
echo Installation completed successfully!
pause
