// Modified from https://wiki.qt.io/Browser_for_QDebug_output
#include <QMetaType>
#include <QDir>
#include "logbrowser.h"
#include "logbrowserdialog.h"

bool LogBrowser::m_enableOutputToFile = true;

LogBrowser::LogBrowser(QObject *parent) :
    QObject(parent)
{
    qRegisterMetaType<QtMsgType>("QtMsgType");
    m_browserDialog = new LogBrowserDialog;
    connect(this, &LogBrowser::sendMessage, m_browserDialog, &LogBrowserDialog::outputMessage, Qt::QueuedConnection);
    
    if (m_enableOutputToFile) {
        QString filePath = "dust3d.log";
        m_outputTo = fopen(filePath.toUtf8().constData(), "w");
    }
}

LogBrowser::~LogBrowser()
{
    delete m_browserDialog;
    if (m_outputTo)
        fclose(m_outputTo);
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
    if (m_outputTo) {
        fprintf(m_outputTo, "[%s:%d]: %s\n", source.toUtf8().constData(), line, msg.toUtf8().constData());
        fflush(m_outputTo);
    }
    emit sendMessage(type, msg, source, line);
}
