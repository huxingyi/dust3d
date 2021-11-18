#ifndef DUST3D_APPLICATION_LOG_BROWSER_H_
#define DUST3D_APPLICATION_LOG_BROWSER_H_

#include <QObject>
#include <cstdio>

class LogBrowserDialog;

class LogBrowser : public QObject
{
    Q_OBJECT
public:
    explicit LogBrowser(QObject *parent=0);
    ~LogBrowser();

public slots:
    void outputMessage(QtMsgType type, const QString &msg, const QString &source, int line);
    void showDialog();
    void hideDialog();
    bool isDialogVisible();

signals:
    void sendMessage(QtMsgType type, const QString &msg, const QString &source, int line);

private:
    LogBrowserDialog *m_browserDialog = nullptr;
    FILE *m_outputTo = nullptr;
    static bool m_enableOutputToFile;
};

#endif
