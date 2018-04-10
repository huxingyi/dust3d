#ifndef LOG_BROWSER_H
#define LOG_BROWSER_H
// Modified from https://wiki.qt.io/Browser_for_QDebug_output
#include <QObject>

class LogBrowserDialog;

class LogBrowser : public QObject
{
    Q_OBJECT
public:
    explicit LogBrowser(QObject *parent = 0);
    ~LogBrowser();

public slots:
    void outputMessage(QtMsgType type, const QString &msg);
    void showDialog();
    void hideDialog();
    bool isDialogVisible();

signals:
    void sendMessage(QtMsgType type, const QString &msg);

private:
    LogBrowserDialog *m_browserDialog;
};

#endif // LOGBROWSER_H
