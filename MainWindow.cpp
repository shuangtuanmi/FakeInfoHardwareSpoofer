#include "MainWindow.h"
#include <QtWidgets/QApplication>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QSplitter>
#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonArray>
#include <QFile>
#include <QTextStream>
#include <QDebug>
#include <QSettings>
#include <Windows.h>
#include <TlHelp32.h>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), targetProcessId(0), hProcess(nullptr), hTargetThread(nullptr)
{
    setupUI();
    setWindowTitle("FakeInfo Hardware Spoofer v1.0");
    resize(1200, 800);

    // 初始化随机化器
    HardwareRandomizer::initializeDatabase();

    // 设置定时器更新系统信息
    refreshTimer = new QTimer(this);
    connect(refreshTimer, &QTimer::timeout, this, &MainWindow::updateSystemInfo);
    refreshTimer->start(5000); // 每5秒更新一次

    // 初始加载系统信息
    updateSystemInfo();

    // 加载上次保存的硬件配置
    loadHardwareConfigSettings();
}

MainWindow::~MainWindow()
{
    if (hProcess) {
        CloseHandle(hProcess);
    }
    if (hTargetThread) {
        CloseHandle(hTargetThread);
    }
}

void MainWindow::setupUI()
{
    QWidget *centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);

    QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);

    // 创建标签页
    tabWidget = new QTabWidget(this);

    createSystemInfoTab();
    createHardwareConfigTab();
    createControlTab();

    mainLayout->addWidget(tabWidget);
}

void MainWindow::createSystemInfoTab()
{
    QWidget *systemInfoWidget = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout(systemInfoWidget);

    // 刷新按钮
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    refreshSystemInfoBtn = new QPushButton("刷新系统信息", this);
    buttonLayout->addWidget(refreshSystemInfoBtn);
    buttonLayout->addStretch();
    layout->addLayout(buttonLayout);

    // 系统信息显示
    systemInfoDisplay = new QTextEdit(this);
    systemInfoDisplay->setReadOnly(true);
    systemInfoDisplay->setFont(QFont("Consolas", 9));
    layout->addWidget(systemInfoDisplay);

    tabWidget->addTab(systemInfoWidget, "系统信息");

    connect(refreshSystemInfoBtn, &QPushButton::clicked, this, &MainWindow::onRefreshSystemInfoClicked);
}

void MainWindow::createHardwareConfigTab()
{
    QWidget *configWidget = new QWidget();
    QVBoxLayout *mainLayout = new QVBoxLayout(configWidget);

    // 创建滚动区域
    QScrollArea *scrollArea = new QScrollArea();
    QWidget *scrollWidget = new QWidget();
    QVBoxLayout *scrollLayout = new QVBoxLayout(scrollWidget);

    // 全局随机化按钮
    QHBoxLayout *globalButtonLayout = new QHBoxLayout();
    randomizeAllBtn = new QPushButton("一键随机化所有硬件", this);
    randomizeAllBtn->setStyleSheet("QPushButton { background-color: #4CAF50; color: white; font-weight: bold; padding: 8px; }");
    globalButtonLayout->addWidget(randomizeAllBtn);
    globalButtonLayout->addStretch();
    scrollLayout->addLayout(globalButtonLayout);

    // BIOS信息组
    QGroupBox *biosGroup = new QGroupBox("BIOS信息", this);
    QGridLayout *biosLayout = new QGridLayout(biosGroup);

    biosLayout->addWidget(new QLabel("厂商:"), 0, 0);
    biosVendor = new QLineEdit(this);
    biosLayout->addWidget(biosVendor, 0, 1);

    biosLayout->addWidget(new QLabel("版本:"), 1, 0);
    biosVersion = new QLineEdit(this);
    biosLayout->addWidget(biosVersion, 1, 1);

    biosLayout->addWidget(new QLabel("日期:"), 2, 0);
    biosDate = new QLineEdit(this);
    biosLayout->addWidget(biosDate, 2, 1);

    biosLayout->addWidget(new QLabel("序列号:"), 3, 0);
    biosSerial = new QLineEdit(this);
    biosLayout->addWidget(biosSerial, 3, 1);

    biosLayout->addWidget(new QLabel("系统UUID:"), 4, 0);
    systemUuid = new QLineEdit(this);
    biosLayout->addWidget(systemUuid, 4, 1);

    randomizeBiosBtn = new QPushButton("随机化BIOS", this);
    biosLayout->addWidget(randomizeBiosBtn, 5, 0, 1, 2);

    scrollLayout->addWidget(biosGroup);

    // 主板信息组
    QGroupBox *motherboardGroup = new QGroupBox("主板信息", this);
    QGridLayout *motherboardLayout = new QGridLayout(motherboardGroup);

    motherboardLayout->addWidget(new QLabel("制造商:"), 0, 0);
    motherboardManufacturer = new QLineEdit(this);
    motherboardLayout->addWidget(motherboardManufacturer, 0, 1);

    motherboardLayout->addWidget(new QLabel("产品:"), 1, 0);
    motherboardProduct = new QLineEdit(this);
    motherboardLayout->addWidget(motherboardProduct, 1, 1);

    motherboardLayout->addWidget(new QLabel("版本:"), 2, 0);
    motherboardVersion = new QLineEdit(this);
    motherboardLayout->addWidget(motherboardVersion, 2, 1);

    motherboardLayout->addWidget(new QLabel("序列号:"), 3, 0);
    motherboardSerial = new QLineEdit(this);
    motherboardLayout->addWidget(motherboardSerial, 3, 1);

    randomizeMotherboardBtn = new QPushButton("随机化主板", this);
    motherboardLayout->addWidget(randomizeMotherboardBtn, 4, 0, 1, 2);

    scrollLayout->addWidget(motherboardGroup);

    // CPU信息组
    QGroupBox *cpuGroup = new QGroupBox("处理器信息", this);
    QGridLayout *cpuLayout = new QGridLayout(cpuGroup);

    cpuLayout->addWidget(new QLabel("制造商:"), 0, 0);
    cpuManufacturer = new QLineEdit(this);
    cpuLayout->addWidget(cpuManufacturer, 0, 1);

    cpuLayout->addWidget(new QLabel("名称:"), 1, 0);
    cpuName = new QLineEdit(this);
    cpuLayout->addWidget(cpuName, 1, 1);

    cpuLayout->addWidget(new QLabel("CPU ID:"), 2, 0);
    cpuId = new QLineEdit(this);
    cpuLayout->addWidget(cpuId, 2, 1);

    cpuLayout->addWidget(new QLabel("序列号:"), 3, 0);
    cpuSerial = new QLineEdit(this);
    cpuLayout->addWidget(cpuSerial, 3, 1);

    cpuLayout->addWidget(new QLabel("核心数:"), 4, 0);
    cpuCores = new QSpinBox(this);
    cpuCores->setRange(1, 64);
    cpuLayout->addWidget(cpuCores, 4, 1);

    cpuLayout->addWidget(new QLabel("线程数:"), 5, 0);
    cpuThreads = new QSpinBox(this);
    cpuThreads->setRange(1, 128);
    cpuLayout->addWidget(cpuThreads, 5, 1);

    randomizeCpuBtn = new QPushButton("随机化CPU", this);
    cpuLayout->addWidget(randomizeCpuBtn, 6, 0, 1, 2);

    scrollLayout->addWidget(cpuGroup);

    // 内存信息组
    QGroupBox *memoryGroup = new QGroupBox("内存信息", this);
    QGridLayout *memoryLayout = new QGridLayout(memoryGroup);

    memoryLayout->addWidget(new QLabel("制造商:"), 0, 0);
    memoryManufacturer = new QLineEdit(this);
    memoryLayout->addWidget(memoryManufacturer, 0, 1);

    memoryLayout->addWidget(new QLabel("型号:"), 1, 0);
    memoryPartNumber = new QLineEdit(this);
    memoryLayout->addWidget(memoryPartNumber, 1, 1);

    memoryLayout->addWidget(new QLabel("序列号:"), 2, 0);
    memorySerial = new QLineEdit(this);
    memoryLayout->addWidget(memorySerial, 2, 1);

    memoryLayout->addWidget(new QLabel("总容量(GB):"), 3, 0);
    totalMemoryGB = new QSpinBox(this);
    totalMemoryGB->setRange(1, 256);
    memoryLayout->addWidget(totalMemoryGB, 3, 1);

    randomizeMemoryBtn = new QPushButton("随机化内存", this);
    memoryLayout->addWidget(randomizeMemoryBtn, 4, 0, 1, 2);

    scrollLayout->addWidget(memoryGroup);

    // 硬盘信息组
    QGroupBox *diskGroup = new QGroupBox("硬盘信息", this);
    QGridLayout *diskLayout = new QGridLayout(diskGroup);

    diskLayout->addWidget(new QLabel("型号:"), 0, 0);
    diskModel = new QLineEdit(this);
    diskLayout->addWidget(diskModel, 0, 1);

    diskLayout->addWidget(new QLabel("序列号:"), 1, 0);
    diskSerial = new QLineEdit(this);
    diskLayout->addWidget(diskSerial, 1, 1);

    diskLayout->addWidget(new QLabel("固件版本:"), 2, 0);
    diskFirmware = new QLineEdit(this);
    diskLayout->addWidget(diskFirmware, 2, 1);

    diskLayout->addWidget(new QLabel("容量:"), 3, 0);
    diskSize = new QLineEdit(this);
    diskLayout->addWidget(diskSize, 3, 1);

    randomizeDiskBtn = new QPushButton("随机化硬盘", this);
    diskLayout->addWidget(randomizeDiskBtn, 4, 0, 1, 2);

    scrollLayout->addWidget(diskGroup);

    // 显卡信息组
    QGroupBox *gpuGroup = new QGroupBox("显卡信息", this);
    QGridLayout *gpuLayout = new QGridLayout(gpuGroup);

    gpuLayout->addWidget(new QLabel("名称:"), 0, 0);
    gpuName = new QLineEdit(this);
    gpuLayout->addWidget(gpuName, 0, 1);

    gpuLayout->addWidget(new QLabel("制造商:"), 1, 0);
    gpuManufacturer = new QLineEdit(this);
    gpuLayout->addWidget(gpuManufacturer, 1, 1);

    gpuLayout->addWidget(new QLabel("驱动版本:"), 2, 0);
    gpuDriverVersion = new QLineEdit(this);
    gpuLayout->addWidget(gpuDriverVersion, 2, 1);

    randomizeGpuBtn = new QPushButton("随机化显卡", this);
    gpuLayout->addWidget(randomizeGpuBtn, 3, 0, 1, 2);

    scrollLayout->addWidget(gpuGroup);

    // 网卡信息组
    QGroupBox *nicGroup = new QGroupBox("网卡信息", this);
    QGridLayout *nicLayout = new QGridLayout(nicGroup);

    nicLayout->addWidget(new QLabel("名称:"), 0, 0);
    nicName = new QLineEdit(this);
    nicLayout->addWidget(nicName, 0, 1);

    nicLayout->addWidget(new QLabel("制造商:"), 1, 0);
    nicManufacturer = new QLineEdit(this);
    nicLayout->addWidget(nicManufacturer, 1, 1);

    nicLayout->addWidget(new QLabel("MAC地址:"), 2, 0);
    nicMacAddress = new QLineEdit(this);
    nicLayout->addWidget(nicMacAddress, 2, 1);

    randomizeNetworkBtn = new QPushButton("随机化网卡", this);
    nicLayout->addWidget(randomizeNetworkBtn, 3, 0, 1, 2);

    scrollLayout->addWidget(nicGroup);

    // 声卡信息组
    QGroupBox *audioGroup = new QGroupBox("声卡信息", this);
    QGridLayout *audioLayout = new QGridLayout(audioGroup);

    audioLayout->addWidget(new QLabel("设备名称:"), 0, 0);
    audioDeviceName = new QLineEdit(this);
    audioLayout->addWidget(audioDeviceName, 0, 1);

    audioLayout->addWidget(new QLabel("制造商:"), 1, 0);
    audioManufacturer = new QLineEdit(this);
    audioLayout->addWidget(audioManufacturer, 1, 1);

    randomizeAudioBtn = new QPushButton("随机化声卡", this);
    audioLayout->addWidget(randomizeAudioBtn, 2, 0, 1, 2);

    scrollLayout->addWidget(audioGroup);

    scrollArea->setWidget(scrollWidget);
    scrollArea->setWidgetResizable(true);
    mainLayout->addWidget(scrollArea);

    tabWidget->addTab(configWidget, "硬件配置");

    // 连接信号
    connect(randomizeAllBtn, &QPushButton::clicked, this, &MainWindow::onRandomizeAllClicked);
    connect(randomizeBiosBtn, &QPushButton::clicked, this, &MainWindow::onRandomizeBiosClicked);
    connect(randomizeMotherboardBtn, &QPushButton::clicked, this, &MainWindow::onRandomizeMotherboardClicked);
    connect(randomizeCpuBtn, &QPushButton::clicked, this, &MainWindow::onRandomizeCpuClicked);
    connect(randomizeMemoryBtn, &QPushButton::clicked, this, &MainWindow::onRandomizeMemoryClicked);
    connect(randomizeDiskBtn, &QPushButton::clicked, this, &MainWindow::onRandomizeDiskClicked);
    connect(randomizeGpuBtn, &QPushButton::clicked, this, &MainWindow::onRandomizeGpuClicked);
    connect(randomizeNetworkBtn, &QPushButton::clicked, this, &MainWindow::onRandomizeNetworkClicked);
    connect(randomizeAudioBtn, &QPushButton::clicked, this, &MainWindow::onRandomizeAudioClicked);
}

void MainWindow::createControlTab()
{
    QWidget *controlWidget = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout(controlWidget);

    // 目标程序配置组
    QGroupBox *programGroup = new QGroupBox("目标程序配置", this);
    QVBoxLayout *programLayout = new QVBoxLayout(programGroup);

    // 程序路径配置
    QHBoxLayout *pathLayout = new QHBoxLayout();
    QLabel *pathLabel = new QLabel("程序路径:", this);
    pathLabel->setMinimumWidth(80);
    targetProgramPath = new QLineEdit(this);
    targetProgramPath->setPlaceholderText("请选择要启动的目标程序...");
    browseTargetProgramBtn = new QPushButton("浏览...", this);
    browseTargetProgramBtn->setStyleSheet("QPushButton { background-color: #4CAF50; color: white; padding: 6px; }");
    browseTargetProgramBtn->setMaximumWidth(80);

    pathLayout->addWidget(pathLabel);
    pathLayout->addWidget(targetProgramPath);
    pathLayout->addWidget(browseTargetProgramBtn);
    programLayout->addLayout(pathLayout);

    // 程序参数配置
    QHBoxLayout *argsLayout = new QHBoxLayout();
    QLabel *argsLabel = new QLabel("启动参数:", this);
    argsLabel->setMinimumWidth(80);
    targetProgramArgs = new QLineEdit(this);
    targetProgramArgs->setPlaceholderText("可选的程序启动参数...");

    argsLayout->addWidget(argsLabel);
    argsLayout->addWidget(targetProgramArgs);
    programLayout->addLayout(argsLayout);

    // 状态显示
    targetProgramStatus = new QLabel("状态: 未配置目标程序", this);
    targetProgramStatus->setStyleSheet("QLabel { color: #666; font-weight: bold; padding: 5px; }");
    programLayout->addWidget(targetProgramStatus);

    layout->addWidget(programGroup);

    // Hook控制组
    QGroupBox *hookGroup = new QGroupBox("Hook控制", this);
    QVBoxLayout *hookLayout = new QVBoxLayout(hookGroup);

    QHBoxLayout *hookButtonLayout = new QHBoxLayout();
    startHookBtn = new QPushButton("开始Hook", this);
    startHookBtn->setStyleSheet("QPushButton { background-color: #2196F3; color: white; font-weight: bold; padding: 8px; }");
    stopHookBtn = new QPushButton("停止Hook", this);
    stopHookBtn->setStyleSheet("QPushButton { background-color: #f44336; color: white; font-weight: bold; padding: 8px; }");
    stopHookBtn->setEnabled(false);

    hookButtonLayout->addWidget(startHookBtn);
    hookButtonLayout->addWidget(stopHookBtn);
    hookButtonLayout->addStretch();
    hookLayout->addLayout(hookButtonLayout);

    layout->addWidget(hookGroup);

    // 配置管理组
    QGroupBox *configGroup = new QGroupBox("配置管理", this);
    QVBoxLayout *configLayout = new QVBoxLayout(configGroup);

    QHBoxLayout *configButtonLayout = new QHBoxLayout();
    saveConfigBtn = new QPushButton("保存配置", this);
    loadConfigBtn = new QPushButton("加载配置", this);

    configButtonLayout->addWidget(saveConfigBtn);
    configButtonLayout->addWidget(loadConfigBtn);
    configButtonLayout->addStretch();
    configLayout->addLayout(configButtonLayout);

    layout->addWidget(configGroup);

    // 日志显示
    QGroupBox *logGroup = new QGroupBox("运行日志", this);
    QVBoxLayout *logLayout = new QVBoxLayout(logGroup);
    logTextEdit = new QTextEdit(this);
    logTextEdit->setMaximumHeight(200);
    logTextEdit->setFont(QFont("Consolas", 9));
    logLayout->addWidget(logTextEdit);
    layout->addWidget(logGroup);

    layout->addStretch();

    tabWidget->addTab(controlWidget, "控制面板");

    // 连接信号
    connect(startHookBtn, &QPushButton::clicked, this, &MainWindow::onStartHookClicked);
    connect(stopHookBtn, &QPushButton::clicked, this, &MainWindow::onStopHookClicked);
    connect(saveConfigBtn, &QPushButton::clicked, this, &MainWindow::onSaveConfigClicked);
    connect(loadConfigBtn, &QPushButton::clicked, this, &MainWindow::onLoadConfigClicked);
    connect(browseTargetProgramBtn, &QPushButton::clicked, this, &MainWindow::onBrowseTargetProgramClicked);
    connect(targetProgramPath, &QLineEdit::textChanged, this, &MainWindow::onTargetProgramPathChanged);

    // 初始化目标程序配置
    loadTargetProgramSettings();
}

void MainWindow::updateSystemInfo()
{
    currentSystemInfo = SystemInfo::getCurrentSystemInfo();
    updateSystemInfoDisplay();
}

void MainWindow::updateSystemInfoDisplay()
{
    QString info;

    info += "=== 操作系统信息 ===\n";
    info += QString("操作系统: %1\n").arg(currentSystemInfo.osName);
    info += QString("版本: %1\n").arg(currentSystemInfo.osVersion);
    info += QString("构建号: %1\n").arg(currentSystemInfo.osBuild);
    info += QString("计算机名: %1\n").arg(currentSystemInfo.computerName);
    info += QString("产品ID: %1\n").arg(currentSystemInfo.productId);
    info += QString("安装日期: %1\n").arg(currentSystemInfo.installDate.toString());
    info += QString("开机时间: %1\n").arg(currentSystemInfo.bootTime.toString());
    info += "\n";

    info += "=== 网络信息 ===\n";
    info += QString("IP地址: %1\n").arg(currentSystemInfo.ipAddresses.join(", "));
    info += QString("MAC地址: %1\n").arg(currentSystemInfo.macAddresses.join(", "));
    info += "\n";

    info += "=== BIOS信息 ===\n";
    info += QString("厂商: %1\n").arg(currentSystemInfo.biosVendor);
    info += QString("版本: %1\n").arg(currentSystemInfo.biosVersion);
    info += QString("日期: %1\n").arg(currentSystemInfo.biosDate);
    info += QString("序列号: %1\n").arg(currentSystemInfo.biosSerial);
    info += QString("系统UUID: %1\n").arg(currentSystemInfo.systemUuid);
    info += "\n";

    info += "=== 主板信息 ===\n";
    info += QString("制造商: %1\n").arg(currentSystemInfo.motherboardManufacturer);
    info += QString("产品: %1\n").arg(currentSystemInfo.motherboardProduct);
    info += QString("版本: %1\n").arg(currentSystemInfo.motherboardVersion);
    info += QString("序列号: %1\n").arg(currentSystemInfo.motherboardSerial);
    info += "\n";

    info += "=== 处理器信息 ===\n";
    info += QString("制造商: %1\n").arg(currentSystemInfo.cpuManufacturer);
    info += QString("名称: %1\n").arg(currentSystemInfo.cpuName);
    info += QString("CPU ID: %1\n").arg(currentSystemInfo.cpuId);
    info += QString("核心数: %1\n").arg(currentSystemInfo.cpuCores);
    info += QString("线程数: %1\n").arg(currentSystemInfo.cpuThreads);
    info += "\n";

    info += "=== 内存信息 ===\n";
    info += QString("制造商: %1\n").arg(currentSystemInfo.memoryManufacturers.join(", "));
    info += QString("型号: %1\n").arg(currentSystemInfo.memoryPartNumbers.join(", "));
    info += QString("总容量: %1 GB\n").arg(currentSystemInfo.totalMemory / (1024*1024*1024));
    info += "\n";

    info += "=== 硬盘信息 ===\n";
    info += QString("型号: %1\n").arg(currentSystemInfo.diskModels.join(", "));
    info += QString("序列号: %1\n").arg(currentSystemInfo.diskSerials.join(", "));
    info += QString("固件版本: %1\n").arg(currentSystemInfo.diskFirmwares.join(", "));
    info += "\n";

    info += "=== 显卡信息 ===\n";
    info += QString("名称: %1\n").arg(currentSystemInfo.gpuNames.join(", "));
    info += QString("制造商: %1\n").arg(currentSystemInfo.gpuManufacturers.join(", "));
    info += QString("驱动版本: %1\n").arg(currentSystemInfo.gpuDriverVersions.join(", "));
    info += "\n";

    info += "=== 网卡信息 ===\n";
    info += QString("名称: %1\n").arg(currentSystemInfo.nicNames.join(", "));
    info += QString("制造商: %1\n").arg(currentSystemInfo.nicManufacturers.join(", "));
    info += QString("MAC地址: %1\n").arg(currentSystemInfo.nicMacAddresses.join(", "));
    info += "\n";

    info += "=== 声卡信息 ===\n";
    info += QString("设备名称: %1\n").arg(currentSystemInfo.audioDeviceNames.join(", "));
    info += QString("制造商: %1\n").arg(currentSystemInfo.audioManufacturers.join(", "));

    systemInfoDisplay->setPlainText(info);
}

void MainWindow::onRefreshSystemInfoClicked()
{
    updateSystemInfo();
    logTextEdit->append("系统信息已刷新");
}

void MainWindow::onRandomizeAllClicked()
{
    SystemInformation randomInfo = HardwareRandomizer::generateRandomSystemInfo();
    setHardwareConfigToUI(randomInfo);
    saveHardwareConfigSettings(); // 自动保存配置
    logTextEdit->append("已随机化所有硬件信息");
}

void MainWindow::onRandomizeBiosClicked()
{
    biosVendor->setText(HardwareRandomizer::generateRandomBiosVendor());
    biosVersion->setText(HardwareRandomizer::generateRandomBiosVersion());
    biosDate->setText(HardwareRandomizer::generateRandomBiosDate());
    biosSerial->setText(HardwareRandomizer::generateRandomSerial());
    systemUuid->setText(HardwareRandomizer::generateRandomUuid());
    saveHardwareConfigSettings(); // 自动保存配置
    logTextEdit->append("已随机化BIOS信息");
}

void MainWindow::onRandomizeMotherboardClicked()
{
    motherboardManufacturer->setText(HardwareRandomizer::generateRandomMotherboardManufacturer());
    motherboardProduct->setText(HardwareRandomizer::generateRandomMotherboardProduct());
    motherboardVersion->setText("1.0");
    motherboardSerial->setText(HardwareRandomizer::generateRandomSerial());
    saveHardwareConfigSettings(); // 自动保存配置
    logTextEdit->append("已随机化主板信息");
}

void MainWindow::onRandomizeCpuClicked()
{
    cpuManufacturer->setText(HardwareRandomizer::generateRandomCpuManufacturer());
    cpuName->setText(HardwareRandomizer::generateRandomCpuName());
    cpuId->setText(HardwareRandomizer::generateRandomCpuId());
    cpuSerial->setText(HardwareRandomizer::generateRandomSerial());
    cpuCores->setValue(QRandomGenerator::global()->bounded(2, 17));
    cpuThreads->setValue(cpuCores->value() * QRandomGenerator::global()->bounded(1, 3));
    saveHardwareConfigSettings(); // 自动保存配置
    logTextEdit->append("已随机化CPU信息");
}

void MainWindow::onRandomizeMemoryClicked()
{
    memoryManufacturer->setText(HardwareRandomizer::generateRandomMemoryManufacturer());
    memoryPartNumber->setText(HardwareRandomizer::generateRandomMemoryPartNumber());
    memorySerial->setText(HardwareRandomizer::generateRandomSerial());
    totalMemoryGB->setValue(QRandomGenerator::global()->bounded(4, 65));
    saveHardwareConfigSettings(); // 自动保存配置
    logTextEdit->append("已随机化内存信息");
}

void MainWindow::onRandomizeDiskClicked()
{
    diskModel->setText(HardwareRandomizer::generateRandomDiskModel());
    diskSerial->setText(HardwareRandomizer::generateRandomDiskSerial());
    diskFirmware->setText(HardwareRandomizer::generateRandomFirmwareVersion());
    quint64 size = (quint64)QRandomGenerator::global()->bounded(250, 4001) * 1000 * 1000 * 1000;
    diskSize->setText(QString::number(size));
    saveHardwareConfigSettings(); // 自动保存配置
    logTextEdit->append("已随机化硬盘信息");
}

void MainWindow::onRandomizeGpuClicked()
{
    gpuName->setText(HardwareRandomizer::generateRandomGpuName());
    gpuManufacturer->setText(HardwareRandomizer::generateRandomGpuManufacturer());
    gpuDriverVersion->setText(QString("%1.%2.%3.%4")
                             .arg(QRandomGenerator::global()->bounded(20, 32))
                             .arg(QRandomGenerator::global()->bounded(10, 100))
                             .arg(QRandomGenerator::global()->bounded(10, 100))
                             .arg(QRandomGenerator::global()->bounded(1000, 10000)));
    saveHardwareConfigSettings(); // 自动保存配置
    logTextEdit->append("已随机化显卡信息");
}

void MainWindow::onRandomizeNetworkClicked()
{
    nicName->setText(HardwareRandomizer::generateRandomNetworkModel());
    nicManufacturer->setText(HardwareRandomizer::generateRandomNetworkManufacturer());
    nicMacAddress->setText(HardwareRandomizer::generateRandomMacAddress());
    saveHardwareConfigSettings(); // 自动保存配置
    logTextEdit->append("已随机化网卡信息");
}

void MainWindow::onRandomizeAudioClicked()
{
    audioDeviceName->setText(HardwareRandomizer::generateRandomAudioDevice());
    audioManufacturer->setText(HardwareRandomizer::generateRandomAudioManufacturer());
    saveHardwareConfigSettings(); // 自动保存配置
    logTextEdit->append("已随机化声卡信息");
}

void MainWindow::setHardwareConfigToUI(const SystemInformation& info)
{
    // BIOS信息
    biosVendor->setText(info.biosVendor);
    biosVersion->setText(info.biosVersion);
    biosDate->setText(info.biosDate);
    biosSerial->setText(info.biosSerial);
    systemUuid->setText(info.systemUuid);

    // 主板信息
    motherboardManufacturer->setText(info.motherboardManufacturer);
    motherboardProduct->setText(info.motherboardProduct);
    motherboardVersion->setText(info.motherboardVersion);
    motherboardSerial->setText(info.motherboardSerial);

    // CPU信息
    cpuManufacturer->setText(info.cpuManufacturer);
    cpuName->setText(info.cpuName);
    cpuId->setText(info.cpuId);
    cpuSerial->setText(info.cpuSerial);
    cpuCores->setValue(info.cpuCores);
    cpuThreads->setValue(info.cpuThreads);

    // 内存信息
    if (!info.memoryManufacturers.isEmpty()) {
        memoryManufacturer->setText(info.memoryManufacturers.first());
    }
    if (!info.memoryPartNumbers.isEmpty()) {
        memoryPartNumber->setText(info.memoryPartNumbers.first());
    }
    if (!info.memorySerials.isEmpty()) {
        memorySerial->setText(info.memorySerials.first());
    }
    totalMemoryGB->setValue(info.totalMemory / (1024*1024*1024));

    // 硬盘信息
    if (!info.diskModels.isEmpty()) {
        diskModel->setText(info.diskModels.first());
    }
    if (!info.diskSerials.isEmpty()) {
        diskSerial->setText(info.diskSerials.first());
    }
    if (!info.diskFirmwares.isEmpty()) {
        diskFirmware->setText(info.diskFirmwares.first());
    }
    if (!info.diskSizes.isEmpty()) {
        diskSize->setText(info.diskSizes.first());
    }

    // 显卡信息
    if (!info.gpuNames.isEmpty()) {
        gpuName->setText(info.gpuNames.first());
    }
    if (!info.gpuManufacturers.isEmpty()) {
        gpuManufacturer->setText(info.gpuManufacturers.first());
    }
    if (!info.gpuDriverVersions.isEmpty()) {
        gpuDriverVersion->setText(info.gpuDriverVersions.first());
    }

    // 网卡信息
    if (!info.nicNames.isEmpty()) {
        nicName->setText(info.nicNames.first());
    }
    if (!info.nicManufacturers.isEmpty()) {
        nicManufacturer->setText(info.nicManufacturers.first());
    }
    if (!info.nicMacAddresses.isEmpty()) {
        nicMacAddress->setText(info.nicMacAddresses.first());
    }

    // 声卡信息
    if (!info.audioDeviceNames.isEmpty()) {
        audioDeviceName->setText(info.audioDeviceNames.first());
    }
    if (!info.audioManufacturers.isEmpty()) {
        audioManufacturer->setText(info.audioManufacturers.first());
    }
}

SystemInformation MainWindow::getHardwareConfigFromUI()
{
    SystemInformation info;

    // BIOS信息
    info.biosVendor = biosVendor->text();
    info.biosVersion = biosVersion->text();
    info.biosDate = biosDate->text();
    info.biosSerial = biosSerial->text();
    info.systemUuid = systemUuid->text();

    // 主板信息
    info.motherboardManufacturer = motherboardManufacturer->text();
    info.motherboardProduct = motherboardProduct->text();
    info.motherboardVersion = motherboardVersion->text();
    info.motherboardSerial = motherboardSerial->text();

    // CPU信息
    info.cpuManufacturer = cpuManufacturer->text();
    info.cpuName = cpuName->text();
    info.cpuId = cpuId->text();
    info.cpuSerial = cpuSerial->text();
    info.cpuCores = cpuCores->value();
    info.cpuThreads = cpuThreads->value();

    // 内存信息
    info.memoryManufacturers.append(memoryManufacturer->text());
    info.memoryPartNumbers.append(memoryPartNumber->text());
    info.memorySerials.append(memorySerial->text());
    info.totalMemory = (quint64)totalMemoryGB->value() * 1024 * 1024 * 1024;

    // 硬盘信息
    info.diskModels.append(diskModel->text());
    info.diskSerials.append(diskSerial->text());
    info.diskFirmwares.append(diskFirmware->text());
    info.diskSizes.append(diskSize->text());

    // 显卡信息
    info.gpuNames.append(gpuName->text());
    info.gpuManufacturers.append(gpuManufacturer->text());
    info.gpuDriverVersions.append(gpuDriverVersion->text());

    // 网卡信息
    info.nicNames.append(nicName->text());
    info.nicManufacturers.append(nicManufacturer->text());
    info.nicMacAddresses.append(nicMacAddress->text());

    // 声卡信息
    info.audioDeviceNames.append(audioDeviceName->text());
    info.audioManufacturers.append(audioManufacturer->text());

    return info;
}

DWORD FindProcessByName(const QString &processName)
{
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE) {
        return 0;
    }

    PROCESSENTRY32W pe32;
    pe32.dwSize = sizeof(PROCESSENTRY32W);

    if (Process32FirstW(hSnapshot, &pe32)) {
        do {
            QString currentProcessName = QString::fromWCharArray(pe32.szExeFile);
            if (currentProcessName.compare(processName, Qt::CaseInsensitive) == 0) {
                CloseHandle(hSnapshot);
                return pe32.th32ProcessID;
            }
        } while (Process32NextW(hSnapshot, &pe32));
    }

    CloseHandle(hSnapshot);
    return 0;
}

void MainWindow::onStartHookClicked()
{
    // 检查目标程序路径
    QString programPath = targetProgramPath->text().trimmed();
    if (programPath.isEmpty()) {
        logTextEdit->append("错误: 未配置目标程序路径");
        QMessageBox::warning(this, "错误", "请先配置要启动的目标程序路径");
        return;
    }

    // 检查程序文件是否存在
    if (!QFile::exists(programPath)) {
        logTextEdit->append(QString("错误: 目标程序文件不存在: %1").arg(programPath));
        QMessageBox::warning(this, "错误", "目标程序文件不存在，请检查路径");
        return;
    }

    logTextEdit->append(QString("准备启动目标程序: %1").arg(programPath));

    // 保存当前硬件配置设置（自动保存功能）
    saveHardwareConfigSettings();
    logTextEdit->append("当前硬件配置已自动保存");

    // 保存当前配置到临时文件
    QJsonObject config = getHardwareConfig();
    QJsonDocument doc(config);

    QFile configFile("hardware_config.json");
    if (configFile.open(QIODevice::WriteOnly)) {
        configFile.write(doc.toJson());
        configFile.close();
        logTextEdit->append("硬件配置已保存到 hardware_config.json");
    }

    // 创建暂停的目标进程
    QString arguments = targetProgramArgs->text().trimmed();
    if (createSuspendedProcess(programPath, arguments)) {
        logTextEdit->append(QString("成功创建暂停进程 PID: %1").arg(targetProcessId));

        // 注入DLL
        if (injectDLL()) {
            logTextEdit->append("成功注入Hook DLL到目标进程");

            // 恢复进程执行
            if (resumeTargetProcess()) {
                logTextEdit->append("目标进程已恢复执行，Hook生效");
                startHookBtn->setEnabled(false);
                stopHookBtn->setEnabled(true);
                targetProgramStatus->setText("状态: 目标程序运行中 (已Hook)");
                targetProgramStatus->setStyleSheet("QLabel { color: #4CAF50; font-weight: bold; padding: 5px; }");
            } else {
                logTextEdit->append("警告: 进程恢复失败");
            }
        } else {
            logTextEdit->append("错误: DLL注入失败，终止目标进程");
            if (hProcess) {
                TerminateProcess(hProcess, 1);
                CloseHandle(hProcess);
                hProcess = nullptr;
            }
            QMessageBox::critical(this, "错误", "DLL注入失败");
        }
    } else {
        logTextEdit->append("错误: 无法创建目标进程");
        QMessageBox::critical(this, "错误", "无法创建目标进程");
    }
}

void MainWindow::onStopHookClicked()
{
    if (ejectDLL()) {
        logTextEdit->append("成功从目标进程中移除Hook DLL");
        startHookBtn->setEnabled(true);
        stopHookBtn->setEnabled(false);
        targetProgramStatus->setText("状态: 目标程序已停止");
        targetProgramStatus->setStyleSheet("QLabel { color: #666; font-weight: bold; padding: 5px; }");
    } else {
        logTextEdit->append("警告: DLL移除可能失败");
    }
}

void MainWindow::onSaveConfigClicked()
{
    QString fileName = QFileDialog::getSaveFileName(this, "保存硬件配置", "", "JSON文件 (*.json)");
    if (!fileName.isEmpty()) {
        SystemInformation info = getHardwareConfigFromUI();
        QJsonObject config = SystemInfo::systemInfoToJson(info);
        QJsonDocument doc(config);

        QFile file(fileName);
        if (file.open(QIODevice::WriteOnly)) {
            file.write(doc.toJson());
            file.close();
            logTextEdit->append(QString("配置已保存到: %1").arg(fileName));
        } else {
            logTextEdit->append("错误: 无法保存配置文件");
        }
    }
}

void MainWindow::onLoadConfigClicked()
{
    QString fileName = QFileDialog::getOpenFileName(this, "加载硬件配置", "", "JSON文件 (*.json)");
    if (!fileName.isEmpty()) {
        QFile file(fileName);
        if (file.open(QIODevice::ReadOnly)) {
            QByteArray fileData = file.readAll();
            file.close();

            QJsonDocument doc = QJsonDocument::fromJson(fileData);
            SystemInformation info = SystemInfo::systemInfoFromJson(doc.object());
            setHardwareConfigToUI(info);
            saveHardwareConfigSettings(); // 自动保存到设置
            logTextEdit->append(QString("配置已从文件加载: %1").arg(fileName));
        } else {
            logTextEdit->append("错误: 无法读取配置文件");
        }
    }
}

QJsonObject MainWindow::getHardwareConfig()
{
    SystemInformation info = getHardwareConfigFromUI();
    return SystemInfo::systemInfoToJson(info);
}

bool MainWindow::injectDLL()
{
    hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, targetProcessId);
    if (!hProcess) {
        logTextEdit->append(QString("错误: 无法打开目标进程 (错误代码: %1)").arg(GetLastError()));
        return false;
    }

    // DLL路径
    QString dllPath = QApplication::applicationDirPath() + "/HookDLL.dll";
    QByteArray dllPathBytes = dllPath.toLocal8Bit();

    // 检查DLL文件是否存在
    if (!QFile::exists(dllPath)) {
        logTextEdit->append(QString("错误: 找不到DLL文件: %1").arg(dllPath));
        CloseHandle(hProcess);
        return false;
    }

    logTextEdit->append(QString("正在注入DLL: %1").arg(dllPath));

    // 在目标进程中分配内存
    LPVOID pRemoteMemory = VirtualAllocEx(hProcess, NULL, dllPathBytes.size() + 1,
                                         MEM_COMMIT, PAGE_READWRITE);
    if (!pRemoteMemory) {
        logTextEdit->append(QString("错误: 无法在目标进程中分配内存 (错误代码: %1)").arg(GetLastError()));
        CloseHandle(hProcess);
        return false;
    }

    // 写入DLL路径
    if (!WriteProcessMemory(hProcess, pRemoteMemory, dllPathBytes.data(),
                           dllPathBytes.size() + 1, NULL)) {
        logTextEdit->append(QString("错误: 无法写入内存 (错误代码: %1)").arg(GetLastError()));
        VirtualFreeEx(hProcess, pRemoteMemory, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        return false;
    }

    // 获取LoadLibraryA地址
    HMODULE hKernel32 = GetModuleHandleA("kernel32.dll");
    LPVOID pLoadLibrary = GetProcAddress(hKernel32, "LoadLibraryA");

    if (!pLoadLibrary) {
        logTextEdit->append("错误: 无法获取LoadLibraryA地址");
        VirtualFreeEx(hProcess, pRemoteMemory, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        return false;
    }

    // 创建远程线程
    HANDLE hRemoteThread = CreateRemoteThread(hProcess, NULL, 0,
                                            (LPTHREAD_START_ROUTINE)pLoadLibrary,
                                            pRemoteMemory, 0, NULL);

    if (hRemoteThread) {
        logTextEdit->append("远程线程已创建，等待DLL加载...");
        DWORD waitResult = WaitForSingleObject(hRemoteThread, 10000); // 10秒超时

        if (waitResult == WAIT_TIMEOUT) {
            logTextEdit->append("警告: DLL注入超时");
        } else if (waitResult == WAIT_OBJECT_0) {
            // 获取线程退出代码（LoadLibrary的返回值）
            DWORD exitCode;
            if (GetExitCodeThread(hRemoteThread, &exitCode)) {
                if (exitCode != 0) {
                    logTextEdit->append("DLL加载成功，模块句柄: 0x" + QString::number(exitCode, 16));

                    // 测试DLL功能
                    QTimer::singleShot(1000, this, [this]() {
                        testHookFunctionality();
                    });
                } else {
                    logTextEdit->append("错误: DLL加载失败");
                    CloseHandle(hRemoteThread);
                    VirtualFreeEx(hProcess, pRemoteMemory, 0, MEM_RELEASE);
                    CloseHandle(hProcess);
                    return false;
                }
            }
        }

        CloseHandle(hRemoteThread);
        VirtualFreeEx(hProcess, pRemoteMemory, 0, MEM_RELEASE);
        return true;
    } else {
        logTextEdit->append(QString("错误: 无法创建远程线程 (错误代码: %1)").arg(GetLastError()));
        VirtualFreeEx(hProcess, pRemoteMemory, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        return false;
    }
}

void MainWindow::testHookFunctionality()
{
    logTextEdit->append("正在测试Hook功能...");

    // 测试WMI Hook
    logTextEdit->append("--- 测试WMI Hook ---");
    QString wmiTestCommand = QString("powershell -Command \"Get-WmiObject -Class Win32_Processor | Select-Object Name, Manufacturer\"");

    QProcess wmiTestProcess;
    wmiTestProcess.start(wmiTestCommand, QStringList());
    wmiTestProcess.waitForFinished(5000);

    QString wmiOutput = wmiTestProcess.readAllStandardOutput();
    if (!wmiOutput.isEmpty()) {
        logTextEdit->append("WMI测试查询结果:");
        logTextEdit->append(wmiOutput);
    }

    // 测试注册表Hook
    logTextEdit->append("--- 测试注册表Hook ---");
    QString registryTestPath = QApplication::applicationDirPath() + "/TestRegistry.exe";
    if (QFile::exists(registryTestPath)) {
        QProcess registryTestProcess;
        registryTestProcess.start(registryTestPath, QStringList());
        registryTestProcess.waitForFinished(10000);

        QString registryOutput = registryTestProcess.readAllStandardOutput();
        if (!registryOutput.isEmpty()) {
            logTextEdit->append("注册表测试结果:");
            QStringList lines = registryOutput.split('\n');
            // 只显示关键信息
            for (const QString& line : lines) {
                if (line.contains("BIOS") || line.contains("CPU") || line.contains("System") ||
                    line.contains("Data:") || line.contains("===")) {
                    logTextEdit->append(line.trimmed());
                }
            }
        }
    } else {
        logTextEdit->append("注册表测试程序未找到: " + registryTestPath);
    }

    // 测试PowerShell注册表查询
    logTextEdit->append("--- 测试PowerShell注册表查询 ---");
    QString regTestCommand = QString("powershell -Command \"Get-ItemProperty -Path 'HKLM:\\HARDWARE\\DESCRIPTION\\System\\BIOS' -Name 'BIOSVendor', 'BIOSVersion', 'SystemManufacturer' | Format-List\"");

    QProcess regTestProcess;
    regTestProcess.start(regTestCommand, QStringList());
    regTestProcess.waitForFinished(5000);

    QString regOutput = regTestProcess.readAllStandardOutput();
    if (!regOutput.isEmpty()) {
        logTextEdit->append("PowerShell注册表查询结果:");
        logTextEdit->append(regOutput);
    }

    // 检查Hook日志文件
    QFile logFile("hook_log.txt");
    if (logFile.exists() && logFile.open(QIODevice::ReadOnly)) {
        QString hookLog = logFile.readAll();
        if (!hookLog.isEmpty()) {
            logTextEdit->append("--- Hook操作日志 ---");
            QStringList lines = hookLog.split('\n');
            // 只显示最后10行
            int startLine = qMax(0, lines.size() - 10);
            for (int i = startLine; i < lines.size(); i++) {
                if (!lines[i].trimmed().isEmpty()) {
                    logTextEdit->append(lines[i]);
                }
            }
        }
        logFile.close();
    }
}

bool MainWindow::ejectDLL()
{
    if (!hProcess) {
        return false;
    }

    // 这里应该实现DLL卸载逻辑
    // 由于Detours的限制，DLL卸载比较复杂
    // 简单起见，这里只是关闭进程句柄
    CloseHandle(hProcess);
    hProcess = nullptr;
    return true;
}

// 新增函数实现
void MainWindow::onBrowseTargetProgramClicked()
{
    QString fileName = QFileDialog::getOpenFileName(this,
        "选择目标程序",
        getDefaultVSCodePath(),
        "可执行文件 (*.exe);;所有文件 (*.*)");

    if (!fileName.isEmpty()) {
        targetProgramPath->setText(fileName);
        saveTargetProgramSettings();
        logTextEdit->append(QString("已选择目标程序: %1").arg(fileName));
    }
}

void MainWindow::onTargetProgramPathChanged()
{
    QString programPath = targetProgramPath->text().trimmed();

    if (programPath.isEmpty()) {
        targetProgramStatus->setText("状态: 未配置目标程序");
        targetProgramStatus->setStyleSheet("QLabel { color: #666; font-weight: bold; padding: 5px; }");
        startHookBtn->setEnabled(false);
    } else if (QFile::exists(programPath)) {
        targetProgramStatus->setText("状态: 目标程序已配置");
        targetProgramStatus->setStyleSheet("QLabel { color: #2196F3; font-weight: bold; padding: 5px; }");
        startHookBtn->setEnabled(!stopHookBtn->isEnabled());
    } else {
        targetProgramStatus->setText("状态: 程序文件不存在");
        targetProgramStatus->setStyleSheet("QLabel { color: #f44336; font-weight: bold; padding: 5px; }");
        startHookBtn->setEnabled(false);
    }

    // 保存设置
    saveTargetProgramSettings();
}

bool MainWindow::createSuspendedProcess(const QString& programPath, const QString& arguments)
{
    STARTUPINFOW si;
    PROCESS_INFORMATION pi;

    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));

    // 构建命令行
    QString commandLine = QString("\"%1\"").arg(programPath);
    if (!arguments.isEmpty()) {
        commandLine += " " + arguments;
    }

    std::wstring wCommandLine = commandLine.toStdWString();

    // 创建暂停的进程
    BOOL result = CreateProcessW(
        NULL,                           // 应用程序名称
        &wCommandLine[0],              // 命令行
        NULL,                          // 进程安全属性
        NULL,                          // 线程安全属性
        FALSE,                         // 继承句柄
        CREATE_SUSPENDED,              // 创建标志 - 暂停状态
        NULL,                          // 环境变量
        NULL,                          // 当前目录
        &si,                           // 启动信息
        &pi                            // 进程信息
    );

    if (result) {
        targetProcessId = pi.dwProcessId;
        hProcess = pi.hProcess;
        hTargetThread = pi.hThread;

        logTextEdit->append(QString("成功创建暂停进程，PID: %1, TID: %2")
                           .arg(pi.dwProcessId).arg(pi.dwThreadId));
        return true;
    } else {
        DWORD error = GetLastError();
        logTextEdit->append(QString("创建进程失败，错误代码: %1").arg(error));
        return false;
    }
}

bool MainWindow::resumeTargetProcess()
{
    if (hTargetThread) {
        DWORD result = ResumeThread(hTargetThread);
        if (result != (DWORD)-1) {
            logTextEdit->append("目标进程主线程已恢复执行");
            CloseHandle(hTargetThread);
            hTargetThread = nullptr;
            return true;
        } else {
            DWORD error = GetLastError();
            logTextEdit->append(QString("恢复线程失败，错误代码: %1").arg(error));
            return false;
        }
    }
    return false;
}

void MainWindow::saveTargetProgramSettings()
{
    QSettings settings("FakeInfo", "TargetProgram");
    settings.setValue("programPath", targetProgramPath->text());
    settings.setValue("programArgs", targetProgramArgs->text());
}

void MainWindow::loadTargetProgramSettings()
{
    QSettings settings("FakeInfo", "TargetProgram");

    // 加载保存的程序路径，如果没有则使用默认VSCode路径
    QString savedPath = settings.value("programPath", getDefaultVSCodePath()).toString();
    QString savedArgs = settings.value("programArgs", "").toString();

    targetProgramPath->setText(savedPath);
    targetProgramArgs->setText(savedArgs);

    // 触发路径变化事件以更新状态
    onTargetProgramPathChanged();

    logTextEdit->append("已加载目标程序配置");
}

QString MainWindow::getDefaultVSCodePath()
{
    // 常见的VSCode安装路径
    QStringList possiblePaths = {
        "C:/Users/" + qgetenv("USERNAME") + "/AppData/Local/Programs/Microsoft VS Code/Code.exe",
        "C:/Program Files/Microsoft VS Code/Code.exe",
        "C:/Program Files (x86)/Microsoft VS Code/Code.exe"
    };

    // 查找第一个存在的路径
    for (const QString& path : possiblePaths) {
        if (QFile::exists(path)) {
            return path;
        }
    }

    // 如果都不存在，返回最常见的路径作为默认值
    return possiblePaths.first();
}

void MainWindow::saveHardwareConfigSettings()
{
    QSettings settings("FakeInfo", "HardwareConfig");

    // 保存BIOS信息
    settings.setValue("bios/vendor", biosVendor->text());
    settings.setValue("bios/version", biosVersion->text());
    settings.setValue("bios/date", biosDate->text());
    settings.setValue("bios/serial", biosSerial->text());
    settings.setValue("bios/systemUuid", systemUuid->text());

    // 保存主板信息
    settings.setValue("motherboard/manufacturer", motherboardManufacturer->text());
    settings.setValue("motherboard/product", motherboardProduct->text());
    settings.setValue("motherboard/version", motherboardVersion->text());
    settings.setValue("motherboard/serial", motherboardSerial->text());

    // 保存CPU信息
    settings.setValue("cpu/manufacturer", cpuManufacturer->text());
    settings.setValue("cpu/name", cpuName->text());
    settings.setValue("cpu/id", cpuId->text());
    settings.setValue("cpu/serial", cpuSerial->text());
    settings.setValue("cpu/cores", cpuCores->value());
    settings.setValue("cpu/threads", cpuThreads->value());

    // 保存内存信息
    settings.setValue("memory/manufacturer", memoryManufacturer->text());
    settings.setValue("memory/partNumber", memoryPartNumber->text());
    settings.setValue("memory/serial", memorySerial->text());
    settings.setValue("memory/totalGB", totalMemoryGB->value());

    // 保存硬盘信息
    settings.setValue("disk/model", diskModel->text());
    settings.setValue("disk/serial", diskSerial->text());
    settings.setValue("disk/firmware", diskFirmware->text());
    settings.setValue("disk/size", diskSize->text());

    // 保存显卡信息
    settings.setValue("gpu/name", gpuName->text());
    settings.setValue("gpu/manufacturer", gpuManufacturer->text());
    settings.setValue("gpu/driverVersion", gpuDriverVersion->text());

    // 保存网卡信息
    settings.setValue("network/name", nicName->text());
    settings.setValue("network/manufacturer", nicManufacturer->text());
    settings.setValue("network/macAddress", nicMacAddress->text());

    // 保存声卡信息
    settings.setValue("audio/deviceName", audioDeviceName->text());
    settings.setValue("audio/manufacturer", audioManufacturer->text());

    // 同步设置到磁盘
    settings.sync();
}

void MainWindow::loadHardwareConfigSettings()
{
    QSettings settings("FakeInfo", "HardwareConfig");

    // 加载BIOS信息
    biosVendor->setText(settings.value("bios/vendor", "").toString());
    biosVersion->setText(settings.value("bios/version", "").toString());
    biosDate->setText(settings.value("bios/date", "").toString());
    biosSerial->setText(settings.value("bios/serial", "").toString());
    systemUuid->setText(settings.value("bios/systemUuid", "").toString());

    // 加载主板信息
    motherboardManufacturer->setText(settings.value("motherboard/manufacturer", "").toString());
    motherboardProduct->setText(settings.value("motherboard/product", "").toString());
    motherboardVersion->setText(settings.value("motherboard/version", "").toString());
    motherboardSerial->setText(settings.value("motherboard/serial", "").toString());

    // 加载CPU信息
    cpuManufacturer->setText(settings.value("cpu/manufacturer", "").toString());
    cpuName->setText(settings.value("cpu/name", "").toString());
    cpuId->setText(settings.value("cpu/id", "").toString());
    cpuSerial->setText(settings.value("cpu/serial", "").toString());
    cpuCores->setValue(settings.value("cpu/cores", 8).toInt());
    cpuThreads->setValue(settings.value("cpu/threads", 16).toInt());

    // 加载内存信息
    memoryManufacturer->setText(settings.value("memory/manufacturer", "").toString());
    memoryPartNumber->setText(settings.value("memory/partNumber", "").toString());
    memorySerial->setText(settings.value("memory/serial", "").toString());
    totalMemoryGB->setValue(settings.value("memory/totalGB", 16).toInt());

    // 加载硬盘信息
    diskModel->setText(settings.value("disk/model", "").toString());
    diskSerial->setText(settings.value("disk/serial", "").toString());
    diskFirmware->setText(settings.value("disk/firmware", "").toString());
    diskSize->setText(settings.value("disk/size", "").toString());

    // 加载显卡信息
    gpuName->setText(settings.value("gpu/name", "").toString());
    gpuManufacturer->setText(settings.value("gpu/manufacturer", "").toString());
    gpuDriverVersion->setText(settings.value("gpu/driverVersion", "").toString());

    // 加载网卡信息
    nicName->setText(settings.value("network/name", "").toString());
    nicManufacturer->setText(settings.value("network/manufacturer", "").toString());
    nicMacAddress->setText(settings.value("network/macAddress", "").toString());

    // 加载声卡信息
    audioDeviceName->setText(settings.value("audio/deviceName", "").toString());
    audioManufacturer->setText(settings.value("audio/manufacturer", "").toString());

    // 如果是第一次运行（所有字段都为空），则显示提示信息
    if (biosVendor->text().isEmpty() && cpuName->text().isEmpty() &&
        motherboardManufacturer->text().isEmpty()) {
        logTextEdit->append("首次运行，建议点击\"一键随机化所有硬件\"来生成初始配置");
    } else {
        logTextEdit->append("已加载上次保存的硬件配置");
    }
}






