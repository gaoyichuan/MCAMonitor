#include "main.h"
#include "mainwindow.h"
#include <QApplication>
#include <QSettings>
#include <iostream>

int main(int argc, char *argv[]) {
    QApplication a(argc, argv);
    a.setOrganizationDomain("gycis.me");
    a.setOrganizationName("gaoyichuan");
    a.setApplicationName("MCA Monitor");
    a.setKeyboardInputInterval(100);
    a.setQuitOnLastWindowClosed(true);

    MainWindow w;
    w.show();

    return a.exec();
}
