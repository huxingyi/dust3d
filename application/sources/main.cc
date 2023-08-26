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

static QApplication* g_app = nullptr;
static std::vector<DocumentWindow*> g_windowList;
static QStringList g_openFileList;
static QStringList g_waitingExportList;
static int g_finishedExportFileNum = 0;
static int g_totalExportFileNum = 0;
static int g_succeedExportNum = 0;
static int g_exitCode = -1;

static auto checkToSafelyExit = []() {
    if (-1 == g_exitCode)
        return;
    for (int i = 0; i < g_openFileList.size(); ++i) {
        if (g_windowList[i]->isWorking())
            return;
    }
    g_app->exit(g_exitCode);
};

int main(int argc, char* argv[])
{
    g_app = new QApplication(argc, argv);

#if defined(Q_OS_WASM)
#else
    QSurfaceFormat format = QSurfaceFormat::defaultFormat();
    format.setProfile(QSurfaceFormat::OpenGLContextProfile::CoreProfile);
    format.setVersion(3, 3);
    QSurfaceFormat::setDefaultFormat(format);
#endif

    Theme::initialize();

    QCoreApplication::setApplicationName(APP_NAME);
    QCoreApplication::setOrganizationName(APP_COMPANY);
    QCoreApplication::setOrganizationDomain(APP_HOMEPAGE_URL);

    //freopen("dust3d.log", "w", stdout);
    //setvbuf(stdout, 0, _IONBF, 0);

    DocumentWindow* firstWindow = DocumentWindow::createDocumentWindow();

    bool toggleColor = false;
    for (int i = 1; i < argc; ++i) {
        if ('-' == argv[i][0]) {
            if (0 == strcmp(argv[i], "-output") || 0 == strcmp(argv[i], "-o")) {
                ++i;
                if (i < argc)
                    g_waitingExportList.append(argv[i]);
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
            g_openFileList.append(arg);
            continue;
        }
    }

    if (!g_openFileList.empty()) {
        g_windowList.push_back(firstWindow);
        for (int i = 1; i < g_openFileList.size(); ++i) {
            g_windowList.push_back(DocumentWindow::createDocumentWindow());
        }
        if (toggleColor) {
            for (auto& it : g_windowList)
                it->toggleRenderColor();
        }
        if (!g_waitingExportList.empty() && g_openFileList.size() == 1) {
            g_totalExportFileNum = g_openFileList.size() * g_waitingExportList.size();
            for (int i = 0; i < g_openFileList.size(); ++i) {
                QObject::connect(g_windowList[i], &DocumentWindow::workingStatusChanged, g_app, checkToSafelyExit);
            }
            for (int i = 0; i < g_openFileList.size(); ++i) {
                QObject::connect(g_windowList[i], &DocumentWindow::waitingExportFinished, g_app, [&](const QString& filename, bool isSuccessful) {
                    qDebug() << "Export to" << filename << (isSuccessful ? "isSuccessful" : "failed");
                    ++g_finishedExportFileNum;
                    if (isSuccessful)
                        ++g_succeedExportNum;
                    if (g_finishedExportFileNum == g_totalExportFileNum) {
                        if (g_succeedExportNum == g_totalExportFileNum) {
                            g_exitCode = 0;
                            checkToSafelyExit();
                            return;
                        }
                        g_exitCode = 1;
                        checkToSafelyExit();
                        return;
                    }
                });
            }
            for (int i = 0; i < g_openFileList.size(); ++i) {
                QObject::connect(g_windowList[i]->document(), &Document::exportReady, g_windowList[i], &DocumentWindow::checkExportWaitingList);
                g_windowList[i]->setExportWaitingList(g_waitingExportList);
            }
        }
        for (int i = 0; i < g_openFileList.size(); ++i) {
            g_windowList[i]->openPathAs(g_openFileList[i], g_openFileList[i]);
        }
    }
#if defined(Q_OS_WASM)
    return 0;
#else
    return g_app->exec();
#endif
}
