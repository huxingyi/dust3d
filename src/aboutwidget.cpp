#include <QTextEdit>
#include <QVBoxLayout>
#include "aboutwidget.h"

AboutWidget::AboutWidget()
{
    QTextEdit *versionInfoLabel = new QTextEdit;
    versionInfoLabel->setText(QString("%1 %2 (version: %3 build: %4 %5)").arg(APP_NAME).arg(APP_HUMAN_VER).arg(APP_VER).arg(__DATE__).arg(__TIME__));
    versionInfoLabel->setReadOnly(true);
    
    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->addWidget(versionInfoLabel);
    
    setLayout(mainLayout);
    setFixedSize(QSize(350, 75));
    
    setWindowTitle(APP_NAME);
}
