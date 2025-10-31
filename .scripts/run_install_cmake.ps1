# Temporary helper to run only the Install-CMake step from install.ps1
# This avoids running the whole installer. Intended for debugging in CI or dev environments.

$ErrorActionPreference = 'Stop'

function Write-Info { param($Message) Write-Host "[INFO] $Message" -ForegroundColor Cyan }
function Write-Success { param($Message) Write-Host "[SUCCESS] $Message" -ForegroundColor Green }
function Write-ErrorMsg { param($Message) Write-Host "[ERROR] $Message" -ForegroundColor Red }
function Write-Step { param($Message) Write-Host "`n==> $Message" -ForegroundColor Magenta }

function Install-CMake {
    Write-Step "Downloading CMake..."
    $cmakeVersion = "3.28.1"
    $cmakeUrl = "https://github.com/Kitware/CMake/releases/download/v$cmakeVersion/cmake-$cmakeVersion-windows-x86_64.msi"
    $cmakeInstaller = Join-Path $env:TEMP "cmake-installer.msi"

    try {
        Write-Info "Downloading: $cmakeUrl"
        Invoke-WebRequest -Uri $cmakeUrl -OutFile $cmakeInstaller -UseBasicParsing -Verbose
        Write-Info "Installing CMake (msiexec will be invoked)..."
        $args = "/i", $cmakeInstaller, "/quiet", "/norestart"
        Start-Process msiexec.exe -ArgumentList $args -Wait -NoNewWindow

        # Add to PATH for current session
        $cmakePath = Join-Path $env:ProgramFiles "CMake\bin"
        if (Test-Path $cmakePath) {
            $env:Path = "$cmakePath;$env:Path"
        }

        Write-Success "CMake install step completed"
        return $true
    }
    catch {
        Write-ErrorMsg "Failed to install CMake: $_"
        return $false
    }
}

# Execute
Install-CMake
