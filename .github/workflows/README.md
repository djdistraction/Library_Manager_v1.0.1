# GitHub Actions CI/CD Workflows

This directory contains the GitHub Actions workflows for continuous integration and deployment of the Library Manager application.

## Workflows

### 1. CI Workflow (`ci.yml`)
**Status**: [![CI](https://github.com/djdistraction/Library_Manager/actions/workflows/ci.yml/badge.svg)](https://github.com/djdistraction/Library_Manager/actions/workflows/ci.yml)

The main CI workflow that builds the application on all supported platforms using a matrix strategy.

**Triggers:**
- Push to `main` or `develop` branches
- Pull requests to `main` or `develop` branches
- Manual dispatch

**Platforms:**
- Ubuntu (Linux)
- macOS
- Windows

**Features:**
- Multi-platform build matrix
- Automated dependency installation
- Artifact uploads for each platform
- Fail-fast disabled to see results from all platforms

### 2. Platform-Specific Workflows

#### Build Linux (`build-linux.yml`)
Builds the application specifically for Linux systems.

**Dependencies installed:**
- X11 libraries (libx11-dev, libxrandr-dev, libxinerama-dev, libxcursor-dev)
- Graphics libraries (libfreetype-dev, libgl1-mesa-dev, libfontconfig1-dev)
- Audio libraries (libasound2-dev)
- Additional X11 libraries (libxext-dev, libxrender-dev, libxcomposite-dev)
- SQLite3 (libsqlite3-dev)
- Optional: Chromaprint for audio fingerprinting

#### Build macOS (`build-macos.yml`)
Builds the application specifically for macOS systems.

**Dependencies installed via Homebrew:**
- cmake
- sqlite3
- pkg-config
- Optional: chromaprint

#### Build Windows (`build-windows.yml`)
Builds the application specifically for Windows systems.

**Dependencies installed via vcpkg:**
- sqlite3:x64-windows

**Additional setup:**
- Microsoft Visual Studio Build Tools
- CMake toolchain configuration for vcpkg

## Firewall Considerations

These workflows are specifically designed to address firewall issues in GitHub Actions runners. All dependency installations and setup steps are configured to run **BEFORE** the firewall is enabled on the runner.

### Key Design Decisions:

1. **Early Dependency Installation**: All `apt-get`, `brew`, and `vcpkg` commands run in the early setup steps, ensuring packages are downloaded before any firewall restrictions.

2. **Submodule Checkout**: The JUCE framework and other submodules are checked out recursively in the first step, ensuring all code is available locally.

3. **No Network Access During Build**: After the setup phase, the build process doesn't require network access, making it firewall-safe.

4. **Graceful Fallback**: Optional dependencies (like Chromaprint) have graceful fallback handling - if they can't be installed, the build continues without them.

## Artifacts

Each successful build uploads artifacts that can be downloaded from the Actions tab:

- **linux-build**: Contains the Linux executable
- **macos-build**: Contains the macOS application bundle
- **windows-build**: Contains the Windows executable

## Local Testing

To test the workflows locally before pushing, you can use [act](https://github.com/nektos/act) - a tool that runs GitHub Actions locally.

### Installing act:

**Linux:**
```bash
# Using package manager (recommended):
# For Debian/Ubuntu (if available in repos)
sudo apt-get install act

# Using the install script (verify the script first):
# Visit https://github.com/nektos/act/blob/master/install.sh to review
curl https://raw.githubusercontent.com/nektos/act/master/install.sh | sudo bash

# Or download and install manually (most secure):
# 1. Download from https://github.com/nektos/act/releases
# 2. Verify checksums
# 3. Extract and move to /usr/local/bin/
```

**macOS:**
```bash
brew install act
```

**Windows:**
```powershell
# Using Chocolatey
choco install act-cli

# Or using Scoop
scoop install act

# Or download from https://github.com/nektos/act/releases
```

### Running workflows locally:

```bash
# Run the CI workflow
act -W .github/workflows/ci.yml

# Run a specific platform workflow
act -W .github/workflows/build-linux.yml

# Run with verbose output
act -v -W .github/workflows/ci.yml
```

**Note:** Local testing with `act` may not perfectly replicate the GitHub Actions environment, especially regarding firewall behavior. The workflows have been designed to work in the actual GitHub Actions environment.

## Maintenance

When adding new dependencies:

1. Add them to the appropriate platform's dependency installation step
2. Ensure they're installed BEFORE any build commands
3. Add graceful fallback handling for optional dependencies
4. Update this README with the new dependency information

## Troubleshooting

### Build Failures

If a build fails:

1. Check the Actions tab for detailed logs
2. Verify all dependencies are listed in the workflow
3. Ensure the CMake configuration step completes successfully
4. Check that submodules are properly initialized

### Firewall Issues

If you encounter firewall-related errors:

1. Ensure all network-dependent operations are in the setup steps (before the "Configure CMake" step)
2. Verify no build commands are trying to download packages
3. Check that all submodules are included in the initial checkout

### Platform-Specific Issues

- **Linux**: Verify all required X11 and audio libraries are installed
- **macOS**: Check Homebrew installation logs for any issues
- **Windows**: Ensure vcpkg and MSVC are properly configured

## Contributing

When modifying these workflows:

- Keep dependency installations in early steps
- Add comments explaining non-obvious configurations
- Test on all platforms before merging
- Update this README with any changes
