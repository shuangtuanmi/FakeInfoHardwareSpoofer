#ifndef SYSTEMINFO_H
#define SYSTEMINFO_H

#include <QString>
#include <QStringList>
#include <QDateTime>
#include <QJsonObject>
#include <QJsonArray>
#include <QNetworkInterface>
#include <QAbstractSocket>

struct SystemInformation {
    // 操作系统信息
    QString osName;
    QString osVersion;
    QString osBuild;
    QString computerName;
    QString productId;
    QString productKey;
    QDateTime installDate;
    QDateTime bootTime;
    
    // 网络信息
    QStringList ipAddresses;
    QStringList macAddresses;
    
    // BIOS信息
    QString biosVendor;
    QString biosVersion;
    QString biosDate;
    QString biosSerial;
    QString systemUuid;
    
    // 主板信息
    QString motherboardManufacturer;
    QString motherboardProduct;
    QString motherboardVersion;
    QString motherboardSerial;
    
    // 机箱信息
    QString chassisManufacturer;
    QString chassisType;
    QString chassisSerial;
    
    // 处理器信息
    QString cpuManufacturer;
    QString cpuName;
    QString cpuId;
    QString cpuSerial;
    int cpuCores;
    int cpuThreads;
    
    // 内存信息
    QStringList memoryManufacturers;
    QStringList memoryPartNumbers;
    QStringList memorySerials;
    quint64 totalMemory;
    
    // 硬盘信息
    QStringList diskModels;
    QStringList diskSerials;
    QStringList diskFirmwares;
    QStringList diskSizes;
    
    // 显卡信息
    QStringList gpuNames;
    QStringList gpuManufacturers;
    QStringList gpuDriverVersions;
    
    // 显示器信息
    QStringList monitorNames;
    QStringList monitorManufacturers;
    QStringList monitorSerials;
    
    // 网卡信息
    QStringList nicNames;
    QStringList nicManufacturers;
    QStringList nicMacAddresses;
    
    // 声卡信息
    QStringList audioDeviceNames;
    QStringList audioManufacturers;
};

class SystemInfo
{
public:
    static SystemInformation getCurrentSystemInfo();
    static QJsonObject systemInfoToJson(const SystemInformation& info);
    static SystemInformation systemInfoFromJson(const QJsonObject& json);
    
    // 获取特定硬件信息
    static QString getWindowsProductKey();
    static QDateTime getWindowsInstallDate();
    static QDateTime getSystemBootTime();
    static QStringList getNetworkIpAddresses();
    static QStringList getNetworkMacAddresses();
    
    // WMI查询辅助函数
    static QStringList queryWMI(const QString& query, const QString& property);
    static QString queryWMISingle(const QString& query, const QString& property);
    
private:
    static QString executeCommand(const QString& command);
    static QString getRegistryValue(const QString& keyPath, const QString& valueName);
};

#endif // SYSTEMINFO_H
