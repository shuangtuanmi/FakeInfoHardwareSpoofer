#ifndef HARDWARERANDOMIZER_H
#define HARDWARERANDOMIZER_H

#include <QString>
#include <QStringList>
#include <QJsonObject>
#include <QJsonArray>
#include <QRandomGenerator>
#include "SystemInfo.h"

class HardwareRandomizer
{
public:
    static void initializeDatabase();
    static SystemInformation generateRandomSystemInfo();
    static SystemInformation randomizeSystemInfo(const SystemInformation& original);
    
    // 随机生成特定硬件信息
    static QString generateRandomBiosVendor();
    static QString generateRandomBiosVersion();
    static QString generateRandomBiosDate();
    static QString generateRandomSerial();
    static QString generateRandomUuid();
    
    static QString generateRandomMotherboardManufacturer();
    static QString generateRandomMotherboardProduct();
    
    static QString generateRandomCpuManufacturer();
    static QString generateRandomCpuName();
    static QString generateRandomCpuId();
    
    static QString generateRandomGpuManufacturer();
    static QString generateRandomGpuName();
    
    static QString generateRandomDiskManufacturer();
    static QString generateRandomDiskModel();
    static QString generateRandomDiskSerial();
    static QString generateRandomFirmwareVersion();
    
    static QString generateRandomMemoryManufacturer();
    static QString generateRandomMemoryPartNumber();
    
    static QString generateRandomNetworkManufacturer();
    static QString generateRandomNetworkModel();
    static QString generateRandomMacAddress();
    
    static QString generateRandomAudioManufacturer();
    static QString generateRandomAudioDevice();
    
    // 辅助函数
    static QString generateRandomHexString(int length);
    static QString generateRandomAlphaNumeric(int length);
    static QString generateRandomDate(int yearStart = 2015, int yearEnd = 2023);
    
private:
    // 硬件数据库
    static QStringList biosVendors;
    static QStringList motherboardManufacturers;
    static QStringList cpuManufacturers;
    static QStringList gpuManufacturers;
    static QStringList diskManufacturers;
    static QStringList memoryManufacturers;
    static QStringList networkManufacturers;
    static QStringList audioManufacturers;
    
    static QJsonObject cpuDatabase;
    static QJsonObject gpuDatabase;
    static QJsonObject motherboardDatabase;
    static QJsonObject diskDatabase;
    
    static bool databaseInitialized;
    
    static QString getRandomFromList(const QStringList& list);
    static QJsonObject loadHardwareDatabase();
};

#endif // HARDWARERANDOMIZER_H
