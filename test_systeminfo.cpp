#include <QCoreApplication>
#include <QDebug>
#include "SystemInfo.h"

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    
    qDebug() << "Testing SystemInfo WMI queries...";
    
    // 测试单个查询
    qDebug() << "\n=== Testing individual WMI queries ===";
    
    QString osName = SystemInfo::queryWMISingle("SELECT Caption FROM Win32_OperatingSystem", "Caption");
    qDebug() << "OS Name:" << osName;
    
    QString cpuName = SystemInfo::queryWMISingle("SELECT Name FROM Win32_Processor", "Name");
    qDebug() << "CPU Name:" << cpuName;
    
    QString biosVendor = SystemInfo::queryWMISingle("SELECT Manufacturer FROM Win32_BIOS", "Manufacturer");
    qDebug() << "BIOS Vendor:" << biosVendor;
    
    // 测试完整的系统信息获取
    qDebug() << "\n=== Testing complete system info ===";
    
    SystemInformation info = SystemInfo::getCurrentSystemInfo();
    
    qDebug() << "Complete system info:";
    qDebug() << "OS Name:" << info.osName;
    qDebug() << "OS Version:" << info.osVersion;
    qDebug() << "Computer Name:" << info.computerName;
    qDebug() << "BIOS Vendor:" << info.biosVendor;
    qDebug() << "BIOS Version:" << info.biosVersion;
    qDebug() << "CPU Manufacturer:" << info.cpuManufacturer;
    qDebug() << "CPU Name:" << info.cpuName;
    qDebug() << "CPU Cores:" << info.cpuCores;
    qDebug() << "CPU Threads:" << info.cpuThreads;
    qDebug() << "Total Memory:" << info.totalMemory;
    
    return 0;
}
