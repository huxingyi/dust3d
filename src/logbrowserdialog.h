#ifndef LOG_BROWSER_DIALOG_H
#define LOG_BROWSER_DIALOG_H
// Modified from https://wiki.qt.io/Browser_for_QDebug_output
#include <QDialog>

class QTextBrowser;
class QPushButton;

class LogBrowserDialog : public QDialog
{
    Q_OBJECT
public:
    LogBrowserDialog(QWidget *parent = 0);
    ~LogBrowserDialog();

public slots:
    void outputMessage(QtMsgType type, const QString &msg);

protected slots:
    void save();

protected:
    virtual void keyPressEvent(QKeyEvent *e);
    virtual void closeEvent(QCloseEvent *e);

    QTextBrowser *m_browser;
    QPushButton *m_clearButton;
    QPushButton *m_saveButton;
};

#endif // DIALOG_H