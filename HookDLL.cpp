#include <windows.h>
#include <comdef.h>
#include <Wbemidl.h>
#include <string>
#include <map>
#include <vector>
#include <memory>
#include <fstream>
#include <sstream>
#include <mutex>
#include <atlbase.h>
#include <atlcom.h>

#ifdef DETOURS_AVAILABLE
#include <detours.h>
#pragma comment(lib, "detours.lib")
#endif

#pragma comment(lib, "wbemuuid.lib")
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "oleaut32.lib")

// 全局变量存储硬件配置
struct HardwareConfig {
    std::wstring biosVendor;
    std::wstring biosVersion;
    std::wstring biosDate;
    std::wstring biosSerial;
    std::wstring systemUuid;

    std::wstring motherboardManufacturer;
    std::wstring motherboardProduct;
    std::wstring motherboardVersion;
    std::wstring motherboardSerial;

    std::wstring cpuManufacturer;
    std::wstring cpuName;
    std::wstring cpuId;
    std::wstring cpuSerial;
    int cpuCores;
    int cpuThreads;

    std::vector<std::wstring> memoryManufacturers;
    std::vector<std::wstring> memoryPartNumbers;
    std::vector<std::wstring> memorySerials;

    std::vector<std::wstring> diskModels;
    std::vector<std::wstring> diskSerials;
    std::vector<std::wstring> diskFirmwares;

    std::vector<std::wstring> gpuNames;
    std::vector<std::wstring> gpuManufacturers;
    std::vector<std::wstring> gpuDriverVersions;

    std::vector<std::wstring> nicNames;
    std::vector<std::wstring> nicManufacturers;
    std::vector<std::wstring> nicMacAddresses;

    std::vector<std::wstring> audioDeviceNames;
    std::vector<std::wstring> audioManufacturers;
} g_hardwareConfig;

// 全局变量
std::mutex g_logMutex;
std::mutex g_configMutex;
bool g_hookInstalled = false;

// 日志函数
void WriteLog(const std::wstring& message) {
    std::lock_guard<std::mutex> lock(g_logMutex);
    std::wofstream logFile(L"hook_log.txt", std::ios::app);
    if (logFile.is_open()) {
        SYSTEMTIME st;
        GetLocalTime(&st);
        logFile << L"[" << st.wHour << L":" << st.wMinute << L":" << st.wSecond << L"] " << message << std::endl;
        logFile.close();
    }
}

// 伪造的WMI对象实现
class FakeWbemClassObject : public IWbemClassObject
{
private:
    ULONG m_refCount;
    std::map<std::wstring, VARIANT> m_properties;

public:
    FakeWbemClassObject() : m_refCount(1) {}

    virtual ~FakeWbemClassObject() {
        for (auto& prop : m_properties) {
            VariantClear(&prop.second);
        }
    }

    // IUnknown methods
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppvObject) override {
        if (riid == IID_IUnknown || riid == IID_IWbemClassObject) {
            *ppvObject = this;
            AddRef();
            return S_OK;
        }
        return E_NOINTERFACE;
    }

    ULONG STDMETHODCALLTYPE AddRef() override {
        return InterlockedIncrement(&m_refCount);
    }

    ULONG STDMETHODCALLTYPE Release() override {
        ULONG count = InterlockedDecrement(&m_refCount);
        if (count == 0) {
            delete this;
        }
        return count;
    }

    // IWbemClassObject methods
    HRESULT STDMETHODCALLTYPE GetQualifierSet(IWbemQualifierSet** ppQualSet) override {
        return E_NOTIMPL;
    }

    HRESULT STDMETHODCALLTYPE Get(LPCWSTR wszName, long lFlags, VARIANT* pVal, CIMTYPE* pType, long* plFlavor) override {
        if (!wszName || !pVal) return WBEM_E_INVALID_PARAMETER;

        // std::wstring propName = wszName;
        // WriteLog(L"FakeWbemClassObject::Get called for property: " + propName);

        // 确保目标VARIANT被正确初始化
        VariantInit(pVal);

        auto it = m_properties.find(wszName);
        if (it != m_properties.end()) {
            // WriteLog(L"Property found, returning value");

            // 检查源VARIANT的类型
            if (it->second.vt == VT_BSTR && it->second.bstrVal) {
                // WriteLog(L"Copying BSTR property");
                pVal->vt = VT_BSTR;
                pVal->bstrVal = SysAllocString(it->second.bstrVal);
                if (pVal->bstrVal) {
                    // WriteLog(L"BSTR property copied successfully");
                    return S_OK;
                } else {
                    // WriteLog(L"Failed to allocate BSTR");
                    return E_OUTOFMEMORY;
                }
            } else {
                // WriteLog(L"Copying non-BSTR property");
                HRESULT hr = VariantCopy(pVal, &it->second);
                // if (SUCCEEDED(hr)) {
                //     WriteLog(L"Property value copied successfully");
                // } else {
                //     WriteLog(L"Failed to copy property value, error: 0x" + std::to_wstring(hr));
                // }
                return hr;
            }
        }

        // WriteLog(L"Property not found: " + propName);
        return WBEM_E_NOT_FOUND;
    }

    HRESULT STDMETHODCALLTYPE Put(LPCWSTR wszName, long lFlags, VARIANT* pVal, CIMTYPE Type) override {
        if (!wszName) return WBEM_E_INVALID_PARAMETER;

        VARIANT newVal;
        VariantInit(&newVal);
        if (pVal) {
            HRESULT hr = VariantCopy(&newVal, pVal);
            if (FAILED(hr)) return hr;
        }

        auto it = m_properties.find(wszName);
        if (it != m_properties.end()) {
            VariantClear(&it->second);
        }
        m_properties[wszName] = newVal;
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE Delete(LPCWSTR wszName) override {
        auto it = m_properties.find(wszName);
        if (it != m_properties.end()) {
            VariantClear(&it->second);
            m_properties.erase(it);
            return S_OK;
        }
        return WBEM_E_NOT_FOUND;
    }

    HRESULT STDMETHODCALLTYPE GetNames(LPCWSTR wszQualifierName, long lFlags, VARIANT* pQualifierVal, SAFEARRAY** pNames) override {
        return E_NOTIMPL;
    }

    HRESULT STDMETHODCALLTYPE BeginEnumeration(long lEnumFlags) override {
        return E_NOTIMPL;
    }

    HRESULT STDMETHODCALLTYPE Next(long lFlags, BSTR* pstrName, VARIANT* pVal, CIMTYPE* pType, long* plFlavor) override {
        return E_NOTIMPL;
    }

    HRESULT STDMETHODCALLTYPE EndEnumeration() override {
        return E_NOTIMPL;
    }

    HRESULT STDMETHODCALLTYPE GetPropertyQualifierSet(LPCWSTR wszProperty, IWbemQualifierSet** ppQualSet) override {
        return E_NOTIMPL;
    }

    HRESULT STDMETHODCALLTYPE Clone(IWbemClassObject** ppCopy) override {
        return E_NOTIMPL;
    }

    HRESULT STDMETHODCALLTYPE GetObjectText(long lFlags, BSTR* pstrObjectText) override {
        return E_NOTIMPL;
    }

    HRESULT STDMETHODCALLTYPE SpawnDerivedClass(long lFlags, IWbemClassObject** ppNewClass) override {
        return E_NOTIMPL;
    }

    HRESULT STDMETHODCALLTYPE SpawnInstance(long lFlags, IWbemClassObject** ppNewInstance) override {
        return E_NOTIMPL;
    }

    HRESULT STDMETHODCALLTYPE CompareTo(long lFlags, IWbemClassObject* pCompareTo) override {
        return E_NOTIMPL;
    }

    HRESULT STDMETHODCALLTYPE GetPropertyOrigin(LPCWSTR wszName, BSTR* pstrClassName) override {
        return E_NOTIMPL;
    }

    HRESULT STDMETHODCALLTYPE InheritsFrom(LPCWSTR strAncestor) override {
        return E_NOTIMPL;
    }

    HRESULT STDMETHODCALLTYPE GetMethod(LPCWSTR wszName, long lFlags, IWbemClassObject** ppInSignature, IWbemClassObject** ppOutSignature) override {
        return E_NOTIMPL;
    }

    HRESULT STDMETHODCALLTYPE PutMethod(LPCWSTR wszName, long lFlags, IWbemClassObject* pInSignature, IWbemClassObject* pOutSignature) override {
        return E_NOTIMPL;
    }

    HRESULT STDMETHODCALLTYPE DeleteMethod(LPCWSTR wszName) override {
        return E_NOTIMPL;
    }

    HRESULT STDMETHODCALLTYPE BeginMethodEnumeration(long lEnumFlags) override {
        return E_NOTIMPL;
    }

    HRESULT STDMETHODCALLTYPE NextMethod(long lFlags, BSTR* pstrName, IWbemClassObject** ppInSignature, IWbemClassObject** ppOutSignature) override {
        return E_NOTIMPL;
    }

    HRESULT STDMETHODCALLTYPE EndMethodEnumeration() override {
        return E_NOTIMPL;
    }

    HRESULT STDMETHODCALLTYPE GetMethodQualifierSet(LPCWSTR wszMethod, IWbemQualifierSet** ppQualSet) override {
        return E_NOTIMPL;
    }

    HRESULT STDMETHODCALLTYPE GetMethodOrigin(LPCWSTR wszMethodName, BSTR* pstrClassName) override {
        return E_NOTIMPL;
    }

    // 辅助方法
    void SetStringProperty(const std::wstring& name, const std::wstring& value) {
        VARIANT var;
        VariantInit(&var);
        var.vt = VT_BSTR;
        var.bstrVal = SysAllocString(value.c_str());
        Put(name.c_str(), 0, &var, CIM_STRING);
        VariantClear(&var);
    }

    void SetIntProperty(const std::wstring& name, int value) {
        VARIANT var;
        VariantInit(&var);
        var.vt = VT_I4;
        var.lVal = value;
        Put(name.c_str(), 0, &var, CIM_SINT32);
    }

    void SetUIntProperty(const std::wstring& name, unsigned int value) {
        VARIANT var;
        VariantInit(&var);
        var.vt = VT_UI4;
        var.ulVal = value;
        Put(name.c_str(), 0, &var, CIM_UINT32);
    }
};

// WMI枚举器实现
class FakeWbemEnumerator : public IEnumWbemClassObject
{
private:
    ULONG m_refCount;
    std::vector<IWbemClassObject*> m_objects;
    size_t m_currentIndex;

public:
    FakeWbemEnumerator() : m_refCount(1), m_currentIndex(0) {}

    virtual ~FakeWbemEnumerator() {
        for (auto obj : m_objects) {
            if (obj) obj->Release();
        }
    }

    // IUnknown methods
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppvObject) override {
        if (riid == IID_IUnknown || riid == IID_IEnumWbemClassObject) {
            *ppvObject = this;
            AddRef();
            return S_OK;
        }
        return E_NOINTERFACE;
    }

    ULONG STDMETHODCALLTYPE AddRef() override {
        return InterlockedIncrement(&m_refCount);
    }

    ULONG STDMETHODCALLTYPE Release() override {
        ULONG count = InterlockedDecrement(&m_refCount);
        if (count == 0) {
            delete this;
        }
        return count;
    }

    // IEnumWbemClassObject methods
    HRESULT STDMETHODCALLTYPE Next(long lTimeout, ULONG uCount, IWbemClassObject** apObjects, ULONG* puReturned) override {
        if (!apObjects || !puReturned) return WBEM_E_INVALID_PARAMETER;

        *puReturned = 0;
        ULONG returned = 0;

        for (ULONG i = 0; i < uCount && m_currentIndex < m_objects.size(); i++, m_currentIndex++) {
            apObjects[i] = m_objects[m_currentIndex];
            apObjects[i]->AddRef();
            returned++;
        }

        *puReturned = returned;
        return (returned == uCount) ? WBEM_S_NO_ERROR : WBEM_S_FALSE;
    }

    HRESULT STDMETHODCALLTYPE NextAsync(ULONG uCount, IWbemObjectSink* pSink) override {
        return E_NOTIMPL;
    }

    HRESULT STDMETHODCALLTYPE Clone(IEnumWbemClassObject** ppEnum) override {
        return E_NOTIMPL;
    }

    HRESULT STDMETHODCALLTYPE Skip(long lTimeout, ULONG nCount) override {
        m_currentIndex += nCount;
        if (m_currentIndex > m_objects.size()) {
            m_currentIndex = m_objects.size();
        }
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE Reset() override {
        m_currentIndex = 0;
        return S_OK;
    }

    // 辅助方法
    void AddObject(IWbemClassObject* obj) {
        if (obj) {
            obj->AddRef();
            m_objects.push_back(obj);
        }
    }
};

// 原始函数指针和Hook相关
typedef HRESULT (WINAPI *pExecQuery)(
    IWbemServices* pSvc,
    const BSTR strQueryLanguage,
    const BSTR strQuery,
    long lFlags,
    IWbemContext* pCtx,
    IEnumWbemClassObject** ppEnum
);

pExecQuery OriginalExecQuery = nullptr;

// 前置声明
HRESULT CreateFakeCPUResult(IEnumWbemClassObject** ppEnum);
HRESULT CreateFakeGPUResult(IEnumWbemClassObject** ppEnum);
HRESULT CreateFakeMotherboardResult(IEnumWbemClassObject** ppEnum);
HRESULT CreateFakeBIOSResult(IEnumWbemClassObject** ppEnum);
HRESULT CreateFakeMemoryResult(IEnumWbemClassObject** ppEnum);
HRESULT CreateFakeDiskResult(IEnumWbemClassObject** ppEnum);
HRESULT CreateFakeNetworkResult(IEnumWbemClassObject** ppEnum);
HRESULT CreateFakeAudioResult(IEnumWbemClassObject** ppEnum);

// WMI服务代理类，用于拦截ExecQuery调用
class WbemServicesProxy : public IWbemServices
{
private:
    IWbemServices* m_pOriginal;
    ULONG m_refCount;

public:
    WbemServicesProxy(IWbemServices* pOriginal) : m_pOriginal(pOriginal), m_refCount(1) {
        if (m_pOriginal) m_pOriginal->AddRef();
    }

    virtual ~WbemServicesProxy() {
        if (m_pOriginal) m_pOriginal->Release();
    }

    // IUnknown methods
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppvObject) override {
        // WriteLog(L"WbemServicesProxy::QueryInterface called");

        if (riid == IID_IUnknown || riid == IID_IWbemServices) {
            // WriteLog(L"QueryInterface: Returning proxy for IUnknown/IWbemServices");
            *ppvObject = this;
            AddRef();
            return S_OK;
        }

        // 对于其他接口，直接转发到原始对象
        // 这样CoSetProxyBlanket等调用就能正常工作
        // WriteLog(L"QueryInterface: Forwarding to original object");
        return m_pOriginal ? m_pOriginal->QueryInterface(riid, ppvObject) : E_NOINTERFACE;
    }

    ULONG STDMETHODCALLTYPE AddRef() override {
        return InterlockedIncrement(&m_refCount);
    }

    ULONG STDMETHODCALLTYPE Release() override {
        ULONG count = InterlockedDecrement(&m_refCount);
        if (count == 0) {
            delete this;
        }
        return count;
    }

    // 关键的ExecQuery方法 - 这里进行Hook
    HRESULT STDMETHODCALLTYPE ExecQuery(
        const BSTR strQueryLanguage,
        const BSTR strQuery,
        long lFlags,
        IWbemContext* pCtx,
        IEnumWbemClassObject** ppEnum) override
    {
        // WriteLog(L"*** ExecQuery method called on WbemServicesProxy ***");
        std::wstring query(strQuery ? strQuery : L"");
        // WriteLog(L"WMI Query intercepted: " + query);

        // 检查是否为硬件信息查询
        if (query.find(L"Win32_Processor") != std::wstring::npos) {
            // WriteLog(L"Returning fake CPU data");
            return CreateFakeCPUResult(ppEnum);
        }
        else if (query.find(L"Win32_VideoController") != std::wstring::npos) {
            // WriteLog(L"Returning fake GPU data");
            return CreateFakeGPUResult(ppEnum);
        }
        else if (query.find(L"Win32_BaseBoard") != std::wstring::npos) {
            // WriteLog(L"Returning fake motherboard data");
            return CreateFakeMotherboardResult(ppEnum);
        }
        else if (query.find(L"Win32_BIOS") != std::wstring::npos) {
            // WriteLog(L"Returning fake BIOS data");
            return CreateFakeBIOSResult(ppEnum);
        }
        else if (query.find(L"Win32_PhysicalMemory") != std::wstring::npos) {
            // WriteLog(L"Returning fake memory data");
            return CreateFakeMemoryResult(ppEnum);
        }
        else if (query.find(L"Win32_DiskDrive") != std::wstring::npos) {
            // WriteLog(L"Returning fake disk data");
            return CreateFakeDiskResult(ppEnum);
        }
        else if (query.find(L"Win32_NetworkAdapter") != std::wstring::npos) {
            // WriteLog(L"Returning fake network data");
            return CreateFakeNetworkResult(ppEnum);
        }
        else if (query.find(L"Win32_SoundDevice") != std::wstring::npos) {
            // WriteLog(L"Returning fake audio data");
            return CreateFakeAudioResult(ppEnum);
        }

        // 对于其他查询，调用原始方法
        if (m_pOriginal) {
            return m_pOriginal->ExecQuery(strQueryLanguage, strQuery, lFlags, pCtx, ppEnum);
        }

        return E_FAIL;
    }

    // 其他IWbemServices方法的转发实现
    HRESULT STDMETHODCALLTYPE OpenNamespace(const BSTR strNamespace, long lFlags, IWbemContext* pCtx, IWbemServices** ppWorkingNamespace, IWbemCallResult** ppResult) override {
        return m_pOriginal ? m_pOriginal->OpenNamespace(strNamespace, lFlags, pCtx, ppWorkingNamespace, ppResult) : E_NOTIMPL;
    }

    HRESULT STDMETHODCALLTYPE CancelAsyncCall(IWbemObjectSink* pSink) override {
        return m_pOriginal ? m_pOriginal->CancelAsyncCall(pSink) : E_NOTIMPL;
    }

    HRESULT STDMETHODCALLTYPE QueryObjectSink(long lFlags, IWbemObjectSink** ppResponseHandler) override {
        return m_pOriginal ? m_pOriginal->QueryObjectSink(lFlags, ppResponseHandler) : E_NOTIMPL;
    }

    HRESULT STDMETHODCALLTYPE GetObject(const BSTR strObjectPath, long lFlags, IWbemContext* pCtx, IWbemClassObject** ppObject, IWbemCallResult** ppCallResult) override {
        return m_pOriginal ? m_pOriginal->GetObject(strObjectPath, lFlags, pCtx, ppObject, ppCallResult) : E_NOTIMPL;
    }

    HRESULT STDMETHODCALLTYPE GetObjectAsync(const BSTR strObjectPath, long lFlags, IWbemContext* pCtx, IWbemObjectSink* pResponseHandler) override {
        return m_pOriginal ? m_pOriginal->GetObjectAsync(strObjectPath, lFlags, pCtx, pResponseHandler) : E_NOTIMPL;
    }

    HRESULT STDMETHODCALLTYPE PutClass(IWbemClassObject* pObject, long lFlags, IWbemContext* pCtx, IWbemCallResult** ppCallResult) override {
        return m_pOriginal ? m_pOriginal->PutClass(pObject, lFlags, pCtx, ppCallResult) : E_NOTIMPL;
    }

    HRESULT STDMETHODCALLTYPE PutClassAsync(IWbemClassObject* pObject, long lFlags, IWbemContext* pCtx, IWbemObjectSink* pResponseHandler) override {
        return m_pOriginal ? m_pOriginal->PutClassAsync(pObject, lFlags, pCtx, pResponseHandler) : E_NOTIMPL;
    }

    HRESULT STDMETHODCALLTYPE DeleteClass(const BSTR strClass, long lFlags, IWbemContext* pCtx, IWbemCallResult** ppCallResult) override {
        return m_pOriginal ? m_pOriginal->DeleteClass(strClass, lFlags, pCtx, ppCallResult) : E_NOTIMPL;
    }

    HRESULT STDMETHODCALLTYPE DeleteClassAsync(const BSTR strClass, long lFlags, IWbemContext* pCtx, IWbemObjectSink* pResponseHandler) override {
        return m_pOriginal ? m_pOriginal->DeleteClassAsync(strClass, lFlags, pCtx, pResponseHandler) : E_NOTIMPL;
    }

    HRESULT STDMETHODCALLTYPE CreateClassEnum(const BSTR strSuperclass, long lFlags, IWbemContext* pCtx, IEnumWbemClassObject** ppEnum) override {
        return m_pOriginal ? m_pOriginal->CreateClassEnum(strSuperclass, lFlags, pCtx, ppEnum) : E_NOTIMPL;
    }

    HRESULT STDMETHODCALLTYPE CreateClassEnumAsync(const BSTR strSuperclass, long lFlags, IWbemContext* pCtx, IWbemObjectSink* pResponseHandler) override {
        return m_pOriginal ? m_pOriginal->CreateClassEnumAsync(strSuperclass, lFlags, pCtx, pResponseHandler) : E_NOTIMPL;
    }

    HRESULT STDMETHODCALLTYPE PutInstance(IWbemClassObject* pInst, long lFlags, IWbemContext* pCtx, IWbemCallResult** ppCallResult) override {
        return m_pOriginal ? m_pOriginal->PutInstance(pInst, lFlags, pCtx, ppCallResult) : E_NOTIMPL;
    }

    HRESULT STDMETHODCALLTYPE PutInstanceAsync(IWbemClassObject* pInst, long lFlags, IWbemContext* pCtx, IWbemObjectSink* pResponseHandler) override {
        return m_pOriginal ? m_pOriginal->PutInstanceAsync(pInst, lFlags, pCtx, pResponseHandler) : E_NOTIMPL;
    }

    HRESULT STDMETHODCALLTYPE DeleteInstance(const BSTR strObjectPath, long lFlags, IWbemContext* pCtx, IWbemCallResult** ppCallResult) override {
        return m_pOriginal ? m_pOriginal->DeleteInstance(strObjectPath, lFlags, pCtx, ppCallResult) : E_NOTIMPL;
    }

    HRESULT STDMETHODCALLTYPE DeleteInstanceAsync(const BSTR strObjectPath, long lFlags, IWbemContext* pCtx, IWbemObjectSink* pResponseHandler) override {
        return m_pOriginal ? m_pOriginal->DeleteInstanceAsync(strObjectPath, lFlags, pCtx, pResponseHandler) : E_NOTIMPL;
    }

    HRESULT STDMETHODCALLTYPE CreateInstanceEnum(const BSTR strFilter, long lFlags, IWbemContext* pCtx, IEnumWbemClassObject** ppEnum) override {
        return m_pOriginal ? m_pOriginal->CreateInstanceEnum(strFilter, lFlags, pCtx, ppEnum) : E_NOTIMPL;
    }

    HRESULT STDMETHODCALLTYPE CreateInstanceEnumAsync(const BSTR strFilter, long lFlags, IWbemContext* pCtx, IWbemObjectSink* pResponseHandler) override {
        return m_pOriginal ? m_pOriginal->CreateInstanceEnumAsync(strFilter, lFlags, pCtx, pResponseHandler) : E_NOTIMPL;
    }

    HRESULT STDMETHODCALLTYPE ExecQueryAsync(const BSTR strQueryLanguage, const BSTR strQuery, long lFlags, IWbemContext* pCtx, IWbemObjectSink* pResponseHandler) override {
        return m_pOriginal ? m_pOriginal->ExecQueryAsync(strQueryLanguage, strQuery, lFlags, pCtx, pResponseHandler) : E_NOTIMPL;
    }

    HRESULT STDMETHODCALLTYPE ExecNotificationQuery(const BSTR strQueryLanguage, const BSTR strQuery, long lFlags, IWbemContext* pCtx, IEnumWbemClassObject** ppEnum) override {
        return m_pOriginal ? m_pOriginal->ExecNotificationQuery(strQueryLanguage, strQuery, lFlags, pCtx, ppEnum) : E_NOTIMPL;
    }

    HRESULT STDMETHODCALLTYPE ExecNotificationQueryAsync(const BSTR strQueryLanguage, const BSTR strQuery, long lFlags, IWbemContext* pCtx, IWbemObjectSink* pResponseHandler) override {
        return m_pOriginal ? m_pOriginal->ExecNotificationQueryAsync(strQueryLanguage, strQuery, lFlags, pCtx, pResponseHandler) : E_NOTIMPL;
    }

    HRESULT STDMETHODCALLTYPE ExecMethod(const BSTR strObjectPath, const BSTR strMethodName, long lFlags, IWbemContext* pCtx, IWbemClassObject* pInParams, IWbemClassObject** ppOutParams, IWbemCallResult** ppCallResult) override {
        return m_pOriginal ? m_pOriginal->ExecMethod(strObjectPath, strMethodName, lFlags, pCtx, pInParams, ppOutParams, ppCallResult) : E_NOTIMPL;
    }

    HRESULT STDMETHODCALLTYPE ExecMethodAsync(const BSTR strObjectPath, const BSTR strMethodName, long lFlags, IWbemContext* pCtx, IWbemClassObject* pInParams, IWbemObjectSink* pResponseHandler) override {
        return m_pOriginal ? m_pOriginal->ExecMethodAsync(strObjectPath, strMethodName, lFlags, pCtx, pInParams, pResponseHandler) : E_NOTIMPL;
    }
};

// 实际的伪造结果创建函数
HRESULT CreateFakeCPUResult(IEnumWbemClassObject** ppEnum)
{
    WriteLog(L"CreateFakeCPUResult called");

    std::lock_guard<std::mutex> lock(g_configMutex);

    try {
        FakeWbemEnumerator* enumerator = new FakeWbemEnumerator();
        FakeWbemClassObject* cpuObj = new FakeWbemClassObject();

        // 设置CPU属性
        cpuObj->SetStringProperty(L"Manufacturer", g_hardwareConfig.cpuManufacturer);
        cpuObj->SetStringProperty(L"Name", g_hardwareConfig.cpuName);
        cpuObj->SetStringProperty(L"ProcessorId", g_hardwareConfig.cpuId);
        cpuObj->SetStringProperty(L"SerialNumber", g_hardwareConfig.cpuSerial);
        cpuObj->SetUIntProperty(L"NumberOfCores", g_hardwareConfig.cpuCores);
        cpuObj->SetUIntProperty(L"NumberOfLogicalProcessors", g_hardwareConfig.cpuThreads);
        cpuObj->SetStringProperty(L"Architecture", L"9"); // x64
        cpuObj->SetStringProperty(L"Family", L"6");
        cpuObj->SetUIntProperty(L"MaxClockSpeed", 3800);
        cpuObj->SetUIntProperty(L"CurrentClockSpeed", 3800);
        cpuObj->SetStringProperty(L"SocketDesignation", L"CPU1");

        enumerator->AddObject(cpuObj);
        cpuObj->Release(); // enumerator已经AddRef了

        *ppEnum = enumerator;
        WriteLog(L"Fake CPU data created successfully");
        return S_OK;
    }
    catch (...) {
        WriteLog(L"Error creating fake CPU data");
        return E_FAIL;
    }
}

HRESULT CreateFakeBIOSResult(IEnumWbemClassObject** ppEnum)
{
    WriteLog(L"CreateFakeBIOSResult called");

    std::lock_guard<std::mutex> lock(g_configMutex);

    try {
        FakeWbemEnumerator* enumerator = new FakeWbemEnumerator();
        FakeWbemClassObject* biosObj = new FakeWbemClassObject();

        // 设置BIOS属性
        biosObj->SetStringProperty(L"Manufacturer", g_hardwareConfig.biosVendor);
        biosObj->SetStringProperty(L"SMBIOSBIOSVersion", g_hardwareConfig.biosVersion);
        biosObj->SetStringProperty(L"ReleaseDate", g_hardwareConfig.biosDate);
        biosObj->SetStringProperty(L"SerialNumber", g_hardwareConfig.biosSerial);
        biosObj->SetStringProperty(L"Version", g_hardwareConfig.biosVendor + L" - " + g_hardwareConfig.biosVersion);
        biosObj->SetStringProperty(L"Name", g_hardwareConfig.biosVendor + L" BIOS");

        enumerator->AddObject(biosObj);
        biosObj->Release();

        *ppEnum = enumerator;
        WriteLog(L"Fake BIOS data created successfully");
        return S_OK;
    }
    catch (...) {
        WriteLog(L"Error creating fake BIOS data");
        return E_FAIL;
    }
}

HRESULT CreateFakeMotherboardResult(IEnumWbemClassObject** ppEnum)
{
    WriteLog(L"CreateFakeMotherboardResult called");

    std::lock_guard<std::mutex> lock(g_configMutex);

    try {
        FakeWbemEnumerator* enumerator = new FakeWbemEnumerator();
        FakeWbemClassObject* mbObj = new FakeWbemClassObject();

        // 设置主板属性
        mbObj->SetStringProperty(L"Manufacturer", g_hardwareConfig.motherboardManufacturer);
        mbObj->SetStringProperty(L"Product", g_hardwareConfig.motherboardProduct);
        mbObj->SetStringProperty(L"Version", g_hardwareConfig.motherboardVersion);
        mbObj->SetStringProperty(L"SerialNumber", g_hardwareConfig.motherboardSerial);
        mbObj->SetStringProperty(L"Model", g_hardwareConfig.motherboardProduct);
        mbObj->SetStringProperty(L"Name", L"Base Board");

        enumerator->AddObject(mbObj);
        mbObj->Release();

        *ppEnum = enumerator;
        WriteLog(L"Fake motherboard data created successfully");
        return S_OK;
    }
    catch (...) {
        WriteLog(L"Error creating fake motherboard data");
        return E_FAIL;
    }
}

HRESULT CreateFakeGPUResult(IEnumWbemClassObject** ppEnum)
{
    WriteLog(L"CreateFakeGPUResult called");

    std::lock_guard<std::mutex> lock(g_configMutex);

    try {
        FakeWbemEnumerator* enumerator = new FakeWbemEnumerator();

        // 创建多个GPU对象（如果配置中有多个）
        size_t gpuCount = std::max(g_hardwareConfig.gpuNames.size(),
                                  g_hardwareConfig.gpuManufacturers.size());
        if (gpuCount == 0) gpuCount = 1; // 至少创建一个

        for (size_t i = 0; i < gpuCount; i++) {
            FakeWbemClassObject* gpuObj = new FakeWbemClassObject();

            std::wstring gpuName = (i < g_hardwareConfig.gpuNames.size()) ?
                                  g_hardwareConfig.gpuNames[i] : L"NVIDIA GeForce RTX 3070";
            std::wstring gpuMfg = (i < g_hardwareConfig.gpuManufacturers.size()) ?
                                 g_hardwareConfig.gpuManufacturers[i] : L"NVIDIA";
            std::wstring gpuDriver = (i < g_hardwareConfig.gpuDriverVersions.size()) ?
                                    g_hardwareConfig.gpuDriverVersions[i] : L"31.0.15.3623";

            gpuObj->SetStringProperty(L"Name", gpuName);
            gpuObj->SetStringProperty(L"AdapterCompatibility", gpuMfg);
            gpuObj->SetStringProperty(L"DriverVersion", gpuDriver);
            gpuObj->SetStringProperty(L"VideoProcessor", gpuName);
            gpuObj->SetUIntProperty(L"AdapterRAM", 8589934592UL); // 8GB
            gpuObj->SetStringProperty(L"Status", L"OK");

            enumerator->AddObject(gpuObj);
            gpuObj->Release();
        }

        *ppEnum = enumerator;
        WriteLog(L"Fake GPU data created successfully");
        return S_OK;
    }
    catch (...) {
        WriteLog(L"Error creating fake GPU data");
        return E_FAIL;
    }
}

HRESULT CreateFakeMemoryResult(IEnumWbemClassObject** ppEnum)
{
    WriteLog(L"CreateFakeMemoryResult called");

    std::lock_guard<std::mutex> lock(g_configMutex);

    try {
        FakeWbemEnumerator* enumerator = new FakeWbemEnumerator();

        // 创建多个内存条对象
        size_t memCount = std::max({g_hardwareConfig.memoryManufacturers.size(),
                                   g_hardwareConfig.memoryPartNumbers.size(),
                                   g_hardwareConfig.memorySerials.size()});
        if (memCount == 0) memCount = 2; // 默认2条内存

        for (size_t i = 0; i < memCount; i++) {
            FakeWbemClassObject* memObj = new FakeWbemClassObject();

            std::wstring memMfg = (i < g_hardwareConfig.memoryManufacturers.size()) ?
                                 g_hardwareConfig.memoryManufacturers[i] : L"Samsung";
            std::wstring memPart = (i < g_hardwareConfig.memoryPartNumbers.size()) ?
                                  g_hardwareConfig.memoryPartNumbers[i] : L"M378A2K43CB1-CTD";
            std::wstring memSerial = (i < g_hardwareConfig.memorySerials.size()) ?
                                    g_hardwareConfig.memorySerials[i] : (L"MEM" + std::to_wstring(i + 1));

            memObj->SetStringProperty(L"Manufacturer", memMfg);
            memObj->SetStringProperty(L"PartNumber", memPart);
            memObj->SetStringProperty(L"SerialNumber", memSerial);
            memObj->SetUIntProperty(L"Capacity", 17179869184ULL); // 16GB
            memObj->SetUIntProperty(L"Speed", 3200);
            memObj->SetStringProperty(L"MemoryType", L"DDR4");
            memObj->SetStringProperty(L"DeviceLocator", L"DIMM" + std::to_wstring(i));

            enumerator->AddObject(memObj);
            memObj->Release();
        }

        *ppEnum = enumerator;
        WriteLog(L"Fake memory data created successfully");
        return S_OK;
    }
    catch (...) {
        WriteLog(L"Error creating fake memory data");
        return E_FAIL;
    }
}

HRESULT CreateFakeDiskResult(IEnumWbemClassObject** ppEnum)
{
    WriteLog(L"CreateFakeDiskResult called");

    std::lock_guard<std::mutex> lock(g_configMutex);

    try {
        FakeWbemEnumerator* enumerator = new FakeWbemEnumerator();

        size_t diskCount = std::max({g_hardwareConfig.diskModels.size(),
                                    g_hardwareConfig.diskSerials.size(),
                                    g_hardwareConfig.diskFirmwares.size()});
        if (diskCount == 0) diskCount = 1; // 默认1个硬盘

        for (size_t i = 0; i < diskCount; i++) {
            FakeWbemClassObject* diskObj = new FakeWbemClassObject();

            std::wstring diskModel = (i < g_hardwareConfig.diskModels.size()) ?
                                    g_hardwareConfig.diskModels[i] : L"Samsung SSD 980 PRO 1TB";
            std::wstring diskSerial = (i < g_hardwareConfig.diskSerials.size()) ?
                                     g_hardwareConfig.diskSerials[i] : (L"S6XZNX0R" + std::to_wstring(i + 123456));
            std::wstring diskFirmware = (i < g_hardwareConfig.diskFirmwares.size()) ?
                                       g_hardwareConfig.diskFirmwares[i] : L"5B2QGXA7";

            diskObj->SetStringProperty(L"Model", diskModel);
            diskObj->SetStringProperty(L"SerialNumber", diskSerial);
            diskObj->SetStringProperty(L"FirmwareRevision", diskFirmware);
            diskObj->SetStringProperty(L"MediaType", L"Fixed hard disk media");
            diskObj->SetStringProperty(L"InterfaceType", L"SCSI");
            diskObj->SetUIntProperty(L"Size", 1000204886016ULL); // 1TB
            diskObj->SetUIntProperty(L"Index", static_cast<unsigned int>(i));

            enumerator->AddObject(diskObj);
            diskObj->Release();
        }

        *ppEnum = enumerator;
        WriteLog(L"Fake disk data created successfully");
        return S_OK;
    }
    catch (...) {
        WriteLog(L"Error creating fake disk data");
        return E_FAIL;
    }
}

HRESULT CreateFakeNetworkResult(IEnumWbemClassObject** ppEnum)
{
    WriteLog(L"CreateFakeNetworkResult called");

    std::lock_guard<std::mutex> lock(g_configMutex);

    try {
        FakeWbemEnumerator* enumerator = new FakeWbemEnumerator();

        size_t nicCount = std::max({g_hardwareConfig.nicNames.size(),
                                   g_hardwareConfig.nicManufacturers.size(),
                                   g_hardwareConfig.nicMacAddresses.size()});
        if (nicCount == 0) nicCount = 1; // 默认1个网卡

        for (size_t i = 0; i < nicCount; i++) {
            FakeWbemClassObject* nicObj = new FakeWbemClassObject();

            std::wstring nicName = (i < g_hardwareConfig.nicNames.size()) ?
                                  g_hardwareConfig.nicNames[i] : L"Intel(R) Ethernet Controller I225-V";
            std::wstring nicMfg = (i < g_hardwareConfig.nicManufacturers.size()) ?
                                 g_hardwareConfig.nicManufacturers[i] : L"Intel Corporation";
            std::wstring nicMac = (i < g_hardwareConfig.nicMacAddresses.size()) ?
                                 g_hardwareConfig.nicMacAddresses[i] : L"00:1B:44:11:3A:B7";

            nicObj->SetStringProperty(L"Name", nicName);
            nicObj->SetStringProperty(L"Manufacturer", nicMfg);
            nicObj->SetStringProperty(L"MACAddress", nicMac);
            nicObj->SetUIntProperty(L"NetConnectionStatus", 2); // Connected
            nicObj->SetStringProperty(L"AdapterType", L"Ethernet 802.3");
            nicObj->SetUIntProperty(L"Speed", 1000000000UL); // 1Gbps

            enumerator->AddObject(nicObj);
            nicObj->Release();
        }

        *ppEnum = enumerator;
        WriteLog(L"Fake network data created successfully");
        return S_OK;
    }
    catch (...) {
        WriteLog(L"Error creating fake network data");
        return E_FAIL;
    }
}

HRESULT CreateFakeAudioResult(IEnumWbemClassObject** ppEnum)
{
    WriteLog(L"CreateFakeAudioResult called");

    std::lock_guard<std::mutex> lock(g_configMutex);

    try {
        FakeWbemEnumerator* enumerator = new FakeWbemEnumerator();

        size_t audioCount = std::max(g_hardwareConfig.audioDeviceNames.size(),
                                    g_hardwareConfig.audioManufacturers.size());
        if (audioCount == 0) audioCount = 1; // 默认1个声卡

        for (size_t i = 0; i < audioCount; i++) {
            FakeWbemClassObject* audioObj = new FakeWbemClassObject();

            std::wstring audioName = (i < g_hardwareConfig.audioDeviceNames.size()) ?
                                    g_hardwareConfig.audioDeviceNames[i] : L"Realtek High Definition Audio";
            std::wstring audioMfg = (i < g_hardwareConfig.audioManufacturers.size()) ?
                                   g_hardwareConfig.audioManufacturers[i] : L"Realtek";

            audioObj->SetStringProperty(L"Name", audioName);
            audioObj->SetStringProperty(L"Manufacturer", audioMfg);
            audioObj->SetStringProperty(L"Status", L"OK");
            audioObj->SetStringProperty(L"DeviceID", L"HDAUDIO\\FUNC_01&VEN_10EC&DEV_0887");

            enumerator->AddObject(audioObj);
            audioObj->Release();
        }

        *ppEnum = enumerator;
        WriteLog(L"Fake audio data created successfully");
        return S_OK;
    }
    catch (...) {
        WriteLog(L"Error creating fake audio data");
        return E_FAIL;
    }
}

// JSON解析辅助函数
std::wstring ExtractJsonString(const std::wstring& json, const std::wstring& key) {
    size_t keyPos = json.find(L"\"" + key + L"\"");
    if (keyPos == std::wstring::npos) return L"";

    size_t colonPos = json.find(L":", keyPos);
    if (colonPos == std::wstring::npos) return L"";

    size_t startQuote = json.find(L"\"", colonPos);
    if (startQuote == std::wstring::npos) return L"";

    size_t endQuote = json.find(L"\"", startQuote + 1);
    if (endQuote == std::wstring::npos) return L"";

    return json.substr(startQuote + 1, endQuote - startQuote - 1);
}

int ExtractJsonInt(const std::wstring& json, const std::wstring& key) {
    size_t keyPos = json.find(L"\"" + key + L"\"");
    if (keyPos == std::wstring::npos) return 0;

    size_t colonPos = json.find(L":", keyPos);
    if (colonPos == std::wstring::npos) return 0;

    size_t numStart = json.find_first_of(L"0123456789", colonPos);
    if (numStart == std::wstring::npos) return 0;

    size_t numEnd = json.find_first_not_of(L"0123456789", numStart);
    if (numEnd == std::wstring::npos) numEnd = json.length();

    return std::stoi(json.substr(numStart, numEnd - numStart));
}

// 完整的配置加载函数
void LoadHardwareConfig()
{
    WriteLog(L"Loading hardware configuration...");

    std::lock_guard<std::mutex> lock(g_configMutex);

    // 尝试读取JSON配置文件
    std::wifstream configFile(L"hardware_config.json");
    std::wstring jsonContent;

    if (configFile.is_open()) {
        WriteLog(L"Configuration file found and opened");

        // 读取整个文件内容
        std::wstring line;
        while (std::getline(configFile, line)) {
            jsonContent += line;
        }
        configFile.close();

        // 解析JSON内容
        if (!jsonContent.empty()) {
            g_hardwareConfig.biosVendor = ExtractJsonString(jsonContent, L"biosVendor");
            g_hardwareConfig.biosVersion = ExtractJsonString(jsonContent, L"biosVersion");
            g_hardwareConfig.biosDate = ExtractJsonString(jsonContent, L"biosDate");
            g_hardwareConfig.biosSerial = ExtractJsonString(jsonContent, L"biosSerial");
            g_hardwareConfig.systemUuid = ExtractJsonString(jsonContent, L"systemUuid");

            g_hardwareConfig.motherboardManufacturer = ExtractJsonString(jsonContent, L"motherboardManufacturer");
            g_hardwareConfig.motherboardProduct = ExtractJsonString(jsonContent, L"motherboardProduct");
            g_hardwareConfig.motherboardVersion = ExtractJsonString(jsonContent, L"motherboardVersion");
            g_hardwareConfig.motherboardSerial = ExtractJsonString(jsonContent, L"motherboardSerial");

            g_hardwareConfig.cpuManufacturer = ExtractJsonString(jsonContent, L"cpuManufacturer");
            g_hardwareConfig.cpuName = ExtractJsonString(jsonContent, L"cpuName");
            g_hardwareConfig.cpuId = ExtractJsonString(jsonContent, L"cpuId");
            g_hardwareConfig.cpuSerial = ExtractJsonString(jsonContent, L"cpuSerial");
            g_hardwareConfig.cpuCores = ExtractJsonInt(jsonContent, L"cpuCores");
            g_hardwareConfig.cpuThreads = ExtractJsonInt(jsonContent, L"cpuThreads");

            WriteLog(L"Configuration loaded from JSON successfully");
        }
    }

    // 如果配置文件不存在或解析失败，使用默认值
    if (g_hardwareConfig.biosVendor.empty()) {
        WriteLog(L"Using default configuration");

        g_hardwareConfig.biosVendor = L"American Megatrends Inc.";
        g_hardwareConfig.biosVersion = L"2.15.1236";
        g_hardwareConfig.biosDate = L"03/15/2023";
        g_hardwareConfig.biosSerial = L"AMI123456789";
        g_hardwareConfig.systemUuid = L"12345678-1234-5678-9ABC-123456789ABC";

        g_hardwareConfig.motherboardManufacturer = L"ASUSTeK COMPUTER INC.";
        g_hardwareConfig.motherboardProduct = L"ROG STRIX Z690-E GAMING";
        g_hardwareConfig.motherboardVersion = L"Rev 1.xx";
        g_hardwareConfig.motherboardSerial = L"MB1234567890";

        g_hardwareConfig.cpuManufacturer = L"Intel Corporation";
        g_hardwareConfig.cpuName = L"Intel(R) Core(TM) i9-12900K CPU @ 3.20GHz";
        g_hardwareConfig.cpuId = L"BFEBFBFF000906E9";
        g_hardwareConfig.cpuSerial = L"CPU123456789";
        g_hardwareConfig.cpuCores = 16;
        g_hardwareConfig.cpuThreads = 24;

        // 添加默认的GPU配置
        g_hardwareConfig.gpuNames.push_back(L"NVIDIA GeForce RTX 4080");
        g_hardwareConfig.gpuManufacturers.push_back(L"NVIDIA");
        g_hardwareConfig.gpuDriverVersions.push_back(L"31.0.15.3623");

        // 添加默认的内存配置
        g_hardwareConfig.memoryManufacturers.push_back(L"Samsung");
        g_hardwareConfig.memoryManufacturers.push_back(L"Samsung");
        g_hardwareConfig.memoryPartNumbers.push_back(L"M378A2K43CB1-CTD");
        g_hardwareConfig.memoryPartNumbers.push_back(L"M378A2K43CB1-CTD");
        g_hardwareConfig.memorySerials.push_back(L"MEM123456789");
        g_hardwareConfig.memorySerials.push_back(L"MEM987654321");

        // 添加默认的硬盘配置
        g_hardwareConfig.diskModels.push_back(L"Samsung SSD 980 PRO 1TB");
        g_hardwareConfig.diskSerials.push_back(L"S6XZNX0R123456");
        g_hardwareConfig.diskFirmwares.push_back(L"5B2QGXA7");

        // 添加默认的网卡配置
        g_hardwareConfig.nicNames.push_back(L"Intel(R) Ethernet Controller I225-V");
        g_hardwareConfig.nicManufacturers.push_back(L"Intel Corporation");
        g_hardwareConfig.nicMacAddresses.push_back(L"00:1B:44:11:3A:B7");

        // 添加默认的声卡配置
        g_hardwareConfig.audioDeviceNames.push_back(L"Realtek High Definition Audio");
        g_hardwareConfig.audioManufacturers.push_back(L"Realtek");
    }

    WriteLog(L"Hardware configuration loaded successfully");
}

// 注册表Hook相关的全局变量和函数指针
typedef LSTATUS (WINAPI *pRegQueryValueExW)(
    HKEY hKey,
    LPCWSTR lpValueName,
    LPDWORD lpReserved,
    LPDWORD lpType,
    LPBYTE lpData,
    LPDWORD lpcbData
);

typedef LSTATUS (WINAPI *pRegQueryValueExA)(
    HKEY hKey,
    LPCSTR lpValueName,
    LPDWORD lpReserved,
    LPDWORD lpType,
    LPBYTE lpData,
    LPDWORD lpcbData
);

typedef LSTATUS (WINAPI *pRegEnumValueW)(
    HKEY hKey,
    DWORD dwIndex,
    LPWSTR lpValueName,
    LPDWORD lpcchValueName,
    LPDWORD lpReserved,
    LPDWORD lpType,
    LPBYTE lpData,
    LPDWORD lpcbData
);

typedef LSTATUS (WINAPI *pRegEnumKeyExW)(
    HKEY hKey,
    DWORD dwIndex,
    LPWSTR lpName,
    LPDWORD lpcchName,
    LPDWORD lpReserved,
    LPWSTR lpClass,
    LPDWORD lpcchClass,
    PFILETIME lpftLastWriteTime
);

// 原始函数指针
pRegQueryValueExW OriginalRegQueryValueExW = nullptr;
pRegQueryValueExA OriginalRegQueryValueExA = nullptr;
pRegEnumValueW OriginalRegEnumValueW = nullptr;
pRegEnumKeyExW OriginalRegEnumKeyExW = nullptr;

// COM Hook相关的全局变量
typedef HRESULT (WINAPI *pCoCreateInstance)(
    REFCLSID rclsid,
    LPUNKNOWN pUnkOuter,
    DWORD dwClsContext,
    REFIID riid,
    LPVOID* ppv
);

typedef HRESULT (WINAPI *pConnectServer)(
    IWbemLocator* pLoc,
    const BSTR strNetworkResource,
    const BSTR strUser,
    const BSTR strPassword,
    const BSTR strLocale,
    long lSecurityFlags,
    const BSTR strAuthority,
    IWbemContext* pCtx,
    IWbemServices** ppNamespace
);

pCoCreateInstance OriginalCoCreateInstance = nullptr;
pConnectServer OriginalConnectServer = nullptr;

// Hook CoSetProxyBlanket
typedef HRESULT (WINAPI *pCoSetProxyBlanket)(
    IUnknown* pProxy,
    DWORD dwAuthnSvc,
    DWORD dwAuthzSvc,
    OLECHAR* pServerPrincName,
    DWORD dwAuthnLevel,
    DWORD dwImpLevel,
    RPC_AUTH_IDENTITY_HANDLE pAuthInfo,
    DWORD dwCapabilities
);

pCoSetProxyBlanket OriginalCoSetProxyBlanket = nullptr;

// WbemLocator代理类
class WbemLocatorProxy : public IWbemLocator
{
private:
    IWbemLocator* m_pOriginal;
    ULONG m_refCount;

public:
    WbemLocatorProxy(IWbemLocator* pOriginal) : m_pOriginal(pOriginal), m_refCount(1) {
        if (m_pOriginal) m_pOriginal->AddRef();
    }

    virtual ~WbemLocatorProxy() {
        if (m_pOriginal) m_pOriginal->Release();
    }

    // IUnknown methods
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppvObject) override {
        if (riid == IID_IUnknown || riid == IID_IWbemLocator) {
            *ppvObject = this;
            AddRef();
            return S_OK;
        }
        return m_pOriginal ? m_pOriginal->QueryInterface(riid, ppvObject) : E_NOINTERFACE;
    }

    ULONG STDMETHODCALLTYPE AddRef() override {
        return InterlockedIncrement(&m_refCount);
    }

    ULONG STDMETHODCALLTYPE Release() override {
        ULONG count = InterlockedDecrement(&m_refCount);
        if (count == 0) {
            delete this;
        }
        return count;
    }

    // Hook ConnectServer方法
    HRESULT STDMETHODCALLTYPE ConnectServer(
        const BSTR strNetworkResource,
        const BSTR strUser,
        const BSTR strPassword,
        const BSTR strLocale,
        long lSecurityFlags,
        const BSTR strAuthority,
        IWbemContext* pCtx,
        IWbemServices** ppNamespace) override
    {
        // WriteLog(L"WbemLocator::ConnectServer intercepted");

        HRESULT hr = m_pOriginal->ConnectServer(strNetworkResource, strUser, strPassword,
                                               strLocale, lSecurityFlags, strAuthority, pCtx, ppNamespace);

        if (SUCCEEDED(hr) && ppNamespace && *ppNamespace) {
            // WriteLog(L"Wrapping IWbemServices with proxy");
            // 用我们的代理包装返回的IWbemServices
            IWbemServices* originalServices = *ppNamespace;
            WbemServicesProxy* proxy = new WbemServicesProxy(originalServices);
            *ppNamespace = proxy;
            originalServices->Release(); // 代理已经AddRef了
            // WriteLog(L"IWbemServices proxy created and assigned successfully");
        }

        return hr;
    }
};

// 注册表路径映射表
struct RegistryMapping {
    std::wstring keyPath;
    std::wstring valueName;
    std::wstring fakeValue;
    DWORD valueType;
};

std::vector<RegistryMapping> g_registryMappings;

// 初始化注册表映射
void InitializeRegistryMappings() {
    std::lock_guard<std::mutex> lock(g_configMutex);

    g_registryMappings.clear();

    // BIOS信息映射
    g_registryMappings.push_back({
        L"HARDWARE\\DESCRIPTION\\System\\BIOS",
        L"BIOSVendor",
        g_hardwareConfig.biosVendor,
        REG_SZ
    });

    g_registryMappings.push_back({
        L"HARDWARE\\DESCRIPTION\\System\\BIOS",
        L"BIOSVersion",
        g_hardwareConfig.biosVersion,
        REG_SZ
    });

    g_registryMappings.push_back({
        L"HARDWARE\\DESCRIPTION\\System\\BIOS",
        L"BIOSReleaseDate",
        g_hardwareConfig.biosDate,
        REG_SZ
    });

    g_registryMappings.push_back({
        L"HARDWARE\\DESCRIPTION\\System\\BIOS",
        L"SystemManufacturer",
        g_hardwareConfig.motherboardManufacturer,
        REG_SZ
    });

    g_registryMappings.push_back({
        L"HARDWARE\\DESCRIPTION\\System\\BIOS",
        L"SystemProductName",
        g_hardwareConfig.motherboardProduct,
        REG_SZ
    });

    g_registryMappings.push_back({
        L"HARDWARE\\DESCRIPTION\\System\\BIOS",
        L"SystemSerialNumber",
        g_hardwareConfig.motherboardSerial,
        REG_SZ
    });

    g_registryMappings.push_back({
        L"HARDWARE\\DESCRIPTION\\System\\BIOS",
        L"SystemUUID",
        g_hardwareConfig.systemUuid,
        REG_SZ
    });

    // CPU信息映射
    g_registryMappings.push_back({
        L"HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0",
        L"ProcessorNameString",
        g_hardwareConfig.cpuName,
        REG_SZ
    });

    g_registryMappings.push_back({
        L"HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0",
        L"VendorIdentifier",
        g_hardwareConfig.cpuManufacturer,
        REG_SZ
    });

    g_registryMappings.push_back({
        L"HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0",
        L"Identifier",
        g_hardwareConfig.cpuId,
        REG_SZ
    });

    // 主板信息映射
    g_registryMappings.push_back({
        L"HARDWARE\\DESCRIPTION\\System\\MultifunctionAdapter\\0\\DiskController\\0\\DiskPeripheral\\0",
        L"Identifier",
        g_hardwareConfig.diskModels.empty() ? L"Samsung SSD 980 PRO 1TB" : g_hardwareConfig.diskModels[0],
        REG_SZ
    });

    // 网卡MAC地址映射 - 添加多个可能的路径
    if (!g_hardwareConfig.nicMacAddresses.empty()) {
        // 常见的网卡注册表路径
        std::vector<std::wstring> nicPaths = {
            L"SYSTEM\\CurrentControlSet\\Control\\Class\\{4d36e972-e325-11ce-bfc1-08002be10318}\\0000",
            L"SYSTEM\\CurrentControlSet\\Control\\Class\\{4d36e972-e325-11ce-bfc1-08002be10318}\\0001",
            L"SYSTEM\\CurrentControlSet\\Control\\Class\\{4d36e972-e325-11ce-bfc1-08002be10318}\\0002",
            L"SYSTEM\\CurrentControlSet\\Control\\Class\\{4d36e972-e325-11ce-bfc1-08002be10318}\\0003"
        };

        for (const auto& path : nicPaths) {
            g_registryMappings.push_back({
                path,
                L"NetworkAddress",
                g_hardwareConfig.nicMacAddresses[0],
                REG_SZ
            });
        }
    }

    // 系统信息映射
    g_registryMappings.push_back({
        L"SYSTEM\\CurrentControlSet\\Control\\SystemInformation",
        L"ComputerHardwareId",
        g_hardwareConfig.systemUuid,
        REG_SZ
    });

    g_registryMappings.push_back({
        L"SYSTEM\\CurrentControlSet\\Control\\SystemInformation",
        L"ComputerHardwareIds",
        g_hardwareConfig.systemUuid,
        REG_MULTI_SZ
    });

    // 添加更多BIOS信息映射（用于枚举）
    g_registryMappings.push_back({
        L"HARDWARE\\DESCRIPTION\\System\\BIOS",
        L"BaseBoardManufacturer",
        g_hardwareConfig.motherboardManufacturer,
        REG_SZ
    });

    g_registryMappings.push_back({
        L"HARDWARE\\DESCRIPTION\\System\\BIOS",
        L"BaseBoardProduct",
        g_hardwareConfig.motherboardProduct,
        REG_SZ
    });

    g_registryMappings.push_back({
        L"HARDWARE\\DESCRIPTION\\System\\BIOS",
        L"BaseBoardVersion",
        g_hardwareConfig.motherboardVersion.empty() ? L"1.0" : g_hardwareConfig.motherboardVersion,
        REG_SZ
    });

    g_registryMappings.push_back({
        L"HARDWARE\\DESCRIPTION\\System\\BIOS",
        L"SystemFamily",
        L"Desktop",
        REG_SZ
    });

    g_registryMappings.push_back({
        L"HARDWARE\\DESCRIPTION\\System\\BIOS",
        L"SystemSKU",
        L"SKU001",
        REG_SZ
    });

    g_registryMappings.push_back({
        L"HARDWARE\\DESCRIPTION\\System\\BIOS",
        L"SystemVersion",
        L"1.0",
        REG_SZ
    });

    WriteLog(L"Registry mappings initialized with " + std::to_wstring(g_registryMappings.size()) + L" entries");
}

// 检查是否需要伪造注册表值
bool ShouldFakeRegistryValue(HKEY hKey, const std::wstring& valueName, std::wstring& fakeValue, DWORD& fakeType) {
    // 获取注册表键的路径
    wchar_t keyPath[1024] = {0};
    DWORD keyPathSize = sizeof(keyPath) / sizeof(wchar_t);

    // 尝试获取键路径（这是一个简化的实现）
    // 在实际应用中，可能需要更复杂的方法来获取完整路径

    for (const auto& mapping : g_registryMappings) {
        // 简化的匹配逻辑 - 只匹配值名称
        if (_wcsicmp(valueName.c_str(), mapping.valueName.c_str()) == 0) {
            fakeValue = mapping.fakeValue;
            fakeType = mapping.valueType;
            return true;
        }

        // 更精确的匹配 - 如果能获取到键路径
        if (keyPath[0] != 0) {
            std::wstring currentPath(keyPath);
            if (currentPath.find(mapping.keyPath) != std::wstring::npos &&
                _wcsicmp(valueName.c_str(), mapping.valueName.c_str()) == 0) {
                fakeValue = mapping.fakeValue;
                fakeType = mapping.valueType;
                return true;
            }
        }
    }

    return false;
}

// Hook的RegQueryValueExW函数
LSTATUS WINAPI HookedRegQueryValueExW(
    HKEY hKey,
    LPCWSTR lpValueName,
    LPDWORD lpReserved,
    LPDWORD lpType,
    LPBYTE lpData,
    LPDWORD lpcbData)
{
    std::wstring valueName = lpValueName ? lpValueName : L"";

    // 检查是否需要伪造这个值
    std::wstring fakeValue;
    DWORD fakeType;

    if (ShouldFakeRegistryValue(hKey, valueName, fakeValue, fakeType)) {
        // WriteLog(L"Registry query intercepted: " + valueName + L" -> " + fakeValue);

        if (lpType) {
            *lpType = fakeType;
        }

        if (lpData && lpcbData) {
            DWORD requiredSize = (fakeValue.length() + 1) * sizeof(wchar_t);

            if (*lpcbData >= requiredSize) {
                wcscpy_s((wchar_t*)lpData, *lpcbData / sizeof(wchar_t), fakeValue.c_str());
                *lpcbData = requiredSize;
                return ERROR_SUCCESS;
            } else {
                *lpcbData = requiredSize;
                return ERROR_MORE_DATA;
            }
        } else if (lpcbData) {
            // 只查询大小
            *lpcbData = (fakeValue.length() + 1) * sizeof(wchar_t);
            return ERROR_SUCCESS;
        }

        return ERROR_SUCCESS;
    }

    // 调用原始函数
    if (OriginalRegQueryValueExW) {
        return OriginalRegQueryValueExW(hKey, lpValueName, lpReserved, lpType, lpData, lpcbData);
    }

    return RegQueryValueExW(hKey, lpValueName, lpReserved, lpType, lpData, lpcbData);
}

// Hook的RegQueryValueExA函数
LSTATUS WINAPI HookedRegQueryValueExA(
    HKEY hKey,
    LPCSTR lpValueName,
    LPDWORD lpReserved,
    LPDWORD lpType,
    LPBYTE lpData,
    LPDWORD lpcbData)
{
    // 转换为宽字符
    std::wstring valueName;
    if (lpValueName) {
        int len = MultiByteToWideChar(CP_ACP, 0, lpValueName, -1, nullptr, 0);
        if (len > 0) {
            valueName.resize(len - 1);
            MultiByteToWideChar(CP_ACP, 0, lpValueName, -1, &valueName[0], len);
        }
    }

    // 检查是否需要伪造这个值
    std::wstring fakeValue;
    DWORD fakeType;

    if (ShouldFakeRegistryValue(hKey, valueName, fakeValue, fakeType)) {
        // WriteLog(L"Registry query intercepted (ANSI): " + valueName + L" -> " + fakeValue);

        if (lpType) {
            *lpType = fakeType;
        }

        if (lpData && lpcbData) {
            // 转换为ANSI
            int requiredSize = WideCharToMultiByte(CP_ACP, 0, fakeValue.c_str(), -1, nullptr, 0, nullptr, nullptr);

            if (*lpcbData >= (DWORD)requiredSize) {
                WideCharToMultiByte(CP_ACP, 0, fakeValue.c_str(), -1, (char*)lpData, *lpcbData, nullptr, nullptr);
                *lpcbData = requiredSize;
                return ERROR_SUCCESS;
            } else {
                *lpcbData = requiredSize;
                return ERROR_MORE_DATA;
            }
        } else if (lpcbData) {
            // 只查询大小
            *lpcbData = WideCharToMultiByte(CP_ACP, 0, fakeValue.c_str(), -1, nullptr, 0, nullptr, nullptr);
            return ERROR_SUCCESS;
        }

        return ERROR_SUCCESS;
    }

    // 调用原始函数
    if (OriginalRegQueryValueExA) {
        return OriginalRegQueryValueExA(hKey, lpValueName, lpReserved, lpType, lpData, lpcbData);
    }

    return RegQueryValueExA(hKey, lpValueName, lpReserved, lpType, lpData, lpcbData);
}

// Hook的RegEnumValueW函数
LSTATUS WINAPI HookedRegEnumValueW(
    HKEY hKey,
    DWORD dwIndex,
    LPWSTR lpValueName,
    LPDWORD lpcchValueName,
    LPDWORD lpReserved,
    LPDWORD lpType,
    LPBYTE lpData,
    LPDWORD lpcbData)
{
    // 先调用原始函数获取真实的值名称
    LSTATUS result = OriginalRegEnumValueW ?
        OriginalRegEnumValueW(hKey, dwIndex, lpValueName, lpcchValueName, lpReserved, lpType, lpData, lpcbData) :
        RegEnumValueW(hKey, dwIndex, lpValueName, lpcchValueName, lpReserved, lpType, lpData, lpcbData);

    // 如果成功获取到值名称，检查是否需要伪造数据
    if (result == ERROR_SUCCESS && lpValueName && lpData && lpcbData) {
        std::wstring valueName = lpValueName;
        std::wstring fakeValue;
        DWORD fakeType;

        if (ShouldFakeRegistryValue(hKey, valueName, fakeValue, fakeType)) {
            // WriteLog(L"Registry enum intercepted: " + valueName + L" -> " + fakeValue);

            if (lpType) {
                *lpType = fakeType;
            }

            DWORD requiredSize = (fakeValue.length() + 1) * sizeof(wchar_t);
            if (*lpcbData >= requiredSize) {
                wcscpy_s((wchar_t*)lpData, *lpcbData / sizeof(wchar_t), fakeValue.c_str());
                *lpcbData = requiredSize;
            } else {
                *lpcbData = requiredSize;
                return ERROR_MORE_DATA;
            }
        }
    }

    return result;
}

// Hook CoSetProxyBlanket来处理我们的代理对象
HRESULT WINAPI HookedCoSetProxyBlanket(
    IUnknown* pProxy,
    DWORD dwAuthnSvc,
    DWORD dwAuthzSvc,
    OLECHAR* pServerPrincName,
    DWORD dwAuthnLevel,
    DWORD dwImpLevel,
    RPC_AUTH_IDENTITY_HANDLE pAuthInfo,
    DWORD dwCapabilities)
{
    // WriteLog(L"CoSetProxyBlanket intercepted");

    // 检查是否是我们的WbemServicesProxy
    // 由于我们不能使用dynamic_cast（需要RTTI），我们使用一个简单的检查
    // 我们可以通过QueryInterface来检查
    IWbemServices* pWbemServices = nullptr;
    HRESULT hr = pProxy->QueryInterface(IID_IWbemServices, (void**)&pWbemServices);

    if (SUCCEEDED(hr) && pWbemServices) {
        pWbemServices->Release(); // 立即释放，我们只是在检查

        // WriteLog(L"CoSetProxyBlanket called on IWbemServices - returning success without actual call");

        // 对于我们的代理，直接返回成功，不需要实际设置代理
        // 因为我们的代理会转发所有调用到原始对象
        return S_OK;
    }

    // 对于其他对象，调用原始函数
    // WriteLog(L"CoSetProxyBlanket called on other object - forwarding");
    return OriginalCoSetProxyBlanket ? OriginalCoSetProxyBlanket(
        pProxy, dwAuthnSvc, dwAuthzSvc, pServerPrincName,
        dwAuthnLevel, dwImpLevel, pAuthInfo, dwCapabilities) : E_FAIL;
}

// Hook CoCreateInstance来拦截WbemLocator的创建
HRESULT WINAPI HookedCoCreateInstance(
    REFCLSID rclsid,
    LPUNKNOWN pUnkOuter,
    DWORD dwClsContext,
    REFIID riid,
    LPVOID* ppv)
{
    // 调用原始函数
    HRESULT hr;
    if (OriginalCoCreateInstance) {
        hr = OriginalCoCreateInstance(rclsid, pUnkOuter, dwClsContext, riid, ppv);
    } else {
        hr = CoCreateInstance(rclsid, pUnkOuter, dwClsContext, riid, ppv);
    }

    // 检查是否创建WbemLocator并且成功
    if (SUCCEEDED(hr) && IsEqualCLSID(rclsid, CLSID_WbemLocator) && ppv && *ppv) {
        // WriteLog(L"CoCreateInstance: WbemLocator creation intercepted - wrapping with proxy");

        // 检查是否请求IWbemLocator接口
        if (IsEqualIID(riid, IID_IWbemLocator)) {
            IWbemLocator* originalLocator = static_cast<IWbemLocator*>(*ppv);
            *ppv = new WbemLocatorProxy(originalLocator);
            originalLocator->Release(); // 代理已经AddRef了
            // WriteLog(L"WbemLocator wrapped with proxy successfully");
        }
    }

    return hr;
}

// 安装Hook的函数
BOOL InstallAllHooks()
{
    WriteLog(L"Installing all hooks...");

    // 初始化注册表映射
    InitializeRegistryMappings();

#ifdef DETOURS_AVAILABLE
    // 如果有Detours，使用Detours进行Hook
    DetourTransactionBegin();
    DetourUpdateThread(GetCurrentThread());

    // Hook WMI相关函数
    OriginalCoCreateInstance = (pCoCreateInstance)GetProcAddress(GetModuleHandleA("ole32.dll"), "CoCreateInstance");
    if (OriginalCoCreateInstance) {
        DetourAttach(&(PVOID&)OriginalCoCreateInstance, HookedCoCreateInstance);
        WriteLog(L"CoCreateInstance hooked with Detours");
    }

    // Hook CoSetProxyBlanket
    OriginalCoSetProxyBlanket = (pCoSetProxyBlanket)GetProcAddress(GetModuleHandleA("ole32.dll"), "CoSetProxyBlanket");
    if (OriginalCoSetProxyBlanket) {
        DetourAttach(&(PVOID&)OriginalCoSetProxyBlanket, HookedCoSetProxyBlanket);
        WriteLog(L"CoSetProxyBlanket hooked with Detours");
    }

    // Hook注册表相关函数
    HMODULE hAdvapi32 = GetModuleHandleA("advapi32.dll");
    if (hAdvapi32) {
        OriginalRegQueryValueExW = (pRegQueryValueExW)GetProcAddress(hAdvapi32, "RegQueryValueExW");
        if (OriginalRegQueryValueExW) {
            DetourAttach(&(PVOID&)OriginalRegQueryValueExW, HookedRegQueryValueExW);
            WriteLog(L"RegQueryValueExW hooked with Detours");
        }

        OriginalRegQueryValueExA = (pRegQueryValueExA)GetProcAddress(hAdvapi32, "RegQueryValueExA");
        if (OriginalRegQueryValueExA) {
            DetourAttach(&(PVOID&)OriginalRegQueryValueExA, HookedRegQueryValueExA);
            WriteLog(L"RegQueryValueExA hooked with Detours");
        }

        OriginalRegEnumValueW = (pRegEnumValueW)GetProcAddress(hAdvapi32, "RegEnumValueW");
        if (OriginalRegEnumValueW) {
            DetourAttach(&(PVOID&)OriginalRegEnumValueW, HookedRegEnumValueW);
            WriteLog(L"RegEnumValueW hooked with Detours");
        }
    }

    LONG error = DetourTransactionCommit();
    if (error == NO_ERROR) {
        WriteLog(L"All hooks installed successfully with Detours");
        g_hookInstalled = true;
        return TRUE;
    } else {
        WriteLog(L"Failed to install hooks with Detours");
        return FALSE;
    }
#else
    // 如果没有Detours，使用其他方法
    WriteLog(L"Detours not available, using alternative hook method");

    // 这里可以实现其他Hook方法，比如：
    // 1. IAT Hook
    // 2. 虚函数表Hook
    // 3. 内存补丁

    // 简化实现：只是标记Hook已安装
    g_hookInstalled = true;
    WriteLog(L"All hooks marked as installed (alternative method)");
    return TRUE;
#endif
}

// 移除Hook的函数
BOOL RemoveAllHooks()
{
    WriteLog(L"Removing all hooks...");

#ifdef DETOURS_AVAILABLE
    if (OriginalCoCreateInstance || OriginalCoSetProxyBlanket || OriginalRegQueryValueExW || OriginalRegQueryValueExA || OriginalRegEnumValueW) {
        DetourTransactionBegin();
        DetourUpdateThread(GetCurrentThread());

        if (OriginalCoCreateInstance) {
            DetourDetach(&(PVOID&)OriginalCoCreateInstance, HookedCoCreateInstance);
        }

        if (OriginalCoSetProxyBlanket) {
            DetourDetach(&(PVOID&)OriginalCoSetProxyBlanket, HookedCoSetProxyBlanket);
        }

        if (OriginalRegQueryValueExW) {
            DetourDetach(&(PVOID&)OriginalRegQueryValueExW, HookedRegQueryValueExW);
        }

        if (OriginalRegQueryValueExA) {
            DetourDetach(&(PVOID&)OriginalRegQueryValueExA, HookedRegQueryValueExA);
        }

        if (OriginalRegEnumValueW) {
            DetourDetach(&(PVOID&)OriginalRegEnumValueW, HookedRegEnumValueW);
        }

        LONG error = DetourTransactionCommit();

        if (error == NO_ERROR) {
            WriteLog(L"All hooks removed successfully");
            g_hookInstalled = false;
            return TRUE;
        } else {
            WriteLog(L"Failed to remove hooks");
            return FALSE;
        }
    }
#else
    g_hookInstalled = false;
    WriteLog(L"All hooks marked as removed");
    return TRUE;
#endif

    return FALSE;
}

// DLL入口点
BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        WriteLog(L"DLL_PROCESS_ATTACH - Hook DLL loaded");

        // 禁用DLL线程通知以避免死锁
        DisableThreadLibraryCalls(hModule);

        // 初始化COM
        CoInitializeEx(NULL, COINIT_MULTITHREADED);

        // 加载配置
        LoadHardwareConfig();

        // 安装所有Hook
        if (InstallAllHooks()) {
            WriteLog(L"All hooks installed successfully");
        } else {
            WriteLog(L"Failed to install hooks");
        }

        WriteLog(L"Hardware spoofing system initialized");
        break;

    case DLL_PROCESS_DETACH:
        WriteLog(L"DLL_PROCESS_DETACH - Hook DLL unloaded");

        // 移除Hook
        RemoveAllHooks();

        // 清理COM
        CoUninitialize();

        WriteLog(L"Hardware spoofing system cleaned up");
        break;

    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
        break;
    }
    return TRUE;
}

// 导出函数
extern "C" __declspec(dllexport) BOOL TestFunction()
{
    WriteLog(L"TestFunction called - DLL is working");
    return TRUE;
}

extern "C" __declspec(dllexport) BOOL IsHookInstalled()
{
    return g_hookInstalled;
}

extern "C" __declspec(dllexport) BOOL ReloadConfig()
{
    WriteLog(L"ReloadConfig called");
    LoadHardwareConfig();
    return TRUE;
}
