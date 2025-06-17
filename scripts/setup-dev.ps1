# FakeInfo Hardware Spoofer Development Environment Setup Script
param(
    [string]$VcpkgRoot = "d:/vcpkg",
    [switch]$InstallVcpkg = $false,
    [switch]$InstallDependencies = $false,
    [switch]$ConfigureVSCode = $false
)

Write-Host "========================================" -ForegroundColor Green
Write-Host "FakeInfo Hardware Spoofer Dev Setup" -ForegroundColor Green
Write-Host "========================================" -ForegroundColor Green

# 设置错误处理
$ErrorActionPreference = "Stop"

# 检查管理员权限
$isAdmin = ([Security.Principal.WindowsPrincipal] [Security.Principal.WindowsIdentity]::GetCurrent()).IsInRole([Security.Principal.WindowsBuiltInRole] "Administrator")
if (-not $isAdmin) {
    Write-Warning "This script should be run as administrator for best results."
    Write-Host "Some operations may fail without administrator privileges." -ForegroundColor Yellow
}

# 获取项目根目录
$ProjectRoot = Split-Path -Parent (Split-Path -Parent $MyInvocation.MyCommand.Path)
Write-Host "Project Root: $ProjectRoot" -ForegroundColor Cyan

# 1. 安装vcpkg (如果需要)
if ($InstallVcpkg) {
    Write-Host "`n1. Installing vcpkg..." -ForegroundColor Yellow
    
    if (Test-Path $VcpkgRoot) {
        Write-Host "vcpkg already exists at: $VcpkgRoot" -ForegroundColor Green
    } else {
        Write-Host "Cloning vcpkg to: $VcpkgRoot"
        git clone https://github.com/Microsoft/vcpkg.git $VcpkgRoot
        
        if ($LASTEXITCODE -ne 0) {
            Write-Error "Failed to clone vcpkg"
        }
        
        Write-Host "Bootstrapping vcpkg..."
        & "$VcpkgRoot\bootstrap-vcpkg.bat"
        
        if ($LASTEXITCODE -ne 0) {
            Write-Error "Failed to bootstrap vcpkg"
        }
    }
    
    # 集成vcpkg到Visual Studio
    Write-Host "Integrating vcpkg with Visual Studio..."
    & "$VcpkgRoot\vcpkg.exe" integrate install
} else {
    Write-Host "`n1. Checking vcpkg installation..." -ForegroundColor Yellow
    
    if (-not (Test-Path $VcpkgRoot)) {
        Write-Error "vcpkg not found at: $VcpkgRoot"
        Write-Host "Please install vcpkg or use -InstallVcpkg flag"
        exit 1
    }
    
    if (-not (Test-Path "$VcpkgRoot\vcpkg.exe")) {
        Write-Error "vcpkg.exe not found. Please bootstrap vcpkg first."
        exit 1
    }
    
    Write-Host "vcpkg found at: $VcpkgRoot" -ForegroundColor Green
}

# 2. 安装依赖包
if ($InstallDependencies) {
    Write-Host "`n2. Installing dependencies..." -ForegroundColor Yellow
    
    $packages = @(
        "qt5-base:x64-windows",
        "qt5-tools:x64-windows", 
        "detours:x64-windows"
    )
    
    foreach ($package in $packages) {
        Write-Host "Installing $package..."
        & "$VcpkgRoot\vcpkg.exe" install $package
        
        if ($LASTEXITCODE -ne 0) {
            Write-Warning "Failed to install $package"
        } else {
            Write-Host "✓ $package installed successfully" -ForegroundColor Green
        }
    }
} else {
    Write-Host "`n2. Checking dependencies..." -ForegroundColor Yellow
    
    $packages = @("qt5-base", "qt5-tools", "detours")
    $allInstalled = $true
    
    foreach ($package in $packages) {
        $installed = & "$VcpkgRoot\vcpkg.exe" list | Select-String "$package.*x64-windows"
        if ($installed) {
            Write-Host "✓ $package is installed" -ForegroundColor Green
        } else {
            Write-Host "✗ $package is NOT installed" -ForegroundColor Red
            $allInstalled = $false
        }
    }
    
    if (-not $allInstalled) {
        Write-Host "Run with -InstallDependencies to install missing packages" -ForegroundColor Yellow
    }
}

# 3. 设置环境变量
Write-Host "`n3. Setting up environment variables..." -ForegroundColor Yellow

# 设置VCPKG_ROOT环境变量
$currentVcpkgRoot = [Environment]::GetEnvironmentVariable("VCPKG_ROOT", "User")
if ($currentVcpkgRoot -ne $VcpkgRoot) {
    Write-Host "Setting VCPKG_ROOT environment variable..."
    [Environment]::SetEnvironmentVariable("VCPKG_ROOT", $VcpkgRoot, "User")
    $env:VCPKG_ROOT = $VcpkgRoot
    Write-Host "✓ VCPKG_ROOT set to: $VcpkgRoot" -ForegroundColor Green
} else {
    Write-Host "✓ VCPKG_ROOT already set correctly" -ForegroundColor Green
}

# 4. 配置VSCode (如果需要)
if ($ConfigureVSCode) {
    Write-Host "`n4. Configuring VSCode..." -ForegroundColor Yellow
    
    # 创建.env文件
    $envFile = Join-Path $ProjectRoot ".env"
    if (-not (Test-Path $envFile)) {
        $envContent = @"
VCPKG_ROOT=$VcpkgRoot
CMAKE_BUILD_TYPE=Release
VCPKG_TARGET_TRIPLET=x64-windows
"@
        $envContent | Out-File -FilePath $envFile -Encoding UTF8
        Write-Host "✓ Created .env file" -ForegroundColor Green
    } else {
        Write-Host "✓ .env file already exists" -ForegroundColor Green
    }
    
    # 检查VSCode扩展
    $requiredExtensions = @(
        "ms-vscode.cpptools",
        "ms-vscode.cmake-tools",
        "twxs.cmake"
    )
    
    foreach ($ext in $requiredExtensions) {
        $installed = code --list-extensions | Select-String $ext
        if ($installed) {
            Write-Host "✓ VSCode extension $ext is installed" -ForegroundColor Green
        } else {
            Write-Host "Installing VSCode extension: $ext"
            code --install-extension $ext
        }
    }
}

# 5. 验证CMake配置
Write-Host "`n5. Verifying CMake configuration..." -ForegroundColor Yellow

$buildDir = Join-Path $ProjectRoot "build"
if (Test-Path $buildDir) {
    Write-Host "Removing existing build directory..."
    Remove-Item $buildDir -Recurse -Force
}

Write-Host "Testing CMake configuration..."
try {
    cmake -B $buildDir -S $ProjectRoot `
        -DCMAKE_TOOLCHAIN_FILE="$VcpkgRoot/scripts/buildsystems/vcpkg.cmake" `
        -DVCPKG_TARGET_TRIPLET=x64-windows `
        -G "Visual Studio 17 2022" -A x64
    
    if ($LASTEXITCODE -eq 0) {
        Write-Host "✓ CMake configuration successful" -ForegroundColor Green
    } else {
        Write-Warning "CMake configuration failed"
    }
} catch {
    Write-Warning "CMake configuration test failed: $($_.Exception.Message)"
}

# 6. 显示下一步操作
Write-Host "`n========================================" -ForegroundColor Green
Write-Host "Setup completed!" -ForegroundColor Green
Write-Host "========================================" -ForegroundColor Green

Write-Host "`nNext steps:" -ForegroundColor Cyan
Write-Host "1. Open the project in VSCode: code ." -ForegroundColor White
Write-Host "2. Press Ctrl+Shift+P and run 'CMake: Configure'" -ForegroundColor White
Write-Host "3. Press F5 to build and debug" -ForegroundColor White
Write-Host "4. Or use the build script: .\build.bat" -ForegroundColor White

Write-Host "`nEnvironment:" -ForegroundColor Cyan
Write-Host "  VCPKG_ROOT: $VcpkgRoot" -ForegroundColor White
Write-Host "  Project: $ProjectRoot" -ForegroundColor White

if ($isAdmin) {
    Write-Host "`nNote: You can now run the application with administrator privileges for full functionality." -ForegroundColor Yellow
} else {
    Write-Host "`nNote: Remember to run the final application as administrator for hook functionality." -ForegroundColor Yellow
}
