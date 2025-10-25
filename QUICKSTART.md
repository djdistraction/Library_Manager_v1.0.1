# Quick Start Installation Guide

## For Windows Users - Simple 3-Step Process

### Optional: Check Prerequisites First

Want to verify your system is ready? Double-click `check-prerequisites.bat` to run a quick check!

### Step 1: Download
Download or clone the Library Manager repository to your computer.

### Step 2: Double-Click
Locate and double-click the `install.bat` file in the repository folder.

### Step 3: Wait
The installer will:
- Check your system
- Download needed components
- Build the application
- Install Library Manager

That's it! Once complete, you'll find Library Manager on your desktop and in your Start Menu.

---

## Before You Start

Make sure you have these installed first:

### ✅ Required (Install These First)

1. **Git for Windows**
   - Download: https://git-scm.com/download/win
   - Use default settings during installation

2. **Visual Studio 2019 or later**
   - Download: https://visualstudio.microsoft.com/downloads/
   - Install the "Desktop development with C++" workload
   - Community Edition (free) works perfectly

### ⚙️ Optional (Can Be Auto-Installed)

- **CMake** - The installer can install this for you automatically

---

## What to Expect

### Installation Time
- First-time installation: **15-30 minutes**
  - Downloading dependencies: 5-10 minutes
  - Building application: 10-20 minutes
  - Installation: < 1 minute

### Disk Space Required
- Temporary build files: ~2 GB
- Final installation: ~100 MB
- Your music library database: Varies

### Internet Connection
An active internet connection is required for:
- Downloading vcpkg (package manager)
- Installing SQLite3 library
- Downloading JUCE framework (if not already present)

---

## Installation Steps (Detailed)

When you run `install.bat`, you'll see:

1. **Welcome Screen** - Colorful banner with installation info

2. **Prerequisite Check**
   - Verifies Git installation
   - Checks for CMake (offers to install if missing)
   - Confirms Visual Studio with C++ tools

3. **vcpkg Setup**
   - Clones vcpkg package manager (if not present)
   - Bootstraps vcpkg executable

4. **Dependency Installation**
   - Installs SQLite3 via vcpkg
   - Shows progress for downloads

5. **JUCE Framework**
   - Initializes JUCE submodule
   - Downloads JUCE if needed

6. **Building**
   - Configures project with CMake
   - Compiles in Release mode
   - Shows build progress

7. **Installation**
   - Copies executable to install location
   - Creates desktop shortcut
   - Creates Start Menu entry

8. **Completion**
   - Shows success message
   - Offers to launch the application

---

## After Installation

You can launch Library Manager from:

- **Desktop** - Look for "Library Manager" shortcut
- **Start Menu** - Search for "Library Manager"
- **Direct** - Located at `%LOCALAPPDATA%\LibraryManager\`

On first launch:
1. You'll see the Onboarding screen
2. Select 2-5 test audio files
3. Run a compatibility test
4. Review the results
5. Click "Complete Setup" to start using Library Manager

---

## Need Help?

### Common Issues

**"Git is not installed"**
- Install Git from https://git-scm.com/download/win
- Restart the installer

**"Visual Studio not found"**
- Ensure you selected "Desktop development with C++" during VS installation
- Re-run Visual Studio Installer to add it if needed

**"Build failed"**
- Check you have at least 2 GB free disk space
- Try running as Administrator
- Check the error message for specific details

**"Permission denied"**
- Right-click `install.bat` and select "Run as Administrator"

### Get Support

For additional help:
- Read the full documentation: [INSTALL.md](INSTALL.md)
- Check the main README: [README.md](README.md)
- Report issues: https://github.com/djdistraction/Library_Manager/issues

---

## Advanced Options

### Custom Installation Path

Edit `install.ps1` and change the `$InstallPath` parameter, or run:

```powershell
.\install.ps1 -InstallPath "C:\Your\Custom\Path"
```

### Silent/Automated Installation

For automated deployment:

```powershell
.\install.ps1 -NoInteractive
```

### Verify Installation

After installation, verify the files:

```
%LOCALAPPDATA%\LibraryManager\
├── Library Manager.exe   (Main application)
```

Desktop and Start Menu shortcuts should also be created.

---

**Ready to install?** Just double-click `install.bat` and follow the prompts!
