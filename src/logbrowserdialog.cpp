#include "logbrowserdialog.h"
// Modified from https://wiki.qt.io/Browser_for_QDebug_output
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTextBrowser>
#include <QPushButton>
#include <QFileDialog>
#include <QDir>
#include <QFile>
#include <QMessageBox>
#include <QTextStream>
#include <QCloseEvent>
#include <QKeyEvent>

LogBrowserDialog::LogBrowserDialog(QWidget *parent) :
    QDialog(parent)
{
    QVBoxLayout *layout = new QVBoxLayout;
    setLayout(layout);

    m_browser = new QTextBrowser(this);
    layout->addWidget(m_browser);

    QHBoxLayout *buttonLayout = new QHBoxLayout;
    buttonLayout->setContentsMargins(0, 0, 0, 0);
    layout->addLayout(buttonLayout);

    buttonLayout->addStretch(10);

    m_clearButton = new QPushButton(this);
    m_clearButton->setText("Clear");
    buttonLayout->addWidget(m_clearButton);
    connect(m_clearButton, SIGNAL(clicked()), m_browser, SLOT(clear()));

    m_saveButton = new QPushButton(this);
    m_saveButton->setText("Save Output");
    buttonLayout->addWidget(m_saveButton);
    connect(m_saveButton, SIGNAL(clicked()), this, SLOT(save()));

    resize(400, 300);
}


LogBrowserDialog::~LogBrowserDialog()
{

}


void LogBrowserDialog::outputMessage(QtMsgType type, const QString &msg)
{
    switch (type) {
    case QtDebugMsg:
        m_browser->append(tr("— DEBUG: %1").arg(msg));
        break;

    case QtWarningMsg:
        m_browser->append(tr("— WARNING: %1").arg(msg));
        break;

    case QtCriticalMsg:
        m_browser->append(tr("— CRITICAL: %1").arg(msg));
        break;

    case QtInfoMsg:
        m_browser->append(tr("— INFO: %1").arg(msg));
        break;

    case QtFatalMsg:
        m_browser->append(tr("— FATAL: %1").arg(msg));
        break;
    }
}


void LogBrowserDialog::save()
{
    QString saveFileName = QFileDialog::getSaveFileName(this,
        tr("Save Log Output"),
        tr("%1/logfile.txt").arg(QDir::homePath()),
        tr("Text Files (*.txt);;All Files (*)"));

    if (saveFileName.isEmpty())
        return;

    QFile file(saveFileName);
    if (!file.open(QIODevice::WriteOnly)) {
        QMessageBox::warning(this,
            tr("Error"),
            QString(tr("<nobr>File '%1'<br/>cannot be opened for writing.<br/><br/>"
            "The log output could <b>not</b> be saved!</nobr>"))
            .arg(saveFileName));
        return;
    }

    QTextStream stream(&file);
    stream << m_browser->toPlainText();
    file.close();
}

void LogBrowserDialog::closeEvent(QCloseEvent *e)
{
    QMessageBox::StandardButton answer = QMessageBox::question(this,
        tr("Close Log Browser?"),
        tr("Do you really want to close the log browser?"),
        QMessageBox::Yes | QMessageBox::No);

    if (answer == QMessageBox::Yes)
        e->accept();
    else
        e->ignore();
}

void LogBrowserDialog::keyPressEvent(QKeyEvent *e)
{
    // ignore all keyboard events
    // protects against accidentally closing of the dialog
    // without asking the user
    e->ignore();
}
