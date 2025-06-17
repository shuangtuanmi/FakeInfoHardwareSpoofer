#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QtWidgets/QMainWindow>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QLabel>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QTextEdit>
#include <QtWidgets/QTabWidget>
#include <QtWidgets/QScrollArea>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QSpinBox>
#include <QtWidgets/QListWidget>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QListWidgetItem>
#include <QJsonObject>
#include <QJsonDocument>
#include <QTimer>
#include <QProcess>
#include <windows.h>
#include <tlhelp32.h>
#include <psapi.h>
#include "SystemInfo.h"
#include "HardwareRandomizer.h"

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onStartHookClicked();
    void onStopHookClicked();
    void onSaveConfigClicked();
    void onLoadConfigClicked();
    void onRefreshSystemInfoClicked();
    void onRandomizeAllClicked();
    void onRandomizeBiosClicked();
    void onRandomizeMotherboardClicked();
    void onRandomizeCpuClicked();
    void onRandomizeMemoryClicked();
    void onRandomizeDiskClicked();
    void onRandomizeGpuClicked();
    void onRandomizeNetworkClicked();
    void onRandomizeAudioClicked();
    void updateSystemInfo();
    void onBrowseTargetProgramClicked();
    void onTargetProgramPathChanged();

private:
    void setupUI();
    void createSystemInfoTab();
    void createHardwareConfigTab();
    void createControlTab();
    void updateSystemInfoDisplay();
    void updateHardwareConfigDisplay(const SystemInformation& info);
    SystemInformation getHardwareConfigFromUI();
    void setHardwareConfigToUI(const SystemInformation& info);
    QJsonObject getHardwareConfig();
    void setHardwareConfig(const QJsonObject &config);
    bool injectDLL();
    bool ejectDLL();
    void testHookFunctionality();
    bool createSuspendedProcess(const QString& programPath, const QString& arguments);
    bool resumeTargetProcess();
    void saveTargetProgramSettings();
    void loadTargetProgramSettings();
    QString getDefaultVSCodePath();
    void saveHardwareConfigSettings();
    void loadHardwareConfigSettings();

    // 主界面
    QTabWidget *tabWidget;
    QTimer *refreshTimer;

    // 系统信息显示控件
    QTextEdit *systemInfoDisplay;
    QPushButton *refreshSystemInfoBtn;

    // 硬件配置控件 - BIOS
    QLineEdit *biosVendor;
    QLineEdit *biosVersion;
    QLineEdit *biosDate;
    QLineEdit *biosSerial;
    QLineEdit *systemUuid;
    QPushButton *randomizeBiosBtn;

    // 硬件配置控件 - 主板
    QLineEdit *motherboardManufacturer;
    QLineEdit *motherboardProduct;
    QLineEdit *motherboardVersion;
    QLineEdit *motherboardSerial;
    QPushButton *randomizeMotherboardBtn;

    // 硬件配置控件 - CPU
    QLineEdit *cpuManufacturer;
    QLineEdit *cpuName;
    QLineEdit *cpuId;
    QLineEdit *cpuSerial;
    QSpinBox *cpuCores;
    QSpinBox *cpuThreads;
    QPushButton *randomizeCpuBtn;

    // 硬件配置控件 - 内存
    QLineEdit *memoryManufacturer;
    QLineEdit *memoryPartNumber;
    QLineEdit *memorySerial;
    QSpinBox *totalMemoryGB;
    QPushButton *randomizeMemoryBtn;

    // 硬件配置控件 - 硬盘
    QLineEdit *diskModel;
    QLineEdit *diskSerial;
    QLineEdit *diskFirmware;
    QLineEdit *diskSize;
    QPushButton *randomizeDiskBtn;

    // 硬件配置控件 - 显卡
    QLineEdit *gpuName;
    QLineEdit *gpuManufacturer;
    QLineEdit *gpuDriverVersion;
    QPushButton *randomizeGpuBtn;

    // 硬件配置控件 - 网卡
    QLineEdit *nicName;
    QLineEdit *nicManufacturer;
    QLineEdit *nicMacAddress;
    QPushButton *randomizeNetworkBtn;

    // 硬件配置控件 - 声卡
    QLineEdit *audioDeviceName;
    QLineEdit *audioManufacturer;
    QPushButton *randomizeAudioBtn;

    // 控制按钮
    QPushButton *startHookBtn;
    QPushButton *stopHookBtn;
    QPushButton *saveConfigBtn;
    QPushButton *loadConfigBtn;
    QPushButton *randomizeAllBtn;

    // 目标程序配置控件
    QLineEdit *targetProgramPath;
    QPushButton *browseTargetProgramBtn;
    QLineEdit *targetProgramArgs;
    QLabel *targetProgramStatus;

    // 日志显示
    QTextEdit *logTextEdit;

    // 系统信息
    SystemInformation currentSystemInfo;

    DWORD targetProcessId;
    HANDLE hProcess;
    HANDLE hTargetThread;
};

#endif // MAINWINDOW_H
