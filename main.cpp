#include <QtWidgets/QApplication>
#include "MainWindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    
    // 设置应用程序信息
    app.setApplicationName("FakeInfo Hardware Spoofer");
    app.setApplicationVersion("1.0");
    app.setOrganizationName("FakeInfo");
    
    MainWindow window;
    window.show();
    
    return app.exec();
}
