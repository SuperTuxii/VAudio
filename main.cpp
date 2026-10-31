#include <QApplication>
#include "mainwindow.h"

int main(int argc, char *argv[]) {
    auto* pipewire = new PipewireAPI();
    QApplication a(argc, argv);
    MainWindow mainWindow(pipewire);
    pipewire->run(argc, argv);
    mainWindow.show();
    return QApplication::exec();
}
