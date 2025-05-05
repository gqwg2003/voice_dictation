#include <QApplication>
#include "mainwindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("Голосовая диктовка");
    app.setApplicationVersion("0.1");
    
    MainWindow mainWindow;
    mainWindow.show();
    
    return app.exec();
} 