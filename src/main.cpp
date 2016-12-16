#include <QApplication>
#include <stdio.h>
#include <assert.h>
#include "mainwindow.h"

int main(int argc, char *argv[])
{
  QApplication a(argc, argv);
 	MainWindow w;

  assert(0 != freopen((a.applicationDirPath() + 
    "/dust3d.log").toUtf8(), "w", stdout));
  assert(0 != freopen((a.applicationDirPath() + 
    "/dust3d_error.log").toUtf8(), "w", stderr));

  w.show();

  return a.exec();
}
