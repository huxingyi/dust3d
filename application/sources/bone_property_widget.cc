#include "bone_property_widget.h"
#include "float_number_widget.h"
#include "image_preview_widget.h"
#include "theme.h"
#include <QGroupBox>
#include <QHBoxLayout>
#include <QPushButton>
#include <QVBoxLayout>
#include <unordered_set>

BonePropertyWidget::BonePropertyWidget(Document* document,
    const std::vector<dust3d::Uuid>& boneIds,
    QWidget* parent)
    : QWidget(parent)
    , m_document(document)
    , m_boneIds(boneIds)
{
    prepareBoneIds();

    QVBoxLayout* mainLayout = new QVBoxLayout;

    mainLayout->setSizeConstraint(QLayout::SetFixedSize);

    connect(this, &BonePropertyWidget::groupOperationAdded, m_document, &Document::saveSnapshot);

    setLayout(mainLayout);

    setFixedSize(minimumSizeHint());
}

void BonePropertyWidget::prepareBoneIds()
{
    if (1 == m_boneIds.size()) {
        m_bone = m_document->findBone(m_boneIds.front());
        if (nullptr != m_bone)
            m_boneId = m_boneIds.front();
    }
}
