#include <windows.h>
#include <iostream>
#include <string>
#include <vector>

struct RegistryTest {
    HKEY rootKey;
    std::wstring subKey;
    std::wstring valueName;
    std::wstring description;
};

void QueryRegistryValue(HKEY hRootKey, const std::wstring& subKey, const std::wstring& valueName, const std::wstring& description)
{
    std::wcout << L"\n=== " << description << L" ===" << std::endl;
    std::wcout << L"Key: " << subKey << std::endl;
    std::wcout << L"Value: " << valueName << std::endl;
    
    HKEY hKey;
    LONG result = RegOpenKeyExW(hRootKey, subKey.c_str(), 0, KEY_READ, &hKey);
    
    if (result != ERROR_SUCCESS) {
        std::wcout << L"Failed to open registry key. Error: " << result << std::endl;
        return;
    }
    
    DWORD dataType;
    DWORD dataSize = 0;
    
    // 首先查询数据大小
    result = RegQueryValueExW(hKey, valueName.c_str(), nullptr, &dataType, nullptr, &dataSize);
    
    if (result != ERROR_SUCCESS && result != ERROR_MORE_DATA) {
        std::wcout << L"Failed to query value size. Error: " << result << std::endl;
        RegCloseKey(hKey);
        return;
    }
    
    if (dataSize == 0) {
        std::wcout << L"Value is empty or does not exist." << std::endl;
        RegCloseKey(hKey);
        return;
    }
    
    // 分配缓冲区并读取数据
    std::vector<BYTE> buffer(dataSize);
    result = RegQueryValueExW(hKey, valueName.c_str(), nullptr, &dataType, buffer.data(), &dataSize);
    
    if (result == ERROR_SUCCESS) {
        std::wcout << L"Type: ";
        switch (dataType) {
            case REG_SZ:
                std::wcout << L"REG_SZ";
                break;
            case REG_DWORD:
                std::wcout << L"REG_DWORD";
                break;
            case REG_BINARY:
                std::wcout << L"REG_BINARY";
                break;
            case REG_MULTI_SZ:
                std::wcout << L"REG_MULTI_SZ";
                break;
            default:
                std::wcout << L"Unknown (" << dataType << L")";
                break;
        }
        std::wcout << std::endl;
        
        std::wcout << L"Data: ";
        if (dataType == REG_SZ || dataType == REG_EXPAND_SZ) {
            std::wcout << reinterpret_cast<const wchar_t*>(buffer.data()) << std::endl;
        } else if (dataType == REG_DWORD) {
            DWORD value = *reinterpret_cast<const DWORD*>(buffer.data());
            std::wcout << value << L" (0x" << std::hex << value << std::dec << L")" << std::endl;
        } else {
            std::wcout << L"[Binary data, " << dataSize << L" bytes]" << std::endl;
        }
    } else {
        std::wcout << L"Failed to read value. Error: " << result << std::endl;
    }
    
    RegCloseKey(hKey);
}

void EnumerateRegistryValues(HKEY hRootKey, const std::wstring& subKey, const std::wstring& description)
{
    std::wcout << L"\n=== Enumerating " << description << L" ===" << std::endl;
    std::wcout << L"Key: " << subKey << std::endl;
    
    HKEY hKey;
    LONG result = RegOpenKeyExW(hRootKey, subKey.c_str(), 0, KEY_READ, &hKey);
    
    if (result != ERROR_SUCCESS) {
        std::wcout << L"Failed to open registry key. Error: " << result << std::endl;
        return;
    }
    
    DWORD index = 0;
    wchar_t valueName[256];
    DWORD valueNameSize;
    DWORD dataType;
    BYTE data[1024];
    DWORD dataSize;
    
    while (true) {
        valueNameSize = sizeof(valueName) / sizeof(wchar_t);
        dataSize = sizeof(data);
        
        result = RegEnumValueW(hKey, index, valueName, &valueNameSize, nullptr, &dataType, data, &dataSize);
        
        if (result == ERROR_NO_MORE_ITEMS) {
            break;
        }
        
        if (result == ERROR_SUCCESS) {
            std::wcout << L"  [" << index << L"] " << valueName << L" = ";
            
            if (dataType == REG_SZ || dataType == REG_EXPAND_SZ) {
                std::wcout << reinterpret_cast<const wchar_t*>(data);
            } else if (dataType == REG_DWORD) {
                DWORD value = *reinterpret_cast<const DWORD*>(data);
                std::wcout << value;
            } else {
                std::wcout << L"[" << dataSize << L" bytes]";
            }
            std::wcout << std::endl;
        }
        
        index++;
    }
    
    std::wcout << L"Total values enumerated: " << index << std::endl;
    RegCloseKey(hKey);
}

int main()
{
    std::wcout << L"Registry Hardware Information Test" << std::endl;
    std::wcout << L"==================================" << std::endl;
    
    // 定义要测试的注册表项
    std::vector<RegistryTest> tests = {
        // BIOS信息
        {HKEY_LOCAL_MACHINE, L"HARDWARE\\DESCRIPTION\\System\\BIOS", L"BIOSVendor", L"BIOS Vendor"},
        {HKEY_LOCAL_MACHINE, L"HARDWARE\\DESCRIPTION\\System\\BIOS", L"BIOSVersion", L"BIOS Version"},
        {HKEY_LOCAL_MACHINE, L"HARDWARE\\DESCRIPTION\\System\\BIOS", L"BIOSReleaseDate", L"BIOS Release Date"},
        {HKEY_LOCAL_MACHINE, L"HARDWARE\\DESCRIPTION\\System\\BIOS", L"SystemManufacturer", L"System Manufacturer"},
        {HKEY_LOCAL_MACHINE, L"HARDWARE\\DESCRIPTION\\System\\BIOS", L"SystemProductName", L"System Product Name"},
        {HKEY_LOCAL_MACHINE, L"HARDWARE\\DESCRIPTION\\System\\BIOS", L"SystemSerialNumber", L"System Serial Number"},
        {HKEY_LOCAL_MACHINE, L"HARDWARE\\DESCRIPTION\\System\\BIOS", L"SystemUUID", L"System UUID"},
        
        // CPU信息
        {HKEY_LOCAL_MACHINE, L"HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0", L"ProcessorNameString", L"CPU Name"},
        {HKEY_LOCAL_MACHINE, L"HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0", L"VendorIdentifier", L"CPU Vendor"},
        {HKEY_LOCAL_MACHINE, L"HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0", L"Identifier", L"CPU Identifier"},
        
        // 其他硬件信息
        {HKEY_LOCAL_MACHINE, L"SYSTEM\\CurrentControlSet\\Control\\SystemInformation", L"ComputerHardwareId", L"Computer Hardware ID"},
        {HKEY_LOCAL_MACHINE, L"SYSTEM\\CurrentControlSet\\Control\\SystemInformation", L"ComputerHardwareIds", L"Computer Hardware IDs"},
    };
    
    // 执行单个值查询测试
    for (const auto& test : tests) {
        QueryRegistryValue(test.rootKey, test.subKey, test.valueName, test.description);
    }
    
    // 执行枚举测试
    std::wcout << L"\n" << std::wstring(50, L'=') << std::endl;
    std::wcout << L"ENUMERATION TESTS" << std::endl;
    std::wcout << std::wstring(50, L'=') << std::endl;
    
    EnumerateRegistryValues(HKEY_LOCAL_MACHINE, L"HARDWARE\\DESCRIPTION\\System\\BIOS", L"BIOS Information");
    EnumerateRegistryValues(HKEY_LOCAL_MACHINE, L"HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0", L"CPU Information");
    
    // 测试网卡MAC地址
    std::wcout << L"\n=== Network Adapter MAC Address ===" << std::endl;
    QueryRegistryValue(HKEY_LOCAL_MACHINE, 
                      L"SYSTEM\\CurrentControlSet\\Control\\Class\\{4d36e972-e325-11ce-bfc1-08002be10318}\\0001", 
                      L"NetworkAddress", 
                      L"Network MAC Address");
    
    // 测试硬盘信息
    std::wcout << L"\n=== Disk Information ===" << std::endl;
    QueryRegistryValue(HKEY_LOCAL_MACHINE,
                      L"HARDWARE\\DESCRIPTION\\System\\MultifunctionAdapter\\0\\DiskController\\0\\DiskPeripheral\\0",
                      L"Identifier",
                      L"Disk Identifier");
    
    std::wcout << L"\nPress Enter to exit..." << std::endl;
    std::wcin.get();
    
    return 0;
}
