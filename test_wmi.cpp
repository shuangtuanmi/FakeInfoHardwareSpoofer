#include <windows.h>
#include <comdef.h>
#include <Wbemidl.h>
#include <iostream>
#include <string>

#pragma comment(lib, "wbemuuid.lib")
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "oleaut32.lib")

void QueryWMI(const std::wstring& query, const std::wstring& className)
{
    std::wcout << L"\n=== Querying: " << className << L" ===" << std::endl;
    std::wcout << L"Query: " << query << std::endl;
    
    HRESULT hres = CoInitializeEx(0, COINIT_MULTITHREADED);
    if (FAILED(hres)) {
        std::wcout << L"Failed to initialize COM library. Error code = 0x" 
                   << std::hex << hres << std::endl;
        return;
    }

    hres = CoInitializeSecurity(
        NULL, 
        -1,                          // COM authentication
        NULL,                        // Authentication services
        NULL,                        // Reserved
        RPC_C_AUTHN_LEVEL_DEFAULT,   // Default authentication 
        RPC_C_IMP_LEVEL_IMPERSONATE, // Default Impersonation  
        NULL,                        // Authentication info
        EOAC_NONE,                   // Additional capabilities 
        NULL                         // Reserved
    );

    if (FAILED(hres)) {
        std::wcout << L"Failed to initialize security. Error code = 0x" 
                   << std::hex << hres << std::endl;
        CoUninitialize();
        return;
    }

    IWbemLocator *pLoc = NULL;
    hres = CoCreateInstance(
        CLSID_WbemLocator,             
        0, 
        CLSCTX_INPROC_SERVER, 
        IID_IWbemLocator, (LPVOID *) &pLoc);

    if (FAILED(hres)) {
        std::wcout << L"Failed to create IWbemLocator object. Error code = 0x" 
                   << std::hex << hres << std::endl;
        CoUninitialize();
        return;
    }

    IWbemServices *pSvc = NULL;
    hres = pLoc->ConnectServer(
        _bstr_t(L"ROOT\\CIMV2"), // Object path of WMI namespace
        NULL,                    // User name. NULL = current user
        NULL,                    // User password. NULL = current
        0,                       // Locale. NULL indicates current
        NULL,                    // Security flags.
        0,                       // Authority (for example, Kerberos)
        0,                       // Context object 
        &pSvc                    // pointer to IWbemServices proxy
    );

    if (FAILED(hres)) {
        std::wcout << L"Could not connect. Error code = 0x" 
                   << std::hex << hres << std::endl;
        pLoc->Release();
        CoUninitialize();
        return;
    }

    std::wcout << L"Connected to ROOT\\CIMV2 WMI namespace" << std::endl;

    hres = CoSetProxyBlanket(
        pSvc,                        // Indicates the proxy to set
        RPC_C_AUTHN_WINNT,           // RPC_C_AUTHN_xxx
        RPC_C_AUTHZ_NONE,            // RPC_C_AUTHZ_xxx
        NULL,                        // Server principal name 
        RPC_C_AUTHN_LEVEL_CALL,      // RPC_C_AUTHN_LEVEL_xxx 
        RPC_C_IMP_LEVEL_IMPERSONATE, // RPC_C_IMP_LEVEL_xxx
        NULL,                        // client identity
        EOAC_NONE                    // proxy capabilities 
    );

    if (FAILED(hres)) {
        std::wcout << L"Could not set proxy blanket. Error code = 0x" 
                   << std::hex << hres << std::endl;
        pSvc->Release();
        pLoc->Release();
        CoUninitialize();
        return;
    }

    IEnumWbemClassObject* pEnumerator = NULL;
    hres = pSvc->ExecQuery(
        bstr_t("WQL"), 
        bstr_t(query.c_str()),
        WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY, 
        NULL,
        &pEnumerator);

    if (FAILED(hres)) {
        std::wcout << L"Query failed. Error code = 0x" 
                   << std::hex << hres << std::endl;
        pSvc->Release();
        pLoc->Release();
        CoUninitialize();
        return;
    }

    IWbemClassObject *pclsObj = NULL;
    ULONG uReturn = 0;
    int count = 0;

    while (pEnumerator) {
        HRESULT hr = pEnumerator->Next(WBEM_INFINITE, 1, &pclsObj, &uReturn);

        if (0 == uReturn) {
            break;
        }

        count++;
        std::wcout << L"\n--- Object " << count << L" ---" << std::endl;

        VARIANT vtProp;

        // 根据不同的类查询不同的属性
        if (className == L"Win32_Processor") {
            hr = pclsObj->Get(L"Name", 0, &vtProp, 0, 0);
            if (SUCCEEDED(hr) && vtProp.vt != VT_NULL) {
                std::wcout << L"Name: " << vtProp.bstrVal << std::endl;
            }
            VariantClear(&vtProp);

            hr = pclsObj->Get(L"Manufacturer", 0, &vtProp, 0, 0);
            if (SUCCEEDED(hr) && vtProp.vt != VT_NULL) {
                std::wcout << L"Manufacturer: " << vtProp.bstrVal << std::endl;
            }
            VariantClear(&vtProp);

            hr = pclsObj->Get(L"ProcessorId", 0, &vtProp, 0, 0);
            if (SUCCEEDED(hr) && vtProp.vt != VT_NULL) {
                std::wcout << L"ProcessorId: " << vtProp.bstrVal << std::endl;
            }
            VariantClear(&vtProp);
        }
        else if (className == L"Win32_BIOS") {
            hr = pclsObj->Get(L"Manufacturer", 0, &vtProp, 0, 0);
            if (SUCCEEDED(hr) && vtProp.vt != VT_NULL) {
                std::wcout << L"Manufacturer: " << vtProp.bstrVal << std::endl;
            }
            VariantClear(&vtProp);

            hr = pclsObj->Get(L"SMBIOSBIOSVersion", 0, &vtProp, 0, 0);
            if (SUCCEEDED(hr) && vtProp.vt != VT_NULL) {
                std::wcout << L"Version: " << vtProp.bstrVal << std::endl;
            }
            VariantClear(&vtProp);

            hr = pclsObj->Get(L"SerialNumber", 0, &vtProp, 0, 0);
            if (SUCCEEDED(hr) && vtProp.vt != VT_NULL) {
                std::wcout << L"SerialNumber: " << vtProp.bstrVal << std::endl;
            }
            VariantClear(&vtProp);
        }
        else if (className == L"Win32_BaseBoard") {
            hr = pclsObj->Get(L"Manufacturer", 0, &vtProp, 0, 0);
            if (SUCCEEDED(hr) && vtProp.vt != VT_NULL) {
                std::wcout << L"Manufacturer: " << vtProp.bstrVal << std::endl;
            }
            VariantClear(&vtProp);

            hr = pclsObj->Get(L"Product", 0, &vtProp, 0, 0);
            if (SUCCEEDED(hr) && vtProp.vt != VT_NULL) {
                std::wcout << L"Product: " << vtProp.bstrVal << std::endl;
            }
            VariantClear(&vtProp);

            hr = pclsObj->Get(L"SerialNumber", 0, &vtProp, 0, 0);
            if (SUCCEEDED(hr) && vtProp.vt != VT_NULL) {
                std::wcout << L"SerialNumber: " << vtProp.bstrVal << std::endl;
            }
            VariantClear(&vtProp);
        }

        pclsObj->Release();
    }

    std::wcout << L"Total objects found: " << count << std::endl;

    pSvc->Release();
    pLoc->Release();
    pEnumerator->Release();
    CoUninitialize();
}

int main()
{
    std::wcout << L"WMI Hardware Information Test" << std::endl;
    std::wcout << L"=============================" << std::endl;
    
    // 测试CPU信息
    QueryWMI(L"SELECT * FROM Win32_Processor", L"Win32_Processor");
    
    // 测试BIOS信息
    QueryWMI(L"SELECT * FROM Win32_BIOS", L"Win32_BIOS");
    
    // 测试主板信息
    QueryWMI(L"SELECT * FROM Win32_BaseBoard", L"Win32_BaseBoard");
    
    std::wcout << L"\nPress Enter to exit..." << std::endl;
    std::cin.get();
    
    return 0;
}
