#include <QApplication>
#include <QDesktopWidget>
#include <QStyleFactory>
#include <QFontDatabase>
#include <QDebug>
#include <QtGlobal>
#include <QSurfaceFormat>
#include <QSettings>
#include <QTranslator>
#include "documentwindow.h"
#include "theme.h"
#include "version.h"

int main(int argc, char ** argv)
{
    QApplication app(argc, argv);
    
    QTranslator translator;
    if (translator.load(QLocale(), QLatin1String("dust3d"), QLatin1String("_"), QLatin1String(":/languages")))
        app.installTranslator(&translator);
    
    //QSurfaceFormat format = QSurfaceFormat::defaultFormat();
    //format.setProfile(QSurfaceFormat::OpenGLContextProfile::CoreProfile);
    //format.setVersion(3, 3);
    //QSurfaceFormat::setDefaultFormat(format);
    
    // QuantumCD/Qt 5 Dark Fusion Palette
    // https://gist.github.com/QuantumCD/6245215
    qApp->setStyle(QStyleFactory::create("Fusion"));
    QPalette darkPalette;
    darkPalette.setColor(QPalette::Window, Theme::black);
    darkPalette.setColor(QPalette::WindowText, Theme::white);
    darkPalette.setColor(QPalette::Base, QColor(25,25,25));
    darkPalette.setColor(QPalette::AlternateBase, QColor(53,53,53));
    //darkPalette.setColor(QPalette::ToolTipBase, Theme::white);
    //darkPalette.setColor(QPalette::ToolTipText, Theme::white);
    darkPalette.setColor(QPalette::Text, Theme::white);
    darkPalette.setColor(QPalette::Disabled, QPalette::Text, Theme::black);
    darkPalette.setColor(QPalette::Button, QColor(53,53,53));
    darkPalette.setColor(QPalette::ButtonText, Theme::white);
    darkPalette.setColor(QPalette::BrightText, Theme::red);
    darkPalette.setColor(QPalette::Link, QColor(42, 130, 218));
    darkPalette.setColor(QPalette::Highlight, Theme::red);
    darkPalette.setColor(QPalette::HighlightedText, Theme::black);    
    qApp->setPalette(darkPalette);
    //qApp->setStyleSheet("QToolTip { color: #ffffff; background-color: #fc6621; border: 1px solid white; }");
    
    QCoreApplication::setApplicationName(APP_NAME);
    QCoreApplication::setOrganizationName(APP_COMPANY);
    QCoreApplication::setOrganizationDomain(APP_HOMEPAGE_URL);
    
    QFont font;
    font.setWeight(QFont::Light);
    //font.setPixelSize(11);
    font.setBold(false);
    QApplication::setFont(font);
    
    Theme::initAwsomeBaseSizes();
    
    DocumentWindow *firstWindow = DocumentWindow::createDocumentWindow();
    
    qDebug() << "Language:" << QLocale().name();
    
    QStringList openFileList;
    QStringList waitingExportList;
    for (int i = 1; i < argc; ++i) {
        if ('-' == argv[i][0]) {
            if (0 == strcmp(argv[i], "-output") ||
                    0 == strcmp(argv[i], "-o")) {
                ++i;
                if (i < argc)
                    waitingExportList.append(argv[i]);
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
    if (!openFileList.empty()) {
        std::vector<DocumentWindow *> windowList = {firstWindow};
        for (int i = 1; i < openFileList.size(); ++i) {
            windowList.push_back(DocumentWindow::createDocumentWindow());
        }
        if (!waitingExportList.empty() &&
                openFileList.size() == 1) {
            totalExportFileNum = openFileList.size() * waitingExportList.size();
            for (int i = 0; i < openFileList.size(); ++i) {
                QObject::connect(windowList[i], &DocumentWindow::waitingExportFinished, &app, [&](const QString &filename, bool succeed) {
                    qDebug() << "Export to" << filename << (succeed ? "succeed" : "failed");
                    ++finishedExportFileNum;
                    if (succeed)
                        ++succeedExportNum;
                    if (finishedExportFileNum == totalExportFileNum) {
                        if (succeedExportNum == totalExportFileNum) {
                            app.exit();
                            return;
                        }
                        app.exit(1);
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
