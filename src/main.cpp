#include <QApplication>
#include <QDesktopWidget>
#include "mainwindow.h"
#include "meshlite.h"

int main(int argc, char ** argv)
{
    /*
    void *lite = meshlite_create_context();
    int first = meshlite_import(lite, "/Users/jeremy/cube.obj");
    int second = meshlite_import(lite, "/Users/jeremy/ball.obj");
    meshlite_scale(lite, first, 0.65);
    int result = meshlite_intersect(lite, first, second);
    meshlite_export(lite, result, "/Users/jeremy/testlib.obj");
    */
    QApplication app(argc, argv);
    QCoreApplication::setApplicationName("Dust 3D");
    MainWindow mainWindow;
    mainWindow.resize(mainWindow.sizeHint());
    int desktopArea = QApplication::desktop()->width() *
                     QApplication::desktop()->height();
    int widgetArea = mainWindow.width() * mainWindow.height();
    if (((float)widgetArea / (float)desktopArea) < 0.75f)
        mainWindow.show();
    else
        mainWindow.showMaximized();
    return app.exec();
}
