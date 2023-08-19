#include "document.h"
#include "document_window.h"
#include "theme.h"
#include "version.h"
#include <QApplication>
#include <QDebug>
#include <QSurfaceFormat>
#include <cstdio>
#include <dust3d/base/string.h>
#include <iostream>

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);

    QSurfaceFormat format;
    format.setDepthBufferSize(24);
    format.setAlphaBufferSize(8);
    format.setSamples(4);
    QSurfaceFormat::setDefaultFormat(format);

    //QSurfaceFormat format = QSurfaceFormat::defaultFormat();
    //format.setProfile(QSurfaceFormat::OpenGLContextProfile::CoreProfile);
    //format.setVersion(3, 3);
    //QSurfaceFormat::setDefaultFormat(format);

    Theme::initialize();

    QCoreApplication::setApplicationName(APP_NAME);
    QCoreApplication::setOrganizationName(APP_COMPANY);
    QCoreApplication::setOrganizationDomain(APP_HOMEPAGE_URL);

    //freopen("dust3d.log", "w", stdout);
    //setvbuf(stdout, 0, _IONBF, 0);

    DocumentWindow* firstWindow = DocumentWindow::createDocumentWindow();

    QStringList openFileList;
    QStringList waitingExportList;
    bool toggleColor = false;
    for (int i = 1; i < argc; ++i) {
        if ('-' == argv[i][0]) {
            if (0 == strcmp(argv[i], "-output") || 0 == strcmp(argv[i], "-o")) {
                ++i;
                if (i < argc)
                    waitingExportList.append(argv[i]);
                continue;
            } else if (0 == strcmp(argv[i], "-toggle-color")) {
                ++i;
                if (i < argc)
                    toggleColor = dust3d::String::isTrue(argv[i]);
                continue;
            }
            qDebug() << "Unknown option:" << argv[i];
            continue;
        }
        QString arg = argv[i];
        if (arg.endsWith(".ds3")) {
            openFileList.append(arg);
            continue;
        }
    }

    int finishedExportFileNum = 0;
    int totalExportFileNum = 0;
    int succeedExportNum = 0;
    int exitCode = -1;
    std::vector<DocumentWindow*> windowList;
    auto checkToSafelyExit = [&]() {
        if (-1 == exitCode)
            return;
        for (int i = 0; i < openFileList.size(); ++i) {
            if (windowList[i]->isWorking())
                return;
        }
        app.exit(exitCode);
    };
    if (!openFileList.empty()) {
        windowList.push_back(firstWindow);
        for (int i = 1; i < openFileList.size(); ++i) {
            windowList.push_back(DocumentWindow::createDocumentWindow());
        }
        if (toggleColor) {
            for (auto& it : windowList)
                it->toggleRenderColor();
        }
        if (!waitingExportList.empty() && openFileList.size() == 1) {
            totalExportFileNum = openFileList.size() * waitingExportList.size();
            for (int i = 0; i < openFileList.size(); ++i) {
                QObject::connect(windowList[i], &DocumentWindow::workingStatusChanged, &app, checkToSafelyExit);
            }
            for (int i = 0; i < openFileList.size(); ++i) {
                QObject::connect(windowList[i], &DocumentWindow::waitingExportFinished, &app, [&](const QString& filename, bool isSuccessful) {
                    qDebug() << "Export to" << filename << (isSuccessful ? "isSuccessful" : "failed");
                    ++finishedExportFileNum;
                    if (isSuccessful)
                        ++succeedExportNum;
                    if (finishedExportFileNum == totalExportFileNum) {
                        if (succeedExportNum == totalExportFileNum) {
                            exitCode = 0;
                            checkToSafelyExit();
                            return;
                        }
                        exitCode = 1;
                        checkToSafelyExit();
                        return;
                    }
                });
            }
            for (int i = 0; i < openFileList.size(); ++i) {
                QObject::connect(windowList[i]->document(), &Document::exportReady, windowList[i], &DocumentWindow::checkExportWaitingList);
                windowList[i]->setExportWaitingList(waitingExportList);
            }
        }
        for (int i = 0; i < openFileList.size(); ++i) {
            windowList[i]->openPathAs(openFileList[i], openFileList[i]);
        }
    }

    return app.exec();
}
