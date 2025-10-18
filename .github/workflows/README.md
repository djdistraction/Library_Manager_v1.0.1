# GitHub Actions CI/CD Workflows

This directory contains the GitHub Actions workflows for continuous integration and deployment of the Library Manager application.

## Workflows

### CI Workflow (`ci.yml`)
**Status**: [![CI](https://github.com/djdistraction/Library_Manager/actions/workflows/ci.yml/badge.svg)](https://github.com/djdistraction/Library_Manager/actions/workflows/ci.yml)

The main CI workflow that builds the application for Windows.

**Triggers:**
- Push to `main` or `develop` branches
- Pull requests to `main` or `develop` branches
- Manual dispatch

**Platform:**
- Windows (latest)

**Features:**
- Automated dependency installation via vcpkg
- Windows-specific build configuration
- Artifact uploads for Windows builds
- Visual Studio 2019+ support

**Dependencies installed via vcpkg:**
- sqlite3:x64-windows

**Additional setup:**
- Microsoft Visual Studio Build Tools
- CMake toolchain configuration for vcpkg

## Firewall Considerations

This workflow is specifically designed to address firewall issues in GitHub Actions runners. All dependency installations and setup steps are configured to run **BEFORE** the firewall is enabled on the runner.

### Key Design Decisions:

1. **Early Dependency Installation**: All `vcpkg` commands run in the early setup steps, ensuring packages are downloaded before any firewall restrictions.

2. **Submodule Checkout**: The JUCE framework and other submodules are checked out recursively in the first step, ensuring all code is available locally.

3. **No Network Access During Build**: After the setup phase, the build process doesn't require network access, making it firewall-safe.

4. **Graceful Fallback**: Optional dependencies (like Chromaprint) have graceful fallback handling - if they can't be installed, the build continues without them.

## Artifacts

Each successful build uploads artifacts that can be downloaded from the Actions tab:

- **windows-build**: Contains the Windows executable (Library Manager.exe)

## Local Testing

To test the build locally on Windows:

### Prerequisites:
- Visual Studio 2019 or higher
- CMake 3.22 or higher
- vcpkg

### Building locally:
```powershell
# Install dependencies
vcpkg install sqlite3:x64-windows

# Clone and build
git clone https://github.com/djdistraction/Library_Manager.git
cd Library_Manager
git submodule update --init JUCE

mkdir build
cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake
cmake --build . --config Release
```

## Maintenance

When adding new dependencies:

1. Add them to the vcpkg installation step
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

### Windows-Specific Issues

- Ensure vcpkg and MSVC are properly configured
- Verify Visual Studio 2019+ is installed
- Check that the CMake toolchain file path is correct

## Contributing

When modifying these workflows:

- Keep dependency installations in early steps
- Add comments explaining non-obvious configurations
- Test on Windows before merging
- Update this README with any changes
