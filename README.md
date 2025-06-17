# 🔧 FakeInfo Hardware Spoofer

[![Build Status](https://github.com/shuangtuanmi/FakeInfoHardwareSpoofer/workflows/CI/badge.svg)](https://github.com/shuangtuanmi/FakeInfoHardwareSpoofer/actions)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![Platform](https://img.shields.io/badge/platform-Windows%20%7C%20Linux%20%7C%20macOS-lightgrey)](https://github.com/shuangtuanmi/FakeInfoHardwareSpoofer)
[![Qt Version](https://img.shields.io/badge/Qt-5.15+-green.svg)](https://www.qt.io/)

A cross-platform Qt5-based hardware information spoofing tool that can intercept and modify system hardware information queries. This tool is designed for privacy protection, testing, and research purposes.

## ✨ Features

### 🎯 Core Functionality

- **WMI Query Interception** (Windows): Hooks Windows Management Instrumentation queries
- **Registry Hook** (Windows): Intercepts Windows registry queries for hardware information
- **Hardware Randomization**: Generates realistic fake hardware information
- **Real-time Configuration**: Modify hardware information on-the-fly
- **Process Injection** (Windows): Injects hooks into target processes
- **Auto-Save Configuration**: Automatically saves and restores hardware settings

### 🖥️ User Interface

- **Intuitive GUI**: Easy-to-use tabbed interface
- **Hardware Categories**: Organized by BIOS, CPU, Memory, Disk, GPU, Network, Audio
- **One-Click Randomization**: Generate complete fake hardware profiles instantly
- **Configuration Management**: Save/load hardware profiles as JSON files
- **Real-time System Info**: View current system information

### 🔧 Advanced Features

- **Suspended Process Creation**: Creates target processes in suspended state for clean injection
- **Comprehensive Hook Coverage**: Covers WMI, Registry, and other system APIs
- **Automatic Settings Persistence**: Maintains configuration across system restarts
- **Test Programs**: Includes utilities to verify hook functionality

## 🚀 Quick Start

### Windows

1. Download the latest release from [Releases](https://github.com/shuangtuanmi/FakeInfoHardwareSpoofer/releases)
2. Extract the archive
3. Run `FakeInfoHardwareSpoofer.exe` as Administrator
4. Configure your target program (VSCode is set as default)
5. Click "一键随机化所有硬件" to generate fake hardware info
6. Click "开始 Hook" to start spoofing

### Linux/macOS

```bash
# Install Qt5 development packages
# Ubuntu/Debian:
sudo apt-get install qt5-default qtbase5-dev

# macOS:
brew install qt5

# Build from source
git clone https://github.com/shuangtuanmi/FakeInfoHardwareSpoofer.git
cd FakeInfoHardwareSpoofer
mkdir build && cd build
cmake ..
make -j$(nproc)
```

## 📋 System Requirements

### Minimum Requirements

- **OS**: Windows 10, Ubuntu 18.04+, macOS 10.14+
- **RAM**: 512 MB
- **Storage**: 100 MB free space
- **Qt**: 5.15 or later

### Windows Specific

- **Detours Library**: For API hooking functionality
- **Administrator Rights**: Required for process injection

### Development Requirements

- **Compiler**: MSVC 2019+, GCC 8+, or Clang 10+
- **CMake**: 3.16 or later
- **vcpkg**: For dependency management (Windows)

## 🛠️ Building from Source

### Windows with vcpkg

1. **Install vcpkg**:

```bash
git clone https://github.com/Microsoft/vcpkg.git
cd vcpkg
.\bootstrap-vcpkg.bat
.\vcpkg integrate install
```

2. **Install dependencies**:

```bash
.\vcpkg install qt5-base qt5-widgets qt5-network detours --triplet x64-windows
```

3. **Build project**:

```bash
mkdir build
cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE=path/to/vcpkg/scripts/buildsystems/vcpkg.cmake
cmake --build . --config Release
```

4. **Deploy Qt libraries**:

```bash
# Manual deployment
windeployqt.exe --no-angle build/bin/FakeInfoHardwareSpoofer.exe
```

### Linux

```bash
# Install dependencies
sudo apt-get update
sudo apt-get install build-essential cmake qt5-default qtbase5-dev

# Build
mkdir build && cd build
cmake ..
make -j$(nproc)

# Install (optional)
sudo make install
```

### macOS

```bash
# Install dependencies
brew install qt5 cmake

# Build
mkdir build && cd build
cmake .. -DCMAKE_PREFIX_PATH=$(brew --prefix qt5)
make -j$(sysctl -n hw.ncpu)
```

## 📖 Usage Guide

### Basic Operations

1. **Launch Application**

   - Windows: Run as Administrator
   - Linux/macOS: Run from terminal or application launcher

2. **View System Information**

   - Navigate to "系统信息" (System Info) tab
   - Click "刷新系统信息" (Refresh) to view current hardware

3. **Configure Hardware**

   - Go to "硬件配置" (Hardware Config) tab
   - Manually edit fields or use randomization

4. **Randomization**

   - **Individual**: Click randomize buttons for specific components
   - **All**: Click "一键随机化所有硬件" (Randomize All Hardware)

5. **Configuration Management**

   - **Save**: Export current config to JSON file
   - **Load**: Import config from JSON file

6. **Enable Spoofing** (Windows only)
   - Go to "控制面板" (Control Panel) tab
   - Set target program path
   - Click "开始 Hook" (Start Hook)

### Configuration File Format

Hardware configurations are saved in JSON format:

```json
{
  "biosVendor": "American Megatrends Inc.",
  "biosVersion": "2.15.1236",
  "cpuManufacturer": "Intel Corporation",
  "cpuName": "Intel(R) Core(TM) i7-10700K CPU @ 3.80GHz",
  "motherboardManufacturer": "ASUSTeK COMPUTER INC.",
  "motherboardProduct": "ROG STRIX Z490-E GAMING",
  "systemUuid": "12345678-1234-1234-1234-123456789ABC",
  "memoryManufacturers": ["Samsung", "SK Hynix"],
  "diskModels": ["Samsung SSD 980 PRO 1TB"],
  "gpuNames": ["NVIDIA GeForce RTX 3080"],
  "nicMacAddresses": ["00:11:22:33:44:55"]
}
```

## 🏗️ Architecture

### Project Structure

```
FakeInfoHardwareSpoofer/
├── main.cpp                    # Application entry point
├── MainWindow.h/.cpp          # Main GUI window
├── SystemInfo.h/.cpp          # System information module
├── HardwareRandomizer.h/.cpp  # Hardware randomization
├── HookDLL.cpp                # Hook DLL implementation (Windows)
├── test_*.cpp                 # Test programs
├── CMakeLists.txt             # Build configuration
├── .github/workflows/         # CI/CD configuration
└── README.md                  # This file
```

### Core Technologies

- **Qt5**: Cross-platform GUI framework
- **Microsoft Detours**: API hooking (Windows)
- **WMI**: Windows Management Instrumentation
- **COM**: Component Object Model (Windows)
- **CMake**: Build system
- **GitHub Actions**: CI/CD pipeline

## 🔒 Security & Legal

### ⚠️ Important Disclaimers

- **Educational Purpose**: This tool is for educational and research purposes only
- **Legal Compliance**: Users must comply with applicable laws and regulations
- **No Warranty**: Use at your own risk
- **Backup Data**: Always backup important data before use

### Security Considerations

- Some antivirus software may flag this as potentially unwanted
- Administrator privileges required for full functionality on Windows
- System protection mechanisms may interfere with hooking

## 🐛 Troubleshooting

### Common Issues

1. **Application won't start**

   - Ensure Qt5 runtime libraries are installed
   - Run as Administrator (Windows)
   - Check system requirements

2. **Hook functionality not working**

   - Verify target process is running
   - Check HookDLL.dll exists in application directory
   - Review log files for errors

3. **Build failures**
   - Verify all dependencies are installed
   - Check CMake version compatibility
   - Ensure correct toolchain configuration

### Log Files

- `hook_log.txt`: Hook operation logs
- Application logs: Displayed in Control Panel tab

## 🤝 Contributing

We welcome contributions! Please see our [Contributing Guidelines](CONTRIBUTING.md) for details.

### Development Setup

1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Add tests if applicable
5. Submit a pull request

## 📄 License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## 🙏 Acknowledgments

- Qt Framework for the excellent GUI toolkit
- Microsoft Detours for API hooking capabilities
- Open source community for inspiration and support

## 📞 Support

- **Issues**: [GitHub Issues](https://github.com/shuangtuanmi/FakeInfoHardwareSpoofer/issues)
- **Discussions**: [GitHub Discussions](https://github.com/shuangtuanmi/FakeInfoHardwareSpoofer/discussions)
- **Wiki**: [Project Wiki](https://github.com/shuangtuanmi/FakeInfoHardwareSpoofer/wiki)

---

**⭐ If you find this project useful, please consider giving it a star!**
