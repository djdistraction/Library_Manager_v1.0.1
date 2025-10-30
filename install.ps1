# Library Manager - One-Click Install Wizard for Windows
# This script automates the complete setup, build, and installation process

param(
    [switch]$SkipChecks,
    [switch]$NoInteractive,
    [string]$InstallPath = "$env:LOCALAPPDATA\LibraryManager"
)

# Set error action preference
$ErrorActionPreference = "Stop"

# Color output helpers
function Write-Info { param($Message) Write-Host "[INFO] $Message" -ForegroundColor Cyan }
function Write-Success { param($Message) Write-Host "[SUCCESS] $Message" -ForegroundColor Green }
function Write-Warning { param($Message) Write-Host "[WARNING] $Message" -ForegroundColor Yellow }
function Write-Error { param($Message) Write-Host "[ERROR] $Message" -ForegroundColor Red }
function Write-Step { param($Message) Write-Host "`n==> $Message" -ForegroundColor Magenta }

# Banner
Write-Host @"

╔═══════════════════════════════════════════════════════════════╗
║                                                               ║
║          Library Manager - One-Click Install Wizard           ║
║                                                               ║
║              For Windows 10/11 (64-bit)                       ║
║                                                               ║
╚═══════════════════════════════════════════════════════════════╝

"@ -ForegroundColor Cyan

Write-Info "Starting installation process..."
Write-Info "Install location: $InstallPath"

# Function to check if a command exists
function Test-Command {
    param($Command)
    try {
        if (Get-Command $Command -ErrorAction SilentlyContinue) {
            return $true
        }
        return $false
    }
    catch {
        return $false
    }
}

# Function to check Visual Studio installation
function Test-VisualStudio {
    $vsWherePath = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
    
    if (Test-Path $vsWherePath) {
        $vsInstances = & $vsWherePath -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -format json | ConvertFrom-Json
        if ($vsInstances) {
            return $true, $vsInstances.installationPath
        }
    }
    
    return $false, $null
}

# Function to download and install CMake
function Install-CMake {
    Write-Step "Downloading CMake..."
    $cmakeVersion = "3.28.1"
    $cmakeUrl = "https://github.com/Kitware/CMake/releases/download/v$cmakeVersion/cmake-$cmakeVersion-windows-x86_64.msi"
    $cmakeInstaller = "$env:TEMP\cmake-installer.msi"
    
    try {
        Invoke-WebRequest -Uri $cmakeUrl -OutFile $cmakeInstaller -UseBasicParsing
        Write-Info "Installing CMake..."
        Start-Process msiexec.exe -ArgumentList "/i", $cmakeInstaller, "/quiet", "/norestart" -Wait
        
        # Add to PATH for current session
        $cmakePath = "${env:ProgramFiles}\CMake\bin"
        if (Test-Path $cmakePath) {
            $env:Path = "$cmakePath;$env:Path"
        }
        
        Write-Success "CMake installed successfully"
        return $true
    }
    catch {
        Write-Error "Failed to install CMake: $_"
        return $false
    }
}

# Function to setup vcpkg
function Install-Vcpkg {
    param($VcpkgPath)
    
    Write-Step "Setting up vcpkg package manager..."
    
    if (Test-Path $VcpkgPath) {
        Write-Info "vcpkg already exists at: $VcpkgPath"
    }
    else {
        Write-Info "Cloning vcpkg repository..."
        git clone https://github.com/microsoft/vcpkg.git $VcpkgPath
        
        if ($LASTEXITCODE -ne 0) {
            throw "Failed to clone vcpkg repository"
        }
    }
    
    Write-Info "Bootstrapping vcpkg..."
    Push-Location $VcpkgPath
    
    if (-not (Test-Path "vcpkg.exe")) {
        & .\bootstrap-vcpkg.bat
        
        if ($LASTEXITCODE -ne 0) {
            Pop-Location
            throw "Failed to bootstrap vcpkg"
        }
    }
    
    Pop-Location
    Write-Success "vcpkg setup complete"
}

# Function to install dependencies via vcpkg
function Install-Dependencies {
    param($VcpkgPath)
    
    Write-Step "Installing SQLite3 dependency via vcpkg..."
    
    $vcpkgExe = Join-Path $VcpkgPath "vcpkg.exe"
    
    Write-Info "Installing sqlite3:x64-windows..."
    & $vcpkgExe install sqlite3:x64-windows
    
    if ($LASTEXITCODE -ne 0) {
        throw "Failed to install sqlite3"
    }
    
    Write-Success "Dependencies installed successfully"
}

# Function to initialize JUCE submodule
function Initialize-Submodules {
    Write-Step "Initializing JUCE framework submodule..."
    
    git submodule update --init --recursive JUCE
    
    if ($LASTEXITCODE -ne 0) {
        throw "Failed to initialize JUCE submodule"
    }
    
    Write-Success "JUCE submodule initialized"
}

# Function to build the project
function Build-Project {
    param($VcpkgPath, $BuildPath)
    
    Write-Step "Configuring project with CMake..."
    
    $vcpkgToolchain = Join-Path $VcpkgPath "scripts\buildsystems\vcpkg.cmake"
    
    if (-not (Test-Path $BuildPath)) {
        New-Item -ItemType Directory -Path $BuildPath | Out-Null
    }
    
    Push-Location $BuildPath
    
    Write-Info "Running CMake configuration..."
    # Invoke CMake with the call operator and quoted arguments so PowerShell expands the
    # `$vcpkgToolchain` variable correctly even if the path contains spaces.
    & cmake .. "-DCMAKE_BUILD_TYPE=Release" "-DCMAKE_TOOLCHAIN_FILE=$vcpkgToolchain"
    
    if ($LASTEXITCODE -ne 0) {
        Pop-Location
        throw "CMake configuration failed"
    }
    
    Write-Step "Building project (this may take several minutes)..."
    cmake --build . --config Release --parallel
    
    if ($LASTEXITCODE -ne 0) {
        Pop-Location
        throw "Build failed"
    }
    
    Pop-Location
    Write-Success "Build completed successfully"
}

# Function to create installation
function Install-Application {
    param($SourcePath, $InstallPath)
    
    Write-Step "Installing Library Manager..."
    
    # Create install directory
    if (-not (Test-Path $InstallPath)) {
        New-Item -ItemType Directory -Path $InstallPath -Force | Out-Null
    }
    
    # Copy executable and dependencies
    $exePath = Join-Path $SourcePath "bin\Release\Library Manager.exe"
    
    if (Test-Path $exePath) {
        Copy-Item $exePath -Destination $InstallPath -Force
        Write-Success "Application installed to: $InstallPath"
    }
    else {
        throw "Built executable not found at: $exePath"
    }
    
    # Create desktop shortcut
    $shortcutPath = Join-Path ([Environment]::GetFolderPath("Desktop")) "Library Manager.lnk"
    $WScriptShell = New-Object -ComObject WScript.Shell
    $Shortcut = $WScriptShell.CreateShortcut($shortcutPath)
    $Shortcut.TargetPath = Join-Path $InstallPath "Library Manager.exe"
    $Shortcut.WorkingDirectory = $InstallPath
    $Shortcut.Description = "uniQuE-ui Library Manager"
    $Shortcut.Save()
    
    Write-Success "Desktop shortcut created"
    
    # Create Start Menu shortcut
    $startMenuPath = Join-Path ([Environment]::GetFolderPath("StartMenu")) "Programs\Library Manager.lnk"
    $Shortcut = $WScriptShell.CreateShortcut($startMenuPath)
    $Shortcut.TargetPath = Join-Path $InstallPath "Library Manager.exe"
    $Shortcut.WorkingDirectory = $InstallPath
    $Shortcut.Description = "uniQuE-ui Library Manager"
    $Shortcut.Save()
    
    Write-Success "Start Menu shortcut created"
}

# Main installation flow
try {
    # Check if running as Administrator (recommended but not required)
    $isAdmin = ([Security.Principal.WindowsPrincipal] [Security.Principal.WindowsIdentity]::GetCurrent()).IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)
    
    if (-not $isAdmin) {
        Write-Warning "Not running as Administrator. Some operations might require elevation."
        if (-not $NoInteractive) {
            $response = Read-Host "Continue anyway? (y/n)"
            if ($response -ne 'y') {
                Write-Info "Installation cancelled by user"
                exit 0
            }
        }
    }
    
    # Step 1: Check prerequisites
    Write-Step "Checking prerequisites..."
    
    $gitInstalled = Test-Command "git"
    $cmakeInstalled = Test-Command "cmake"
    $vsInstalled, $vsPath = Test-VisualStudio
    
    Write-Info "Git installed: $gitInstalled"
    Write-Info "CMake installed: $cmakeInstalled"
    Write-Info "Visual Studio installed: $vsInstalled"
    
    # Check Git
    if (-not $gitInstalled) {
        Write-Error "Git is not installed. Please install Git from https://git-scm.com/download/win"
        Write-Info "After installing Git, restart this script."
        exit 1
    }
    
    # Check/Install CMake
    if (-not $cmakeInstalled) {
        Write-Warning "CMake is not installed"
        if (-not $NoInteractive) {
            $response = Read-Host "Would you like to install CMake automatically? (y/n)"
            if ($response -eq 'y') {
                if (-not (Install-CMake)) {
                    exit 1
                }
                $cmakeInstalled = Test-Command "cmake"
            }
            else {
                Write-Error "CMake is required. Please install it from https://cmake.org/download/"
                exit 1
            }
        }
        else {
            Write-Error "CMake is required but automatic installation is disabled in non-interactive mode"
            exit 1
        }
    }
    
    # Check Visual Studio
    if (-not $vsInstalled) {
        Write-Error "Visual Studio with C++ tools is not installed"
        Write-Info "Please install Visual Studio 2019 or later with 'Desktop development with C++' workload"
        Write-Info "Download from: https://visualstudio.microsoft.com/downloads/"
        exit 1
    }
    
    Write-Success "All prerequisites met"
    
    # Step 2: Setup vcpkg
    $vcpkgPath = Join-Path $env:USERPROFILE "vcpkg"
    
    if (-not $NoInteractive) {
        Write-Info "`nvcpkg will be installed to: $vcpkgPath"
        $response = Read-Host "Use a different path? (press Enter to continue or type new path)"
        if ($response) {
            $vcpkgPath = $response
        }
    }
    
    Install-Vcpkg -VcpkgPath $vcpkgPath
    
    # Step 3: Install dependencies
    Install-Dependencies -VcpkgPath $vcpkgPath
    
    # Step 4: Initialize submodules
    Initialize-Submodules
    
    # Step 5: Build project
    $buildPath = Join-Path $PSScriptRoot "build"
    Build-Project -VcpkgPath $vcpkgPath -BuildPath $buildPath
    
    # Step 6: Install application
    Install-Application -SourcePath $buildPath -InstallPath $InstallPath
    
    # Success!
    Write-Host @"

╔═══════════════════════════════════════════════════════════════╗
║                                                               ║
║            Installation Completed Successfully!               ║
║                                                               ║
╚═══════════════════════════════════════════════════════════════╝

"@ -ForegroundColor Green
    
    Write-Success "Library Manager has been installed to: $InstallPath"
    Write-Success "Desktop shortcut created"
    Write-Success "Start Menu shortcut created"
    Write-Info "`nYou can now launch Library Manager from:"
    Write-Info "  - Desktop shortcut"
    Write-Info "  - Start Menu"
    Write-Info "  - Direct path: $InstallPath\Library Manager.exe"
    
    if (-not $NoInteractive) {
        $response = Read-Host "`nWould you like to launch Library Manager now? (y/n)"
        if ($response -eq 'y') {
            Start-Process (Join-Path $InstallPath "Library Manager.exe")
        }
    }
}
catch {
    Write-Error "Installation failed: $_"
    Write-Info "`nFor help, please visit: https://github.com/djdistraction/Library_Manager/issues"
    exit 1
}
