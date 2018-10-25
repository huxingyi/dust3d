#include <QTextEdit>
#include <QVBoxLayout>
#include <QOpenGLFunctions>
#include "aboutwidget.h"
#include "version.h"
#include "util.h"

AboutWidget::AboutWidget()
{
    QTextEdit *versionInfoLabel = new QTextEdit;
    versionInfoLabel->setText(QString("%1 %2 (version: %3 build: %4 %5)\nopengl: %6 shader: %7 core: %8").arg(APP_NAME).arg(APP_HUMAN_VER).arg(APP_VER).arg(__DATE__).arg(__TIME__).arg((char *)glGetString(GL_VERSION)).arg((char *)glGetString(GL_SHADING_LANGUAGE_VERSION)).arg(QSurfaceFormat::defaultFormat().profile() == QSurfaceFormat::CoreProfile ? "true" : "false"));
    versionInfoLabel->setReadOnly(true);
    
    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->addWidget(versionInfoLabel);
    
    setLayout(mainLayout);
    setFixedSize(QSize(350, 75));
    
    setWindowTitle(unifiedWindowTitle(tr("About")));
}
