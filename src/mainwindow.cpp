#include "mainwindow.h"


MainWindow::MainWindow()
{
    modelingWidget = new ModelingWidget(this);
    setCentralWidget(modelingWidget);
}

