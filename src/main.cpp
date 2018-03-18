#include <QApplication>
#include <QDesktopWidget>
#include <QStyleFactory>
#include <QFontDatabase>
#include "mainwindow.h"
#include "meshlite.h"

int main(int argc, char ** argv)
{
    QApplication app(argc, argv);
    
    // QuantumCD/Qt 5 Dark Fusion Palette
    // https://gist.github.com/QuantumCD/6245215
    qApp->setStyle(QStyleFactory::create("Fusion"));
    QPalette darkPalette;
    darkPalette.setColor(QPalette::Window, QColor(53,53,53));
    darkPalette.setColor(QPalette::WindowText, QColor(239,239,239));
    darkPalette.setColor(QPalette::Base, QColor(25,25,25));
    darkPalette.setColor(QPalette::AlternateBase, QColor(53,53,53));
    darkPalette.setColor(QPalette::ToolTipBase, Qt::white);
    darkPalette.setColor(QPalette::ToolTipText, Qt::white);
    darkPalette.setColor(QPalette::Text, Qt::white);
    darkPalette.setColor(QPalette::Button, QColor(53,53,53));
    darkPalette.setColor(QPalette::ButtonText, QColor(239,239,239));
    darkPalette.setColor(QPalette::BrightText, Qt::red);
    darkPalette.setColor(QPalette::Link, QColor(42, 130, 218));
    darkPalette.setColor(QPalette::Highlight, QColor(42, 130, 218));
    darkPalette.setColor(QPalette::HighlightedText, Qt::black);
    qApp->setPalette(darkPalette);
    qApp->setStyleSheet("QToolTip { color: #ffffff; background-color: #fc6621; border: 1px solid white; }");
    
    QCoreApplication::setApplicationName("Dust 3D");
    
    const QFont fixedFont = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    QApplication::setFont(fixedFont);
    
    MainWindow mainWindow;
    mainWindow.showMaximized();
    return app.exec();
}
