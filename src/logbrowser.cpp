#include "logbrowser.h"
// Modified from https://wiki.qt.io/Browser_for_QDebug_output
#include <QMetaType>
#include "logbrowserdialog.h"

LogBrowser::LogBrowser(QObject *parent) :
    QObject(parent)
{
    qRegisterMetaType<QtMsgType>("QtMsgType");
    m_browserDialog = new LogBrowserDialog;
    connect(this, &LogBrowser::sendMessage, m_browserDialog, &LogBrowserDialog::outputMessage, Qt::QueuedConnection);
}

LogBrowser::~LogBrowser()
{
    delete m_browserDialog;
}

void LogBrowser::showDialog()
{
    m_browserDialog->show();
    m_browserDialog->activateWindow();
    m_browserDialog->raise();
}

void LogBrowser::hideDialog()
{
    m_browserDialog->hide();
}

bool LogBrowser::isDialogVisible()
{
    return m_browserDialog->isVisible();
}

void LogBrowser::outputMessage(QtMsgType type, const QString &msg, const QString &source, int line)
{
#ifdef IN_DEVELOPMENT
    printf("%s\n", msg.toUtf8().constData());
#endif
    emit sendMessage(type, msg, source, line);
}
