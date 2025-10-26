# Library Manager - Prerequisites Checker
# This script checks if your system meets the requirements for installation

$ErrorActionPreference = "Continue"

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

function Test-VisualStudio {
    $vsWherePath = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
    
    if (Test-Path $vsWherePath) {
        $vsInstances = & $vsWherePath -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -format json | ConvertFrom-Json
        if ($vsInstances) {
            return $true, $vsInstances.displayName, $vsInstances.installationVersion
        }
    }
    
    return $false, $null, $null
}

Write-Host @"

╔═══════════════════════════════════════════════════════════════╗
║                                                               ║
║          Library Manager - Prerequisites Checker              ║
║                                                               ║
╚═══════════════════════════════════════════════════════════════╝

"@ -ForegroundColor Cyan

Write-Host "Checking your system for installation requirements...`n" -ForegroundColor Cyan

$allGood = $true

# Check Git
Write-Host "[1/4] Checking Git..." -ForegroundColor Yellow
if (Test-Command "git") {
    $gitVersion = (git --version) -replace 'git version ', ''
    Write-Host "  ✓ Git installed: $gitVersion" -ForegroundColor Green
}
else {
    Write-Host "  ✗ Git is NOT installed" -ForegroundColor Red
    Write-Host "    Download from: https://git-scm.com/download/win" -ForegroundColor Gray
    $allGood = $false
}

# Check CMake
Write-Host "`n[2/4] Checking CMake..." -ForegroundColor Yellow
if (Test-Command "cmake") {
    $cmakeVersion = (cmake --version).Split("`n")[0] -replace 'cmake version ', ''
    Write-Host "  ✓ CMake installed: $cmakeVersion" -ForegroundColor Green
}
else {
    Write-Host "  ⚠ CMake is NOT installed (can be auto-installed)" -ForegroundColor Yellow
    Write-Host "    The installer can download CMake automatically" -ForegroundColor Gray
}

# Check Visual Studio
Write-Host "`n[3/4] Checking Visual Studio..." -ForegroundColor Yellow
$vsInstalled, $vsName, $vsVersion = Test-VisualStudio
if ($vsInstalled) {
    Write-Host "  ✓ Visual Studio with C++ tools: $vsName ($vsVersion)" -ForegroundColor Green
}
else {
    Write-Host "  ✗ Visual Studio with C++ tools is NOT installed" -ForegroundColor Red
    Write-Host "    Download from: https://visualstudio.microsoft.com/downloads/" -ForegroundColor Gray
    Write-Host "    Make sure to select 'Desktop development with C++' workload" -ForegroundColor Gray
    $allGood = $false
}

# Check PowerShell version
Write-Host "`n[4/4] Checking PowerShell..." -ForegroundColor Yellow
$psVersion = $PSVersionTable.PSVersion
if ($psVersion.Major -ge 5) {
    Write-Host "  ✓ PowerShell version: $($psVersion.Major).$($psVersion.Minor)" -ForegroundColor Green
}
else {
    Write-Host "  ⚠ PowerShell version: $($psVersion.Major).$($psVersion.Minor) (5.0+ recommended)" -ForegroundColor Yellow
}

# Check disk space
Write-Host "`n[Bonus] Checking disk space..." -ForegroundColor Yellow
$drive = Get-PSDrive -Name C
$freeSpaceGB = [math]::Round($drive.Free / 1GB, 2)
if ($freeSpaceGB -ge 2) {
    Write-Host "  ✓ Available space on C: $freeSpaceGB GB" -ForegroundColor Green
}
else {
    Write-Host "  ⚠ Available space on C: $freeSpaceGB GB (2 GB recommended)" -ForegroundColor Yellow
}

# Summary
Write-Host "`n" + ("=" * 63) -ForegroundColor Cyan
if ($allGood) {
    Write-Host "`n✓ Your system meets all requirements!" -ForegroundColor Green
    Write-Host "`nYou can proceed with the installation by running:" -ForegroundColor Cyan
    Write-Host "  install.bat" -ForegroundColor White
    Write-Host "`nor" -ForegroundColor Cyan
    Write-Host "  .\install.ps1" -ForegroundColor White
}
else {
    Write-Host "`n✗ Some required components are missing" -ForegroundColor Red
    Write-Host "`nPlease install the missing components listed above," -ForegroundColor Yellow
    Write-Host "then run this checker again or proceed with installation." -ForegroundColor Yellow
}

Write-Host "`n" + ("=" * 63) -ForegroundColor Cyan
Write-Host ""
Read-Host "Press Enter to exit"
