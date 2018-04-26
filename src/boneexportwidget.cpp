#include "boneexportwidget.h"

BoneExportWidget::BoneExportWidget() :
    m_boneModelWidget(nullptr)
{
    m_boneModelWidget = new ModelWidget(this);
    m_boneModelWidget->setMinimumSize(128, 128);
    m_boneModelWidget->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
}
