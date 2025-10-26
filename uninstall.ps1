# Library Manager - Uninstaller for Windows
# This script removes Library Manager from your system

param(
    [switch]$KeepDatabase,
    [switch]$NoInteractive
)

$ErrorActionPreference = "Stop"

# Color output helpers
function Write-Info { param($Message) Write-Host "[INFO] $Message" -ForegroundColor Cyan }
function Write-Success { param($Message) Write-Host "[SUCCESS] $Message" -ForegroundColor Green }
function Write-Warning { param($Message) Write-Host "[WARNING] $Message" -ForegroundColor Yellow }
function Write-Error { param($Message) Write-Host "[ERROR] $Message" -ForegroundColor Red }

# Banner
Write-Host @"

╔═══════════════════════════════════════════════════════════════╗
║                                                               ║
║          Library Manager - Uninstaller                        ║
║                                                               ║
╚═══════════════════════════════════════════════════════════════╝

"@ -ForegroundColor Cyan

Write-Warning "This will remove Library Manager from your system."

# Confirm uninstallation
if (-not $NoInteractive) {
    $response = Read-Host "`nAre you sure you want to continue? (y/n)"
    if ($response -ne 'y') {
        Write-Info "Uninstallation cancelled by user"
        exit 0
    }
}

try {
    $itemsRemoved = 0
    
    # Remove installation directory
    $installPath = "$env:LOCALAPPDATA\LibraryManager"
    if (Test-Path $installPath) {
        Write-Info "Removing application files from: $installPath"
        Remove-Item -Path $installPath -Recurse -Force
        $itemsRemoved++
        Write-Success "Application files removed"
    }
    else {
        Write-Info "Application files not found at: $installPath"
    }
    
    # Remove desktop shortcut
    $desktopShortcut = Join-Path ([Environment]::GetFolderPath("Desktop")) "Library Manager.lnk"
    if (Test-Path $desktopShortcut) {
        Write-Info "Removing desktop shortcut"
        Remove-Item -Path $desktopShortcut -Force
        $itemsRemoved++
        Write-Success "Desktop shortcut removed"
    }
    
    # Remove Start Menu shortcut
    $startMenuShortcut = Join-Path ([Environment]::GetFolderPath("StartMenu")) "Programs\Library Manager.lnk"
    if (Test-Path $startMenuShortcut) {
        Write-Info "Removing Start Menu shortcut"
        Remove-Item -Path $startMenuShortcut -Force
        $itemsRemoved++
        Write-Success "Start Menu shortcut removed"
    }
    
    # Handle database
    $databasePath = "$env:APPDATA\LibraryManager"
    if (Test-Path $databasePath) {
        if ($KeepDatabase) {
            Write-Info "Keeping database at: $databasePath"
        }
        else {
            if ($NoInteractive) {
                Write-Info "Removing database at: $databasePath"
                Remove-Item -Path $databasePath -Recurse -Force
                $itemsRemoved++
                Write-Success "Database removed"
            }
            else {
                $response = Read-Host "`nDo you want to remove your library database? This cannot be undone. (y/n)"
                if ($response -eq 'y') {
                    Write-Info "Removing database at: $databasePath"
                    Remove-Item -Path $databasePath -Recurse -Force
                    $itemsRemoved++
                    Write-Success "Database removed"
                }
                else {
                    Write-Info "Database kept at: $databasePath"
                }
            }
        }
    }
    
    # Summary
    Write-Host "`n"
    if ($itemsRemoved -eq 0) {
        Write-Info "No Library Manager files were found on this system."
    }
    else {
        Write-Success "Uninstallation completed successfully!"
        Write-Info "Removed $itemsRemoved item(s)"
    }
    
    Write-Info "`nNote: Build files in the repository directory were not removed."
    Write-Info "You can manually delete the 'build' folder if desired."
    Write-Info "`nNote: vcpkg was not removed as it may be used by other applications."
    Write-Info "If you want to remove vcpkg, delete: $env:USERPROFILE\vcpkg"
    
    if (-not $NoInteractive) {
        Write-Host "`n"
        Read-Host "Press Enter to exit"
    }
}
catch {
    Write-Error "Uninstallation failed: $_"
    exit 1
}
