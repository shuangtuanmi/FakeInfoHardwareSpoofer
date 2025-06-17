#include "HardwareRandomizer.h"
#include <QJsonDocument>
#include <QFile>
#include <QDateTime>
#include <QDebug>

// 静态成员初始化
QStringList HardwareRandomizer::biosVendors;
QStringList HardwareRandomizer::motherboardManufacturers;
QStringList HardwareRandomizer::cpuManufacturers;
QStringList HardwareRandomizer::gpuManufacturers;
QStringList HardwareRandomizer::diskManufacturers;
QStringList HardwareRandomizer::memoryManufacturers;
QStringList HardwareRandomizer::networkManufacturers;
QStringList HardwareRandomizer::audioManufacturers;

QJsonObject HardwareRandomizer::cpuDatabase;
QJsonObject HardwareRandomizer::gpuDatabase;
QJsonObject HardwareRandomizer::motherboardDatabase;
QJsonObject HardwareRandomizer::diskDatabase;

bool HardwareRandomizer::databaseInitialized = false;

void HardwareRandomizer::initializeDatabase()
{
    if (databaseInitialized) return;
    
    // BIOS厂商
    biosVendors << "American Megatrends Inc." << "Phoenix Technologies Ltd." 
                << "Award Software International Inc." << "Insyde Corp."
                << "Dell Inc." << "Hewlett-Packard" << "LENOVO" << "ASUSTeK Computer Inc.";
    
    // 主板厂商
    motherboardManufacturers << "ASUSTeK Computer Inc." << "Gigabyte Technology Co., Ltd."
                            << "MSI" << "ASRock" << "EVGA" << "Biostar" << "ASUS"
                            << "Dell Inc." << "Hewlett-Packard" << "LENOVO";
    
    // CPU厂商
    cpuManufacturers << "Intel Corporation" << "Advanced Micro Devices, Inc."
                     << "GenuineIntel" << "AuthenticAMD";
    
    // GPU厂商
    gpuManufacturers << "NVIDIA" << "Advanced Micro Devices, Inc." << "Intel Corporation"
                     << "ATI Technologies Inc." << "NVIDIA Corporation";
    
    // 硬盘厂商
    diskManufacturers << "Samsung" << "Western Digital" << "Seagate" << "Toshiba"
                      << "Hitachi" << "SanDisk" << "Kingston" << "Crucial"
                      << "Intel" << "Corsair" << "ADATA" << "Transcend";
    
    // 内存厂商
    memoryManufacturers << "Samsung" << "SK Hynix" << "Micron Technology"
                        << "Kingston" << "Corsair" << "G.Skill" << "Crucial"
                        << "ADATA" << "Team Group" << "Patriot";
    
    // 网卡厂商
    networkManufacturers << "Intel Corporation" << "Realtek Semiconductor Co., Ltd."
                         << "Broadcom" << "Qualcomm Atheros" << "Marvell Technology Group"
                         << "Ralink Technology, Corp." << "MediaTek Inc.";
    
    // 声卡厂商
    audioManufacturers << "Realtek" << "Creative Technology Ltd" << "C-Media Electronics Inc."
                       << "VIA Technologies Inc." << "Intel Corporation"
                       << "NVIDIA Corporation" << "Advanced Micro Devices";
    
    // 加载详细硬件数据库
    QJsonObject database = loadHardwareDatabase();
    cpuDatabase = database["cpu"].toObject();
    gpuDatabase = database["gpu"].toObject();
    motherboardDatabase = database["motherboard"].toObject();
    diskDatabase = database["disk"].toObject();
    
    databaseInitialized = true;
}

SystemInformation HardwareRandomizer::generateRandomSystemInfo()
{
    initializeDatabase();
    
    SystemInformation info;
    
    // 生成随机BIOS信息
    info.biosVendor = generateRandomBiosVendor();
    info.biosVersion = generateRandomBiosVersion();
    info.biosDate = generateRandomBiosDate();
    info.biosSerial = generateRandomSerial();
    info.systemUuid = generateRandomUuid();
    
    // 生成随机主板信息
    info.motherboardManufacturer = generateRandomMotherboardManufacturer();
    info.motherboardProduct = generateRandomMotherboardProduct();
    info.motherboardVersion = "1.0";
    info.motherboardSerial = generateRandomSerial();
    
    // 生成随机机箱信息
    info.chassisManufacturer = info.motherboardManufacturer;
    info.chassisType = "Desktop";
    info.chassisSerial = generateRandomSerial();
    
    // 生成随机CPU信息
    info.cpuManufacturer = generateRandomCpuManufacturer();
    info.cpuName = generateRandomCpuName();
    info.cpuId = generateRandomCpuId();
    info.cpuSerial = generateRandomSerial();
    info.cpuCores = QRandomGenerator::global()->bounded(2, 17); // 2-16核心
    info.cpuThreads = info.cpuCores * (QRandomGenerator::global()->bounded(1, 3)); // 1-2倍线程
    
    // 生成随机内存信息
    int memorySlots = QRandomGenerator::global()->bounded(1, 5); // 1-4个内存条
    for (int i = 0; i < memorySlots; i++) {
        info.memoryManufacturers.append(generateRandomMemoryManufacturer());
        info.memoryPartNumbers.append(generateRandomMemoryPartNumber());
        info.memorySerials.append(generateRandomSerial());
    }
    info.totalMemory = (quint64)QRandomGenerator::global()->bounded(4, 65) * 1024 * 1024 * 1024; // 4GB-64GB
    
    // 生成随机硬盘信息
    int diskCount = QRandomGenerator::global()->bounded(1, 4); // 1-3个硬盘
    for (int i = 0; i < diskCount; i++) {
        info.diskModels.append(generateRandomDiskModel());
        info.diskSerials.append(generateRandomDiskSerial());
        info.diskFirmwares.append(generateRandomFirmwareVersion());
        quint64 size = (quint64)QRandomGenerator::global()->bounded(250, 4001) * 1000 * 1000 * 1000; // 250GB-4TB
        info.diskSizes.append(QString::number(size));
    }
    
    // 生成随机显卡信息
    int gpuCount = QRandomGenerator::global()->bounded(1, 3); // 1-2个显卡
    for (int i = 0; i < gpuCount; i++) {
        info.gpuNames.append(generateRandomGpuName());
        info.gpuManufacturers.append(generateRandomGpuManufacturer());
        info.gpuDriverVersions.append(QString("%1.%2.%3.%4")
                                     .arg(QRandomGenerator::global()->bounded(20, 32))
                                     .arg(QRandomGenerator::global()->bounded(10, 100))
                                     .arg(QRandomGenerator::global()->bounded(10, 100))
                                     .arg(QRandomGenerator::global()->bounded(1000, 10000)));
    }
    
    // 生成随机网卡信息
    int nicCount = QRandomGenerator::global()->bounded(1, 4); // 1-3个网卡
    for (int i = 0; i < nicCount; i++) {
        info.nicNames.append(generateRandomNetworkModel());
        info.nicManufacturers.append(generateRandomNetworkManufacturer());
        info.nicMacAddresses.append(generateRandomMacAddress());
    }
    
    // 生成随机声卡信息
    info.audioDeviceNames.append(generateRandomAudioDevice());
    info.audioManufacturers.append(generateRandomAudioManufacturer());
    
    return info;
}

QString HardwareRandomizer::generateRandomBiosVendor()
{
    initializeDatabase();
    return getRandomFromList(biosVendors);
}

QString HardwareRandomizer::generateRandomBiosVersion()
{
    return QString("%1.%2.%3")
           .arg(QRandomGenerator::global()->bounded(1, 10))
           .arg(QRandomGenerator::global()->bounded(0, 100), 2, 10, QChar('0'))
           .arg(QRandomGenerator::global()->bounded(1, 1000));
}

QString HardwareRandomizer::generateRandomBiosDate()
{
    return generateRandomDate(2018, 2024);
}

QString HardwareRandomizer::generateRandomSerial()
{
    return generateRandomAlphaNumeric(10);
}

QString HardwareRandomizer::generateRandomUuid()
{
    return QString("%1-%2-%3-%4-%5")
           .arg(generateRandomHexString(8))
           .arg(generateRandomHexString(4))
           .arg(generateRandomHexString(4))
           .arg(generateRandomHexString(4))
           .arg(generateRandomHexString(12));
}

QString HardwareRandomizer::generateRandomMotherboardManufacturer()
{
    initializeDatabase();
    return getRandomFromList(motherboardManufacturers);
}

QString HardwareRandomizer::generateRandomMotherboardProduct()
{
    QStringList products;
    products << "ROG STRIX Z690-E GAMING" << "B550M PRO-VDH WIFI" << "X570 AORUS ELITE"
             << "Z590-A PRO" << "B450 TOMAHAWK MAX" << "X299 DARK" << "Z490 GODLIKE"
             << "B550 GAMING PLUS" << "X570 CROSSHAIR VIII HERO" << "Z590 VISION G";
    return getRandomFromList(products);
}

QString HardwareRandomizer::generateRandomCpuManufacturer()
{
    initializeDatabase();
    return getRandomFromList(cpuManufacturers);
}

QString HardwareRandomizer::generateRandomCpuName()
{
    QStringList intelCpus;
    intelCpus << "Intel(R) Core(TM) i9-12900K CPU @ 3.20GHz"
              << "Intel(R) Core(TM) i7-11700K CPU @ 3.60GHz"
              << "Intel(R) Core(TM) i5-10600K CPU @ 4.10GHz"
              << "Intel(R) Core(TM) i9-10900K CPU @ 3.70GHz"
              << "Intel(R) Core(TM) i7-9700K CPU @ 3.60GHz";
    
    QStringList amdCpus;
    amdCpus << "AMD Ryzen 9 5950X 16-Core Processor"
            << "AMD Ryzen 7 5800X 8-Core Processor"
            << "AMD Ryzen 5 5600X 6-Core Processor"
            << "AMD Ryzen 9 3950X 16-Core Processor"
            << "AMD Ryzen 7 3700X 8-Core Processor";
    
    if (QRandomGenerator::global()->bounded(2) == 0) {
        return getRandomFromList(intelCpus);
    } else {
        return getRandomFromList(amdCpus);
    }
}

QString HardwareRandomizer::generateRandomCpuId()
{
    return generateRandomHexString(16).toUpper();
}

QString HardwareRandomizer::generateRandomGpuManufacturer()
{
    initializeDatabase();
    return getRandomFromList(gpuManufacturers);
}

QString HardwareRandomizer::generateRandomGpuName()
{
    QStringList nvidiaGpus;
    nvidiaGpus << "NVIDIA GeForce RTX 4090" << "NVIDIA GeForce RTX 4080"
               << "NVIDIA GeForce RTX 3080" << "NVIDIA GeForce RTX 3070"
               << "NVIDIA GeForce RTX 3060" << "NVIDIA GeForce GTX 1660 SUPER";
    
    QStringList amdGpus;
    amdGpus << "AMD Radeon RX 7900 XTX" << "AMD Radeon RX 6900 XT"
            << "AMD Radeon RX 6800 XT" << "AMD Radeon RX 6700 XT"
            << "AMD Radeon RX 5700 XT" << "AMD Radeon RX 580";
    
    if (QRandomGenerator::global()->bounded(2) == 0) {
        return getRandomFromList(nvidiaGpus);
    } else {
        return getRandomFromList(amdGpus);
    }
}

QString HardwareRandomizer::generateRandomDiskManufacturer()
{
    initializeDatabase();
    return getRandomFromList(diskManufacturers);
}

QString HardwareRandomizer::generateRandomDiskModel()
{
    QStringList models;
    models << "Samsung SSD 980 PRO 1TB" << "WD Black SN850 1TB" << "Seagate Barracuda 2TB"
           << "Samsung 970 EVO Plus 500GB" << "Crucial MX500 1TB" << "WD Blue 1TB"
           << "Seagate IronWolf 4TB" << "Kingston NV2 500GB" << "ADATA XPG SX8200 Pro 1TB";
    return getRandomFromList(models);
}

QString HardwareRandomizer::generateRandomDiskSerial()
{
    return generateRandomAlphaNumeric(20);
}

QString HardwareRandomizer::generateRandomFirmwareVersion()
{
    return QString("%1.%2.%3")
           .arg(QRandomGenerator::global()->bounded(1, 10))
           .arg(QRandomGenerator::global()->bounded(0, 100))
           .arg(QRandomGenerator::global()->bounded(0, 1000));
}

QString HardwareRandomizer::generateRandomMemoryManufacturer()
{
    initializeDatabase();
    return getRandomFromList(memoryManufacturers);
}

QString HardwareRandomizer::generateRandomMemoryPartNumber()
{
    QStringList partNumbers;
    partNumbers << "CMK16GX4M2B3200C16" << "F4-3200C16D-16GVKB" << "BLS8G4D32AESBK"
                << "CT16G4DFRA32A" << "AX4U320016G16A-SR30" << "PVS416G320C6K"
                << "TF3D416G3200HC16CDC01" << "M378A1K43CB2-CTD";
    return getRandomFromList(partNumbers);
}

QString HardwareRandomizer::generateRandomNetworkManufacturer()
{
    initializeDatabase();
    return getRandomFromList(networkManufacturers);
}

QString HardwareRandomizer::generateRandomNetworkModel()
{
    QStringList models;
    models << "Intel(R) Ethernet Controller I225-V" << "Realtek PCIe GbE Family Controller"
           << "Intel(R) Wi-Fi 6 AX200 160MHz" << "Qualcomm Atheros AR9485 Wireless Network Adapter"
           << "Broadcom NetXtreme Gigabit Ethernet" << "Marvell AVASTAR Wireless-AC Network Controller";
    return getRandomFromList(models);
}

QString HardwareRandomizer::generateRandomMacAddress()
{
    QString mac;
    for (int i = 0; i < 6; i++) {
        if (i > 0) mac += ":";
        mac += QString("%1").arg(QRandomGenerator::global()->bounded(256), 2, 16, QChar('0')).toUpper();
    }
    return mac;
}

QString HardwareRandomizer::generateRandomAudioManufacturer()
{
    initializeDatabase();
    return getRandomFromList(audioManufacturers);
}

QString HardwareRandomizer::generateRandomAudioDevice()
{
    QStringList devices;
    devices << "Realtek High Definition Audio" << "Creative Sound Blaster Z"
            << "NVIDIA High Definition Audio" << "Intel(R) Display Audio"
            << "VIA HD Audio" << "C-Media USB Audio Device";
    return getRandomFromList(devices);
}

QString HardwareRandomizer::generateRandomHexString(int length)
{
    const QString chars = "0123456789ABCDEF";
    QString result;
    for (int i = 0; i < length; i++) {
        result += chars.at(QRandomGenerator::global()->bounded(chars.length()));
    }
    return result;
}

QString HardwareRandomizer::generateRandomAlphaNumeric(int length)
{
    const QString chars = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    QString result;
    for (int i = 0; i < length; i++) {
        result += chars.at(QRandomGenerator::global()->bounded(chars.length()));
    }
    return result;
}

QString HardwareRandomizer::generateRandomDate(int yearStart, int yearEnd)
{
    int year = QRandomGenerator::global()->bounded(yearStart, yearEnd + 1);
    int month = QRandomGenerator::global()->bounded(1, 13);
    int day = QRandomGenerator::global()->bounded(1, 29);
    return QString("%1/%2/%3").arg(month, 2, 10, QChar('0'))
                              .arg(day, 2, 10, QChar('0'))
                              .arg(year);
}

QString HardwareRandomizer::getRandomFromList(const QStringList& list)
{
    if (list.isEmpty()) return QString();
    return list.at(QRandomGenerator::global()->bounded(list.size()));
}

QJsonObject HardwareRandomizer::loadHardwareDatabase()
{
    // 这里可以从文件加载更详细的硬件数据库
    // 暂时返回空对象，使用硬编码的数据
    return QJsonObject();
}
