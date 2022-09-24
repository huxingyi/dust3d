#include <QApplication>
#include <QSurfaceFormat>
#include <QDebug>
#include <iostream>
#include <cstdio>
#include "document_window.h"
#include "theme.h"
#include "version.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    
    QSurfaceFormat format = QSurfaceFormat::defaultFormat();
    format.setProfile(QSurfaceFormat::OpenGLContextProfile::CoreProfile);
    format.setVersion(3, 3);
    QSurfaceFormat::setDefaultFormat(format);
    
    Theme::initialize();
    
    QCoreApplication::setApplicationName(APP_NAME);
    QCoreApplication::setOrganizationName(APP_COMPANY);
    QCoreApplication::setOrganizationDomain(APP_HOMEPAGE_URL);
    
    //freopen("dust3d.log", "w", stdout);
    //setvbuf(stdout, 0, _IONBF, 0);
    
    DocumentWindow::createDocumentWindow();
    
    return app.exec();
}
