# Installation Scripts Overview

This document provides an overview of all the installation-related scripts in the Library Manager repository.

## Files Overview

### 🚀 Installation Scripts

#### `install.bat`
- **Type**: Windows Batch File
- **Purpose**: Easy double-click launcher for the PowerShell installer
- **Usage**: Double-click to start installation
- **Requirements**: Windows 10/11 with PowerShell

#### `install.ps1`
- **Type**: PowerShell Script
- **Purpose**: Main installation script with all automation logic
- **Features**:
  - Prerequisite checking (Git, CMake, Visual Studio)
  - Auto-install CMake option
  - vcpkg setup and bootstrapping
  - SQLite3 installation via vcpkg
  - JUCE submodule initialization
  - Automated CMake configuration
  - Release build compilation
  - Application installation
  - Desktop and Start Menu shortcut creation
- **Usage**: `.\install.ps1` or via `install.bat`
- **Options**:
  - `-InstallPath <path>`: Custom installation location
  - `-NoInteractive`: Silent/automated mode
  - `-SkipChecks`: Skip prerequisite validation (not recommended)

### ✅ Prerequisites Checker

#### `check-prerequisites.bat`
- **Type**: Windows Batch File
- **Purpose**: Launcher for the prerequisites checker
- **Usage**: Double-click to check system requirements

#### `check-prerequisites.ps1`
- **Type**: PowerShell Script
- **Purpose**: Verify system meets installation requirements
- **Checks**:
  - Git installation and version
  - CMake installation and version
  - Visual Studio with C++ tools
  - PowerShell version
  - Available disk space
- **Usage**: `.\check-prerequisites.ps1` or via `check-prerequisites.bat`
- **Benefit**: Identify missing requirements before attempting installation

### 🗑️ Uninstallation Scripts

#### `uninstall.bat`
- **Type**: Windows Batch File
- **Purpose**: Easy double-click launcher for uninstaller
- **Usage**: Double-click to start uninstallation

#### `uninstall.ps1`
- **Type**: PowerShell Script
- **Purpose**: Clean removal of Library Manager
- **Features**:
  - Removes application files
  - Deletes desktop shortcut
  - Deletes Start Menu shortcut
  - Optional database removal (with confirmation)
- **Usage**: `.\uninstall.ps1` or via `uninstall.bat`
- **Options**:
  - `-KeepDatabase`: Don't remove the library database
  - `-NoInteractive`: Silent uninstallation (removes everything)

## Documentation Files

### `INSTALL.md`
- **Purpose**: Comprehensive installation documentation
- **Contents**:
  - Quick start guide
  - Detailed prerequisites list
  - What the installer does
  - System requirements
  - Advanced usage options
  - Troubleshooting guide
  - Installation locations
  - Uninstallation instructions

### `QUICKSTART.md`
- **Purpose**: User-friendly quick start guide
- **Contents**:
  - Simple 3-step installation process
  - Prerequisites checklist
  - Installation timeline and requirements
  - Common issues and solutions
  - After-installation guidance

### `README.md` (Updated)
- **Changes**: Added prominent section featuring the one-click installer
- **Location**: Top of the Installation section

## Typical Workflows

### First-Time User
1. **Check Prerequisites**: Run `check-prerequisites.bat`
2. **Install**: Run `install.bat`
3. **Use**: Launch from desktop or Start Menu
4. **Uninstall** (if needed): Run `uninstall.bat`

### Advanced User
1. **Custom Install**: `.\install.ps1 -InstallPath "C:\Custom\Path"`
2. **Silent Install**: `.\install.ps1 -NoInteractive`
3. **Keep Database**: `.\uninstall.ps1 -KeepDatabase`

### CI/CD or Automation
```powershell
# Check prerequisites programmatically
.\check-prerequisites.ps1

# Silent installation
.\install.ps1 -NoInteractive -InstallPath "C:\ProgramData\LibraryManager"

# Silent uninstallation
.\uninstall.ps1 -NoInteractive
```

## Error Handling

All scripts include comprehensive error handling:

- **Colored Output**: Uses colors to indicate success (green), warnings (yellow), and errors (red)
- **Clear Messages**: Provides specific error messages and remediation steps
- **Exit Codes**: Returns appropriate exit codes for automation
- **Graceful Failures**: Allows users to continue or abort on errors

## Prerequisites

### Required (Must Install Manually)
- **Git for Windows**: https://git-scm.com/download/win
- **Visual Studio 2019+** with "Desktop development with C++" workload
  - Visual Studio Community (free) is sufficient
  - Download: https://visualstudio.microsoft.com/downloads/

### Optional (Can Be Auto-Installed)
- **CMake 3.22+**: The installer offers to download and install automatically

### Automatically Handled
- **vcpkg**: Installed and configured by the installer
- **SQLite3**: Installed via vcpkg
- **JUCE**: Initialized as Git submodule

## Technical Details

### Installation Locations
- **Application**: `%LOCALAPPDATA%\LibraryManager\` (default)
- **vcpkg**: `%USERPROFILE%\vcpkg\` (default)
- **Build Files**: `.\build\` (in repository)
- **Database**: `%APPDATA%\LibraryManager\library.db` (created on first run)

### Build Configuration
- **CMake Generator**: Visual Studio (auto-detected)
- **Build Type**: Release
- **Architecture**: x64 (64-bit)
- **Parallel Build**: Enabled for faster compilation

### Time Estimates
- **Prerequisites Check**: < 1 minute
- **First Installation**: 15-30 minutes
  - vcpkg setup: 5 minutes
  - Dependencies: 5-10 minutes
  - Build: 10-20 minutes
  - Installation: < 1 minute
- **Uninstallation**: < 1 minute

## Security Considerations

- Scripts use `ExecutionPolicy Bypass` to run without changing system policy
- No elevation required (runs in user context)
- All downloads from official sources (CMake, vcpkg)
- No data collection or telemetry
- Database optionally preserved on uninstallation

## Support

For issues or questions:
1. Check the troubleshooting section in `INSTALL.md`
2. Review `QUICKSTART.md` for common scenarios
3. Open an issue: https://github.com/djdistraction/Library_Manager/issues

## License

These installation scripts are part of Library Manager and licensed under GPLv3.

---

**Quick Reference**:
- Check system: `check-prerequisites.bat`
- Install: `install.bat`
- Uninstall: `uninstall.bat`
- Documentation: `INSTALL.md` and `QUICKSTART.md`
