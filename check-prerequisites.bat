@echo off
REM Library Manager - Prerequisites Checker
REM This batch file checks if your system meets installation requirements

echo.
echo ========================================================
echo   Library Manager - Prerequisites Checker
echo ========================================================
echo.

REM Check if PowerShell is available
where powershell >nul 2>nul
if %errorlevel% neq 0 (
    echo ERROR: PowerShell is not available on this system.
    pause
    exit /b 1
)

REM Execute the PowerShell checker script
powershell.exe -ExecutionPolicy Bypass -File "%~dp0check-prerequisites.ps1"
