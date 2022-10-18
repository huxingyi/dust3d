#include <QOpenGLFunctions>
#include <QTextEdit>
#include <QVBoxLayout>

#include "about_widget.h"
#include "model_widget.h"
#include "version.h"

AboutWidget::AboutWidget()
{
    QTextEdit* versionInfoLabel = new QTextEdit;
    versionInfoLabel->setText(QString("%1 %2 (version: %3 build: %4 %5)\nopengl: %6 shader: %7 core: %8").arg(APP_NAME).arg(APP_HUMAN_VER).arg(APP_VER).arg(__DATE__).arg(__TIME__).arg(ModelWidget::m_openGLVersion).arg(ModelWidget::m_openGLShadingLanguageVersion).arg(ModelWidget::m_openGLIsCoreProfile ? "true" : "false"));
    versionInfoLabel->setReadOnly(true);

    QVBoxLayout* mainLayout = new QVBoxLayout;
    mainLayout->addWidget(versionInfoLabel);

    setLayout(mainLayout);
    setFixedSize(QSize(350, 100));

    setWindowTitle(applicationTitle(tr("About")));
}
