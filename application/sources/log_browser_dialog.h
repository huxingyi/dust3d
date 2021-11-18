#ifndef DUST3D_APPLICATION_LOG_BROWSER_DIALOG_H_
#define DUST3D_APPLICATION_LOG_BROWSER_DIALOG_H_

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
    void outputMessage(QtMsgType type, const QString &msg, const QString &source, int line);

protected slots:
    void save();

protected:
    virtual void keyPressEvent(QKeyEvent *e);
    virtual void closeEvent(QCloseEvent *e);

    QTextBrowser *m_browser;
    QPushButton *m_clearButton;
    QPushButton *m_saveButton;
};

#endif
