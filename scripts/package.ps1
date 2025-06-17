# FakeInfo Hardware Spoofer Packaging Script
param(
    [string]$BuildDir = "build",
    [string]$OutputDir = "dist",
    [string]$Configuration = "Release",
    [string]$Version = "1.0.0"
)

Write-Host "========================================" -ForegroundColor Green
Write-Host "FakeInfo Hardware Spoofer Packaging" -ForegroundColor Green
Write-Host "========================================" -ForegroundColor Green

# 设置错误处理
$ErrorActionPreference = "Stop"

# 获取脚本目录和项目根目录
$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$ProjectRoot = Split-Path -Parent $ScriptDir
$BuildPath = Join-Path $ProjectRoot $BuildDir
$BinPath = Join-Path $BuildPath "bin"
$OutputPath = Join-Path $ProjectRoot $OutputDir

Write-Host "Project Root: $ProjectRoot" -ForegroundColor Cyan
Write-Host "Build Path: $BuildPath" -ForegroundColor Cyan
Write-Host "Output Path: $OutputPath" -ForegroundColor Cyan

# 检查构建目录
if (-not (Test-Path $BinPath)) {
    Write-Error "Build directory not found: $BinPath"
    Write-Host "Please run 'cmake --build build --config Release' first"
    exit 1
}

# 检查主程序文件
$MainExe = Join-Path $BinPath "FakeInfoHardwareSpoofer.exe"
$HookDll = Join-Path $BinPath "HookDLL.dll"

if (-not (Test-Path $MainExe)) {
    Write-Error "Main executable not found: $MainExe"
    exit 1
}

if (-not (Test-Path $HookDll)) {
    Write-Warning "Hook DLL not found: $HookDll"
}

# 创建输出目录
if (Test-Path $OutputPath) {
    Remove-Item $OutputPath -Recurse -Force
}
New-Item -ItemType Directory -Path $OutputPath -Force | Out-Null

# 创建应用程序目录
$AppDir = Join-Path $OutputPath "FakeInfoHardwareSpoofer"
New-Item -ItemType Directory -Path $AppDir -Force | Out-Null

Write-Host "Copying application files..." -ForegroundColor Yellow

# 复制主要文件
Copy-Item $MainExe $AppDir -Force
if (Test-Path $HookDll) {
    Copy-Item $HookDll $AppDir -Force
}

# 复制Qt DLL和其他依赖
$QtDlls = Get-ChildItem $BinPath -Filter "*.dll" | Where-Object { $_.Name -like "Qt5*" -or $_.Name -like "msvcp*" -or $_.Name -like "vcruntime*" }
foreach ($dll in $QtDlls) {
    Copy-Item $dll.FullName $AppDir -Force
    Write-Host "  Copied: $($dll.Name)" -ForegroundColor Gray
}

# 复制Qt平台插件
$PlatformsDir = Join-Path $BinPath "platforms"
if (Test-Path $PlatformsDir) {
    $AppPlatformsDir = Join-Path $AppDir "platforms"
    Copy-Item $PlatformsDir $AppPlatformsDir -Recurse -Force
    Write-Host "  Copied Qt platforms directory" -ForegroundColor Gray
}

# 复制样式插件
$StylesDir = Join-Path $BinPath "styles"
if (Test-Path $StylesDir) {
    $AppStylesDir = Join-Path $AppDir "styles"
    Copy-Item $StylesDir $AppStylesDir -Recurse -Force
    Write-Host "  Copied Qt styles directory" -ForegroundColor Gray
}

# 复制配置文件和文档
$ConfigFiles = @(
    "sample_config.json",
    "README.md",
    "QUICKSTART.md",
    "HOOK_TESTING.md",
    "REGISTRY_HOOK_GUIDE.md"
)

foreach ($file in $ConfigFiles) {
    $srcFile = Join-Path $ProjectRoot $file
    if (Test-Path $srcFile) {
        Copy-Item $srcFile $AppDir -Force
        Write-Host "  Copied: $file" -ForegroundColor Gray
    }
}

# 复制测试程序
$TestExe = Join-Path $BinPath "TestWMI.exe"
if (Test-Path $TestExe) {
    Copy-Item $TestExe $AppDir -Force
    Write-Host "  Copied: TestWMI.exe" -ForegroundColor Gray
}

$TestRegistryExe = Join-Path $BinPath "TestRegistry.exe"
if (Test-Path $TestRegistryExe) {
    Copy-Item $TestRegistryExe $AppDir -Force
    Write-Host "  Copied: TestRegistry.exe" -ForegroundColor Gray
}

# 创建启动脚本
$LaunchScript = @"
@echo off
echo Starting FakeInfo Hardware Spoofer...
echo.
echo WARNING: This application requires administrator privileges.
echo Please run as administrator for full functionality.
echo.
pause
echo.

REM 检查管理员权限
net session >nul 2>&1
if %errorLevel% == 0 (
    echo Running with administrator privileges...
    echo.
    start "" "FakeInfoHardwareSpoofer.exe"
) else (
    echo Requesting administrator privileges...
    powershell -Command "Start-Process -FilePath '%~dp0FakeInfoHardwareSpoofer.exe' -Verb RunAs"
)
"@

$LaunchScript | Out-File -FilePath (Join-Path $AppDir "Launch.bat") -Encoding ASCII

# 创建卸载脚本
$UninstallScript = @"
@echo off
echo FakeInfo Hardware Spoofer Uninstaller
echo.
echo This will remove all application files.
echo.
pause

echo Stopping any running processes...
taskkill /f /im FakeInfoHardwareSpoofer.exe 2>nul

echo Removing application files...
cd /d "%~dp0.."
rmdir /s /q "FakeInfoHardwareSpoofer"

echo Uninstall completed.
pause
"@

$UninstallScript | Out-File -FilePath (Join-Path $OutputPath "Uninstall.bat") -Encoding ASCII

# 创建安装说明
$InstallInstructions = @"
# FakeInfo Hardware Spoofer Installation

## Installation Steps

1. Extract all files to a directory of your choice (e.g., C:\Program Files\FakeInfoHardwareSpoofer)
2. Run Launch.bat as administrator to start the application
3. The application requires administrator privileges for full functionality

## Files Included

- FakeInfoHardwareSpoofer.exe - Main application
- HookDLL.dll - Hook library for hardware spoofing
- TestWMI.exe - WMI testing program
- Qt5*.dll - Qt runtime libraries
- platforms/ - Qt platform plugins
- sample_config.json - Example configuration file
- hardware_config.json - Default configuration file
- README.md - Detailed documentation
- QUICKSTART.md - Quick start guide
- HOOK_TESTING.md - Hook testing guide

## System Requirements

- Windows 10/11 (x64)
- Administrator privileges (for hook functionality)
- Visual C++ Redistributable 2019 or later

## Usage

1. Launch the application using Launch.bat
2. View current system information in the "System Info" tab
3. Configure fake hardware information in the "Hardware Config" tab
4. Use randomization features to generate realistic fake data
5. Enable hooking in the "Control Panel" tab to apply changes
6. Test the hook functionality using TestWMI.exe

## Testing

- Run TestWMI.exe to test WMI queries before and after hooking
- Check hook_log.txt for detailed hook operation logs
- Use PowerShell WMI commands to verify hook effectiveness

## Troubleshooting

- If the application fails to start, ensure all DLL files are present
- For hook functionality issues, run as administrator
- Check the application log for detailed error messages

## Uninstallation

Run Uninstall.bat to remove all application files.
"@

$InstallInstructions | Out-File -FilePath (Join-Path $OutputPath "INSTALL.md") -Encoding UTF8

Write-Host "Creating ZIP archive..." -ForegroundColor Yellow

# 创建ZIP文件
$ZipName = "FakeInfoHardwareSpoofer-v$Version-Windows-x64.zip"
$ZipPath = Join-Path $OutputPath $ZipName

# 使用PowerShell 5.0+ 的压缩功能
if ($PSVersionTable.PSVersion.Major -ge 5) {
    Compress-Archive -Path "$AppDir\*", (Join-Path $OutputPath "INSTALL.md"), (Join-Path $OutputPath "Uninstall.bat") -DestinationPath $ZipPath -Force
}
else {
    # 备用方法：使用.NET Framework
    Add-Type -AssemblyName System.IO.Compression.FileSystem
    [System.IO.Compression.ZipFile]::CreateFromDirectory($AppDir, $ZipPath)
}

Write-Host "Package created successfully!" -ForegroundColor Green
Write-Host "Location: $ZipPath" -ForegroundColor Green
Write-Host "Size: $([math]::Round((Get-Item $ZipPath).Length / 1MB, 2)) MB" -ForegroundColor Green

# 显示包内容摘要
Write-Host "`nPackage Contents:" -ForegroundColor Cyan
$AppFiles = Get-ChildItem $AppDir -Recurse
Write-Host "  Total files: $($AppFiles.Count)" -ForegroundColor Gray
Write-Host "  Executables: $($AppFiles | Where-Object { $_.Extension -eq '.exe' } | Measure-Object | Select-Object -ExpandProperty Count)" -ForegroundColor Gray
Write-Host "  Libraries: $($AppFiles | Where-Object { $_.Extension -eq '.dll' } | Measure-Object | Select-Object -ExpandProperty Count)" -ForegroundColor Gray

Write-Host "`nPackaging completed successfully!" -ForegroundColor Green
