#include "logbrowser.h"
// Modified from https://wiki.qt.io/Browser_for_QDebug_output
#include <QMetaType>
#include "logbrowserdialog.h"

LogBrowser::LogBrowser(QObject *parent) :
    QObject(parent)
{
    qRegisterMetaType<QtMsgType>("QtMsgType");
    m_browserDialog = new LogBrowserDialog;
    connect(this, SIGNAL(sendMessage(QtMsgType,QString)), m_browserDialog, SLOT(outputMessage(QtMsgType,QString)), Qt::QueuedConnection);
    m_browserDialog->show();
}

LogBrowser::~LogBrowser()
{
    delete m_browserDialog;
}

void LogBrowser::outputMessage(QtMsgType type, const QString &msg)
{
    printf("%s\n", msg.toUtf8().constData());
    emit sendMessage( type, msg );
}
