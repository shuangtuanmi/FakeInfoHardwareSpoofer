@echo off
REM Build script for FakeInfo Hardware Spoofer on Windows
REM Requires vcpkg and Qt5 to be installed

setlocal enabledelayedexpansion

echo ========================================
echo FakeInfo Hardware Spoofer Build Script
echo ========================================

REM Check if vcpkg is available
if not defined VCPKG_ROOT (
    echo Error: VCPKG_ROOT environment variable not set
    echo Please set VCPKG_ROOT to your vcpkg installation directory
    echo Example: set VCPKG_ROOT=D:\vcpkg
    pause
    exit /b 1
)

if not exist "%VCPKG_ROOT%\scripts\buildsystems\vcpkg.cmake" (
    echo Error: vcpkg not found at %VCPKG_ROOT%
    echo Please ensure vcpkg is properly installed
    pause
    exit /b 1
)

REM Set build configuration
set BUILD_TYPE=Release
set BUILD_DIR=build
set ENABLE_TESTS=ON
set ENABLE_DEPLOY=OFF

REM Parse command line arguments
:parse_args
if "%1"=="debug" (
    set BUILD_TYPE=Debug
    shift
    goto parse_args
)
if "%1"=="release" (
    set BUILD_TYPE=Release
    shift
    goto parse_args
)
if "%1"=="deploy" (
    set ENABLE_DEPLOY=ON
    shift
    goto parse_args
)
if "%1"=="no-tests" (
    set ENABLE_TESTS=OFF
    shift
    goto parse_args
)
if "%1"=="clean" (
    echo Cleaning build directory...
    if exist %BUILD_DIR% rmdir /s /q %BUILD_DIR%
    shift
    goto parse_args
)
if "%1"=="help" (
    echo Usage: build.bat [options]
    echo Options:
    echo   debug      - Build in Debug mode (default: Release)
    echo   release    - Build in Release mode
    echo   deploy     - Enable automatic Qt deployment
    echo   no-tests   - Disable building test programs
    echo   clean      - Clean build directory before building
    echo   help       - Show this help message
    pause
    exit /b 0
)
if not "%1"=="" (
    shift
    goto parse_args
)

echo Build Configuration:
echo   Build Type: %BUILD_TYPE%
echo   Build Tests: %ENABLE_TESTS%
echo   Auto Deploy: %ENABLE_DEPLOY%
echo   vcpkg Root: %VCPKG_ROOT%
echo.

REM Create build directory
if not exist %BUILD_DIR% mkdir %BUILD_DIR%

REM Configure with CMake
echo Configuring with CMake...
cd %BUILD_DIR%
cmake .. ^
    -DCMAKE_TOOLCHAIN_FILE="%VCPKG_ROOT%\scripts\buildsystems\vcpkg.cmake" ^
    -DCMAKE_BUILD_TYPE=%BUILD_TYPE% ^
    -DBUILD_TESTS=%ENABLE_TESTS% ^
    -DENABLE_AUTO_DEPLOY=%ENABLE_DEPLOY%

if errorlevel 1 (
    echo Error: CMake configuration failed
    cd ..
    pause
    exit /b 1
)

REM Build the project
echo.
echo Building project...
cmake --build . --config %BUILD_TYPE% --parallel

if errorlevel 1 (
    echo Error: Build failed
    cd ..
    pause
    exit /b 1
)

cd ..

echo.
echo ========================================
echo Build completed successfully!
echo ========================================
echo.
echo Executables are located in: %BUILD_DIR%\bin\
echo   - FakeInfoHardwareSpoofer.exe (Main application)
echo   - HookDLL.dll (Hook library)

if "%ENABLE_TESTS%"=="ON" (
    echo   - TestRegistry.exe (Registry test)
    echo   - TestSystemInfo.exe (System info test)
    echo   - TestWMI.exe (WMI test)
)

echo.
echo To deploy Qt libraries manually, run:
echo   windeployqt.exe --no-angle %BUILD_DIR%\bin\FakeInfoHardwareSpoofer.exe
echo.

if "%ENABLE_DEPLOY%"=="OFF" (
    echo Note: Automatic Qt deployment is disabled.
    echo Run the command above to deploy Qt libraries for distribution.
    echo.
)

echo Build script completed.
pause
