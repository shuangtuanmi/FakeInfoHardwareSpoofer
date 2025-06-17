#include "SystemInfo.h"
#include <QProcess>
#include <QSettings>
#include <QNetworkInterface>
#include <QHostInfo>
#include <QJsonArray>
#include <QDebug>
#include <Windows.h>
#include <comdef.h>
#include <Wbemidl.h>

#pragma comment(lib, "wbemuuid.lib")

SystemInformation SystemInfo::getCurrentSystemInfo()
{
    SystemInformation info;
    
    // 操作系统信息
    info.osName = queryWMISingle("SELECT Caption FROM Win32_OperatingSystem", "Caption");
    info.osVersion = queryWMISingle("SELECT Version FROM Win32_OperatingSystem", "Version");
    info.osBuild = queryWMISingle("SELECT BuildNumber FROM Win32_OperatingSystem", "BuildNumber");
    info.computerName = queryWMISingle("SELECT Name FROM Win32_ComputerSystem", "Name");
    info.productId = getRegistryValue("HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion", "ProductId");
    info.productKey = getWindowsProductKey();
    info.installDate = getWindowsInstallDate();
    info.bootTime = getSystemBootTime();
    
    // 网络信息
    info.ipAddresses = getNetworkIpAddresses();
    info.macAddresses = getNetworkMacAddresses();
    
    // BIOS信息
    info.biosVendor = queryWMISingle("SELECT Manufacturer FROM Win32_BIOS", "Manufacturer");
    info.biosVersion = queryWMISingle("SELECT SMBIOSBIOSVersion FROM Win32_BIOS", "SMBIOSBIOSVersion");
    info.biosDate = queryWMISingle("SELECT ReleaseDate FROM Win32_BIOS", "ReleaseDate");
    info.biosSerial = queryWMISingle("SELECT SerialNumber FROM Win32_BIOS", "SerialNumber");
    info.systemUuid = queryWMISingle("SELECT UUID FROM Win32_ComputerSystemProduct", "UUID");
    
    // 主板信息
    info.motherboardManufacturer = queryWMISingle("SELECT Manufacturer FROM Win32_BaseBoard", "Manufacturer");
    info.motherboardProduct = queryWMISingle("SELECT Product FROM Win32_BaseBoard", "Product");
    info.motherboardVersion = queryWMISingle("SELECT Version FROM Win32_BaseBoard", "Version");
    info.motherboardSerial = queryWMISingle("SELECT SerialNumber FROM Win32_BaseBoard", "SerialNumber");
    
    // 机箱信息
    info.chassisManufacturer = queryWMISingle("SELECT Manufacturer FROM Win32_SystemEnclosure", "Manufacturer");
    info.chassisType = queryWMISingle("SELECT ChassisTypes FROM Win32_SystemEnclosure", "ChassisTypes");
    info.chassisSerial = queryWMISingle("SELECT SerialNumber FROM Win32_SystemEnclosure", "SerialNumber");
    
    // 处理器信息
    info.cpuManufacturer = queryWMISingle("SELECT Manufacturer FROM Win32_Processor", "Manufacturer");
    info.cpuName = queryWMISingle("SELECT Name FROM Win32_Processor", "Name");
    info.cpuId = queryWMISingle("SELECT ProcessorId FROM Win32_Processor", "ProcessorId");
    info.cpuCores = queryWMISingle("SELECT NumberOfCores FROM Win32_Processor", "NumberOfCores").toInt();
    info.cpuThreads = queryWMISingle("SELECT NumberOfLogicalProcessors FROM Win32_Processor", "NumberOfLogicalProcessors").toInt();
    
    // 内存信息
    info.memoryManufacturers = queryWMI("SELECT Manufacturer FROM Win32_PhysicalMemory", "Manufacturer");
    info.memoryPartNumbers = queryWMI("SELECT PartNumber FROM Win32_PhysicalMemory", "PartNumber");
    info.memorySerials = queryWMI("SELECT SerialNumber FROM Win32_PhysicalMemory", "SerialNumber");
    info.totalMemory = queryWMISingle("SELECT TotalPhysicalMemory FROM Win32_ComputerSystem", "TotalPhysicalMemory").toULongLong();
    
    // 硬盘信息
    info.diskModels = queryWMI("SELECT Model FROM Win32_DiskDrive WHERE MediaType='Fixed hard disk media'", "Model");
    info.diskSerials = queryWMI("SELECT SerialNumber FROM Win32_DiskDrive WHERE MediaType='Fixed hard disk media'", "SerialNumber");
    info.diskFirmwares = queryWMI("SELECT FirmwareRevision FROM Win32_DiskDrive WHERE MediaType='Fixed hard disk media'", "FirmwareRevision");
    info.diskSizes = queryWMI("SELECT Size FROM Win32_DiskDrive WHERE MediaType='Fixed hard disk media'", "Size");
    
    // 显卡信息
    info.gpuNames = queryWMI("SELECT Name FROM Win32_VideoController", "Name");
    info.gpuManufacturers = queryWMI("SELECT AdapterCompatibility FROM Win32_VideoController", "AdapterCompatibility");
    info.gpuDriverVersions = queryWMI("SELECT DriverVersion FROM Win32_VideoController", "DriverVersion");
    
    // 显示器信息
    info.monitorNames = queryWMI("SELECT Name FROM Win32_DesktopMonitor", "Name");
    info.monitorManufacturers = queryWMI("SELECT MonitorManufacturer FROM Win32_DesktopMonitor", "MonitorManufacturer");
    
    // 网卡信息
    info.nicNames = queryWMI("SELECT Name FROM Win32_NetworkAdapter WHERE NetConnectionStatus=2", "Name");
    info.nicManufacturers = queryWMI("SELECT Manufacturer FROM Win32_NetworkAdapter WHERE NetConnectionStatus=2", "Manufacturer");
    info.nicMacAddresses = queryWMI("SELECT MACAddress FROM Win32_NetworkAdapter WHERE NetConnectionStatus=2", "MACAddress");
    
    // 声卡信息
    info.audioDeviceNames = queryWMI("SELECT Name FROM Win32_SoundDevice", "Name");
    info.audioManufacturers = queryWMI("SELECT Manufacturer FROM Win32_SoundDevice", "Manufacturer");
    
    return info;
}

QString SystemInfo::queryWMISingle(const QString& query, const QString& property)
{
    QStringList results = queryWMI(query, property);
    return results.isEmpty() ? QString() : results.first();
}

QStringList SystemInfo::queryWMI(const QString& query, const QString& property)
{
    QStringList results;

    // 尝试初始化COM，如果已经初始化则忽略错误
    HRESULT hres = CoInitializeEx(0, COINIT_MULTITHREADED);
    bool comInitialized = SUCCEEDED(hres) || hres == RPC_E_CHANGED_MODE;

    if (!comInitialized) {
        qDebug() << "Failed to initialize COM:" << QString::number(hres, 16);
        return results;
    }

    // 只在我们成功初始化COM时才设置安全性
    if (SUCCEEDED(hres)) {
        hres = CoInitializeSecurity(NULL, -1, NULL, NULL, RPC_C_AUTHN_LEVEL_DEFAULT,
                                   RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE, NULL);
        // 忽略安全性设置失败，因为可能已经设置过了
    }

    IWbemLocator *pLoc = NULL;
    hres = CoCreateInstance(CLSID_WbemLocator, 0, CLSCTX_INPROC_SERVER,
                           IID_IWbemLocator, (LPVOID *)&pLoc);

    if (FAILED(hres)) {
        qDebug() << "Failed to create WbemLocator:" << QString::number(hres, 16);
        if (SUCCEEDED(CoInitializeEx(0, COINIT_MULTITHREADED))) {
            CoUninitialize();
        }
        return results;
    }

    IWbemServices *pSvc = NULL;
    hres = pLoc->ConnectServer(_bstr_t(L"ROOT\\CIMV2"), NULL, NULL, 0, NULL, 0, 0, &pSvc);

    if (FAILED(hres)) {
        qDebug() << "Failed to connect to WMI:" << QString::number(hres, 16);
        pLoc->Release();
        if (SUCCEEDED(CoInitializeEx(0, COINIT_MULTITHREADED))) {
            CoUninitialize();
        }
        return results;
    }

    hres = CoSetProxyBlanket(pSvc, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE, NULL,
                            RPC_C_AUTHN_LEVEL_CALL, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE);

    IEnumWbemClassObject* pEnumerator = NULL;
    hres = pSvc->ExecQuery(bstr_t("WQL"), bstr_t(query.toStdWString().c_str()),
                          WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY, NULL, &pEnumerator);

    if (FAILED(hres)) {
        qDebug() << "Failed to execute WMI query:" << query << "Error:" << QString::number(hres, 16);
    } else {
        IWbemClassObject *pclsObj = NULL;
        ULONG uReturn = 0;

        while (pEnumerator) {
            HRESULT hr = pEnumerator->Next(WBEM_INFINITE, 1, &pclsObj, &uReturn);
            if (0 == uReturn) break;

            VARIANT vtProp;
            VariantInit(&vtProp);
            hr = pclsObj->Get(property.toStdWString().c_str(), 0, &vtProp, 0, 0);
            if (SUCCEEDED(hr) && vtProp.vt != VT_NULL && vtProp.vt != VT_EMPTY) {
                QString value;

                // 根据VARIANT类型处理不同的数据类型
                switch (vtProp.vt) {
                case VT_BSTR:
                    if (vtProp.bstrVal) {
                        value = QString::fromWCharArray(vtProp.bstrVal);
                    }
                    break;
                case VT_I4:
                    value = QString::number(vtProp.lVal);
                    break;
                case VT_UI4:
                    value = QString::number(vtProp.ulVal);
                    break;
                case VT_I8:
                    value = QString::number(vtProp.llVal);
                    break;
                case VT_UI8:
                    value = QString::number(vtProp.ullVal);
                    break;
                case VT_BOOL:
                    value = vtProp.boolVal ? "True" : "False";
                    break;
                case VT_ARRAY | VT_BSTR: // 字符串数组 (如 IPAddress)
                    {
                        SAFEARRAY* pArray = vtProp.parray;
                        if (pArray) {
                            LONG lBound, uBound;
                            SafeArrayGetLBound(pArray, 1, &lBound);
                            SafeArrayGetUBound(pArray, 1, &uBound);

                            QStringList arrayValues;
                            for (LONG i = lBound; i <= uBound; i++) {
                                BSTR bstrValue;
                                if (SUCCEEDED(SafeArrayGetElement(pArray, &i, &bstrValue))) {
                                    if (bstrValue) {
                                        arrayValues.append(QString::fromWCharArray(bstrValue));
                                        SysFreeString(bstrValue);
                                    }
                                }
                            }
                            value = arrayValues.join(", ");
                        }
                    }
                    break;
                case VT_ARRAY | VT_I2: // 短整型数组 (如 ChassisTypes)
                case VT_ARRAY | VT_I4: // 整型数组
                    {
                        SAFEARRAY* pArray = vtProp.parray;
                        if (pArray) {
                            LONG lBound, uBound;
                            SafeArrayGetLBound(pArray, 1, &lBound);
                            SafeArrayGetUBound(pArray, 1, &uBound);

                            QStringList arrayValues;
                            for (LONG i = lBound; i <= uBound; i++) {
                                if (vtProp.vt == (VT_ARRAY | VT_I2)) {
                                    SHORT shortValue;
                                    if (SUCCEEDED(SafeArrayGetElement(pArray, &i, &shortValue))) {
                                        arrayValues.append(QString::number(shortValue));
                                    }
                                } else {
                                    LONG longValue;
                                    if (SUCCEEDED(SafeArrayGetElement(pArray, &i, &longValue))) {
                                        arrayValues.append(QString::number(longValue));
                                    }
                                }
                            }
                            value = arrayValues.join(", ");
                        }
                    }
                    break;
                default:
                    qDebug() << "Unsupported VARIANT type:" << vtProp.vt << "for property:" << property;
                    break;
                }

                if (!value.isEmpty()) {
                    results.append(value);
                }
            }
            VariantClear(&vtProp);
            pclsObj->Release();
        }
        pEnumerator->Release();
    }

    pSvc->Release();
    pLoc->Release();

    // 只在我们成功初始化COM时才清理
    if (SUCCEEDED(CoInitializeEx(0, COINIT_MULTITHREADED))) {
        CoUninitialize();
    }

    return results;
}

QString SystemInfo::getWindowsProductKey()
{
    // 这里需要实现从注册表读取产品密钥的逻辑
    // 由于安全原因，实际的产品密钥可能无法直接读取
    return getRegistryValue("HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion", "DigitalProductId");
}

QDateTime SystemInfo::getWindowsInstallDate()
{
    QString installDate = getRegistryValue("HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion", "InstallDate");
    if (!installDate.isEmpty()) {
        bool ok;
        qint64 timestamp = installDate.toLongLong(&ok);
        if (ok) {
            return QDateTime::fromSecsSinceEpoch(timestamp);
        }
    }
    return QDateTime();
}

QDateTime SystemInfo::getSystemBootTime()
{
    QString bootTime = queryWMISingle("SELECT LastBootUpTime FROM Win32_OperatingSystem", "LastBootUpTime");
    if (!bootTime.isEmpty()) {
        // WMI时间格式: YYYYMMDDHHMMSS.MMMMMM+UUU
        QDateTime dt = QDateTime::fromString(bootTime.left(14), "yyyyMMddhhmmss");
        return dt;
    }
    return QDateTime();
}

QStringList SystemInfo::getNetworkIpAddresses()
{
    QStringList addresses;
    // 暂时使用WMI查询网络信息
    addresses = queryWMI("SELECT IPAddress FROM Win32_NetworkAdapterConfiguration WHERE IPEnabled=True", "IPAddress");
    return addresses;
}

QStringList SystemInfo::getNetworkMacAddresses()
{
    QStringList addresses;
    // 暂时使用WMI查询MAC地址
    addresses = queryWMI("SELECT MACAddress FROM Win32_NetworkAdapterConfiguration WHERE IPEnabled=True", "MACAddress");
    return addresses;
}

QString SystemInfo::getRegistryValue(const QString& keyPath, const QString& valueName)
{
    QSettings settings(keyPath, QSettings::NativeFormat);
    return settings.value(valueName).toString();
}

QString SystemInfo::executeCommand(const QString& command)
{
    QProcess process;
    process.start(command, QStringList());
    process.waitForFinished();
    return QString::fromLocal8Bit(process.readAllStandardOutput()).trimmed();
}

QJsonObject SystemInfo::systemInfoToJson(const SystemInformation& info)
{
    QJsonObject json;

    // 操作系统信息
    json["osName"] = info.osName;
    json["osVersion"] = info.osVersion;
    json["osBuild"] = info.osBuild;
    json["computerName"] = info.computerName;
    json["productId"] = info.productId;
    json["productKey"] = info.productKey;
    json["installDate"] = info.installDate.toString(Qt::ISODate);
    json["bootTime"] = info.bootTime.toString(Qt::ISODate);

    // 网络信息
    json["ipAddresses"] = QJsonArray::fromStringList(info.ipAddresses);
    json["macAddresses"] = QJsonArray::fromStringList(info.macAddresses);

    // BIOS信息
    json["biosVendor"] = info.biosVendor;
    json["biosVersion"] = info.biosVersion;
    json["biosDate"] = info.biosDate;
    json["biosSerial"] = info.biosSerial;
    json["systemUuid"] = info.systemUuid;

    // 主板信息
    json["motherboardManufacturer"] = info.motherboardManufacturer;
    json["motherboardProduct"] = info.motherboardProduct;
    json["motherboardVersion"] = info.motherboardVersion;
    json["motherboardSerial"] = info.motherboardSerial;

    // 机箱信息
    json["chassisManufacturer"] = info.chassisManufacturer;
    json["chassisType"] = info.chassisType;
    json["chassisSerial"] = info.chassisSerial;

    // 处理器信息
    json["cpuManufacturer"] = info.cpuManufacturer;
    json["cpuName"] = info.cpuName;
    json["cpuId"] = info.cpuId;
    json["cpuSerial"] = info.cpuSerial;
    json["cpuCores"] = info.cpuCores;
    json["cpuThreads"] = info.cpuThreads;

    // 内存信息
    json["memoryManufacturers"] = QJsonArray::fromStringList(info.memoryManufacturers);
    json["memoryPartNumbers"] = QJsonArray::fromStringList(info.memoryPartNumbers);
    json["memorySerials"] = QJsonArray::fromStringList(info.memorySerials);
    json["totalMemory"] = (qint64)info.totalMemory;

    // 硬盘信息
    json["diskModels"] = QJsonArray::fromStringList(info.diskModels);
    json["diskSerials"] = QJsonArray::fromStringList(info.diskSerials);
    json["diskFirmwares"] = QJsonArray::fromStringList(info.diskFirmwares);
    json["diskSizes"] = QJsonArray::fromStringList(info.diskSizes);

    // 显卡信息
    json["gpuNames"] = QJsonArray::fromStringList(info.gpuNames);
    json["gpuManufacturers"] = QJsonArray::fromStringList(info.gpuManufacturers);
    json["gpuDriverVersions"] = QJsonArray::fromStringList(info.gpuDriverVersions);

    // 显示器信息
    json["monitorNames"] = QJsonArray::fromStringList(info.monitorNames);
    json["monitorManufacturers"] = QJsonArray::fromStringList(info.monitorManufacturers);
    json["monitorSerials"] = QJsonArray::fromStringList(info.monitorSerials);

    // 网卡信息
    json["nicNames"] = QJsonArray::fromStringList(info.nicNames);
    json["nicManufacturers"] = QJsonArray::fromStringList(info.nicManufacturers);
    json["nicMacAddresses"] = QJsonArray::fromStringList(info.nicMacAddresses);

    // 声卡信息
    json["audioDeviceNames"] = QJsonArray::fromStringList(info.audioDeviceNames);
    json["audioManufacturers"] = QJsonArray::fromStringList(info.audioManufacturers);

    return json;
}

SystemInformation SystemInfo::systemInfoFromJson(const QJsonObject& json)
{
    SystemInformation info;

    // 操作系统信息
    info.osName = json["osName"].toString();
    info.osVersion = json["osVersion"].toString();
    info.osBuild = json["osBuild"].toString();
    info.computerName = json["computerName"].toString();
    info.productId = json["productId"].toString();
    info.productKey = json["productKey"].toString();
    info.installDate = QDateTime::fromString(json["installDate"].toString(), Qt::ISODate);
    info.bootTime = QDateTime::fromString(json["bootTime"].toString(), Qt::ISODate);

    // 网络信息
    QJsonArray ipArray = json["ipAddresses"].toArray();
    for (int i = 0; i < ipArray.size(); ++i) {
        info.ipAddresses.append(ipArray[i].toString());
    }
    QJsonArray macArray = json["macAddresses"].toArray();
    for (int i = 0; i < macArray.size(); ++i) {
        info.macAddresses.append(macArray[i].toString());
    }

    // BIOS信息
    info.biosVendor = json["biosVendor"].toString();
    info.biosVersion = json["biosVersion"].toString();
    info.biosDate = json["biosDate"].toString();
    info.biosSerial = json["biosSerial"].toString();
    info.systemUuid = json["systemUuid"].toString();

    // 主板信息
    info.motherboardManufacturer = json["motherboardManufacturer"].toString();
    info.motherboardProduct = json["motherboardProduct"].toString();
    info.motherboardVersion = json["motherboardVersion"].toString();
    info.motherboardSerial = json["motherboardSerial"].toString();

    // 机箱信息
    info.chassisManufacturer = json["chassisManufacturer"].toString();
    info.chassisType = json["chassisType"].toString();
    info.chassisSerial = json["chassisSerial"].toString();

    // 处理器信息
    info.cpuManufacturer = json["cpuManufacturer"].toString();
    info.cpuName = json["cpuName"].toString();
    info.cpuId = json["cpuId"].toString();
    info.cpuSerial = json["cpuSerial"].toString();
    info.cpuCores = json["cpuCores"].toInt();
    info.cpuThreads = json["cpuThreads"].toInt();

    // 内存信息
    QJsonArray memMfgArray = json["memoryManufacturers"].toArray();
    for (int i = 0; i < memMfgArray.size(); ++i) {
        info.memoryManufacturers.append(memMfgArray[i].toString());
    }
    QJsonArray memPnArray = json["memoryPartNumbers"].toArray();
    for (int i = 0; i < memPnArray.size(); ++i) {
        info.memoryPartNumbers.append(memPnArray[i].toString());
    }
    QJsonArray memSerArray = json["memorySerials"].toArray();
    for (int i = 0; i < memSerArray.size(); ++i) {
        info.memorySerials.append(memSerArray[i].toString());
    }
    info.totalMemory = json["totalMemory"].toVariant().toULongLong();

    // 硬盘信息
    QJsonArray diskModelArray = json["diskModels"].toArray();
    for (int i = 0; i < diskModelArray.size(); ++i) {
        info.diskModels.append(diskModelArray[i].toString());
    }
    QJsonArray diskSerArray = json["diskSerials"].toArray();
    for (int i = 0; i < diskSerArray.size(); ++i) {
        info.diskSerials.append(diskSerArray[i].toString());
    }
    QJsonArray diskFwArray = json["diskFirmwares"].toArray();
    for (int i = 0; i < diskFwArray.size(); ++i) {
        info.diskFirmwares.append(diskFwArray[i].toString());
    }
    QJsonArray diskSizeArray = json["diskSizes"].toArray();
    for (int i = 0; i < diskSizeArray.size(); ++i) {
        info.diskSizes.append(diskSizeArray[i].toString());
    }

    // 显卡信息
    QJsonArray gpuNameArray = json["gpuNames"].toArray();
    for (int i = 0; i < gpuNameArray.size(); ++i) {
        info.gpuNames.append(gpuNameArray[i].toString());
    }
    QJsonArray gpuMfgArray = json["gpuManufacturers"].toArray();
    for (int i = 0; i < gpuMfgArray.size(); ++i) {
        info.gpuManufacturers.append(gpuMfgArray[i].toString());
    }
    QJsonArray gpuDrvArray = json["gpuDriverVersions"].toArray();
    for (int i = 0; i < gpuDrvArray.size(); ++i) {
        info.gpuDriverVersions.append(gpuDrvArray[i].toString());
    }

    // 网卡信息
    QJsonArray nicNameArray = json["nicNames"].toArray();
    for (int i = 0; i < nicNameArray.size(); ++i) {
        info.nicNames.append(nicNameArray[i].toString());
    }
    QJsonArray nicMfgArray = json["nicManufacturers"].toArray();
    for (int i = 0; i < nicMfgArray.size(); ++i) {
        info.nicManufacturers.append(nicMfgArray[i].toString());
    }
    QJsonArray nicMacArray = json["nicMacAddresses"].toArray();
    for (int i = 0; i < nicMacArray.size(); ++i) {
        info.nicMacAddresses.append(nicMacArray[i].toString());
    }

    // 声卡信息
    QJsonArray audioNameArray = json["audioDeviceNames"].toArray();
    for (int i = 0; i < audioNameArray.size(); ++i) {
        info.audioDeviceNames.append(audioNameArray[i].toString());
    }
    QJsonArray audioMfgArray = json["audioManufacturers"].toArray();
    for (int i = 0; i < audioMfgArray.size(); ++i) {
        info.audioManufacturers.append(audioMfgArray[i].toString());
    }

    return info;
}
