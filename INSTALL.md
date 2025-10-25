# Library Manager - One-Click Installer

This installer automates the complete setup, build, and installation process of Library Manager on Windows.

## 🚀 Quick Start

### Option 1: Double-Click Installation (Recommended)

1. **Download or clone** this repository
2. **Double-click** `install.bat`
3. Follow the on-screen prompts
4. Wait for the installation to complete
5. Launch Library Manager from the desktop shortcut or Start Menu

### Option 2: PowerShell Installation

Open PowerShell in the repository directory and run:

```powershell
.\install.ps1
```

## 📋 What the Installer Does

The installer automates the following tasks:

1. ✅ **Checks prerequisites**
   - Git
   - CMake (offers to install if missing)
   - Visual Studio with C++ tools

2. ✅ **Sets up vcpkg**
   - Clones vcpkg repository if not present
   - Bootstraps vcpkg package manager

3. ✅ **Installs dependencies**
   - SQLite3 library via vcpkg

4. ✅ **Initializes JUCE framework**
   - Downloads JUCE submodule

5. ✅ **Builds the application**
   - Configures CMake with proper settings
   - Compiles the application in Release mode

6. ✅ **Installs Library Manager**
   - Copies executable to installation directory
   - Creates desktop shortcut
   - Creates Start Menu entry

## 📝 Prerequisites

Before running the installer, ensure you have:

### Required (Must Be Installed Manually)

- **Git for Windows**: Download from [https://git-scm.com/download/win](https://git-scm.com/download/win)
- **Visual Studio 2019 or later** with "Desktop development with C++" workload
  - Download from [https://visualstudio.microsoft.com/downloads/](https://visualstudio.microsoft.com/downloads/)
  - During installation, select the "Desktop development with C++" workload
  - Visual Studio Community Edition (free) is sufficient

### Optional (Can Be Auto-Installed)

- **CMake 3.22 or higher**: The installer can download and install this automatically
  - Or download manually from [https://cmake.org/download/](https://cmake.org/download/)

## 🎯 System Requirements

- **Operating System**: Windows 10 (64-bit) or Windows 11
- **Processor**: Intel Core i3 or AMD equivalent
- **Memory**: 4 GB RAM (8 GB recommended)
- **Storage**: 2 GB free space (for build files and dependencies)
- **Graphics**: DirectX 11 compatible graphics card

## ⚙️ Advanced Usage

### Command-Line Options

```powershell
# Install to a custom location
.\install.ps1 -InstallPath "C:\Program Files\LibraryManager"

# Non-interactive mode (for automation)
.\install.ps1 -NoInteractive

# Skip prerequisite checks (not recommended)
.\install.ps1 -SkipChecks
```

### Manual Installation

If you prefer to install manually, follow these steps:

```powershell
# 1. Clone the repository
git clone https://github.com/djdistraction/Library_Manager.git
cd Library_Manager

# 2. Initialize JUCE submodule
git submodule update --init --recursive JUCE

# 3. Setup vcpkg (if not already installed)
git clone https://github.com/microsoft/vcpkg.git C:\vcpkg
cd C:\vcpkg
.\bootstrap-vcpkg.bat

# 4. Install SQLite3
.\vcpkg install sqlite3:x64-windows

# 5. Build the project
cd Library_Manager
mkdir build
cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE=C:\vcpkg\scripts\buildsystems\vcpkg.cmake
cmake --build . --config Release

# 6. Run the application
..\bin\Release\"Library Manager.exe"
```

## 🔧 Troubleshooting

### "Git is not installed"

Install Git from [https://git-scm.com/download/win](https://git-scm.com/download/win), then restart the installer.

### "Visual Studio with C++ tools is not installed"

1. Download Visual Studio from [https://visualstudio.microsoft.com/downloads/](https://visualstudio.microsoft.com/downloads/)
2. During installation, select the "Desktop development with C++" workload
3. Complete the installation and restart the installer

### "CMake is not installed"

The installer will offer to download and install CMake automatically. If automatic installation fails, download manually from [https://cmake.org/download/](https://cmake.org/download/).

### Build Failures

If the build fails:

1. Ensure all prerequisites are correctly installed
2. Check that you have enough disk space (at least 2 GB free)
3. Try running the installer as Administrator
4. Check the error messages for specific issues

### vcpkg Issues

If vcpkg setup fails:

1. Delete the vcpkg directory (default: `C:\Users\YourName\vcpkg`)
2. Run the installer again
3. Ensure you have a stable internet connection

### Permission Errors

If you encounter permission errors:

1. Right-click `install.bat` and select "Run as Administrator"
2. Or open PowerShell as Administrator and run `.\install.ps1`

## 📂 Installation Locations

By default, the installer uses these locations:

- **Application**: `%LOCALAPPDATA%\LibraryManager\` (e.g., `C:\Users\YourName\AppData\Local\LibraryManager\`)
- **vcpkg**: `%USERPROFILE%\vcpkg\` (e.g., `C:\Users\YourName\vcpkg\`)
- **Build files**: `.\build\` (in the repository directory)
- **Database**: `%APPDATA%\LibraryManager\library.db` (created on first run)

## 🗑️ Uninstallation

### Automatic Uninstallation (Recommended)

Simply double-click `uninstall.bat` or run:

```powershell
.\uninstall.ps1
```

The uninstaller will:
- Remove the application files
- Delete desktop and Start Menu shortcuts
- Optionally remove the database (you'll be asked)

**Options:**
```powershell
# Keep the database (don't ask about removing it)
.\uninstall.ps1 -KeepDatabase

# Silent uninstallation (removes everything including database)
.\uninstall.ps1 -NoInteractive
```

### Manual Uninstallation

If you prefer to uninstall manually:

1. Delete the installation directory (default: `%LOCALAPPDATA%\LibraryManager\`)
2. Delete desktop shortcut
3. Delete Start Menu shortcut
4. (Optional) Delete database at `%APPDATA%\LibraryManager\`
5. (Optional) Delete build directory in repository
6. (Optional) Delete vcpkg directory if no longer needed

## 🆘 Getting Help

If you encounter issues:

1. Check the [Troubleshooting](#-troubleshooting) section above
2. Review the main [README.md](README.md) for additional information
3. Open an issue on GitHub: [https://github.com/djdistraction/Library_Manager/issues](https://github.com/djdistraction/Library_Manager/issues)

## 📄 License

This installer script is part of Library Manager and is licensed under GPLv3.

---

**uniQuE-ui Library Manager** - Professional music library management for DJs and audio professionals.
