#include "bone_property_widget.h"
#include "float_number_widget.h"
#include "image_preview_widget.h"
#include "theme.h"
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLineEdit>
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

    if (nullptr != m_bone) {
        m_nameEdit = new QLineEdit;
        Theme::initLineEdit(m_nameEdit);
        m_nameEdit->setFixedWidth(Theme::partPreviewImageSize * 2.7);
        m_nameEdit->setText(m_bone->name);

        connect(m_nameEdit, &QLineEdit::textChanged, this, &BonePropertyWidget::nameEditChanged);

        QHBoxLayout* renameLayout = new QHBoxLayout;
        renameLayout->addWidget(new QLabel(tr("Name")));
        renameLayout->addWidget(m_nameEdit);
        renameLayout->addStretch();

        mainLayout->addLayout(renameLayout);
    }

    mainLayout->setSizeConstraint(QLayout::SetFixedSize);

    connect(this, &BonePropertyWidget::renameBone, m_document, &Document::renameBone);
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

void BonePropertyWidget::nameEditChanged()
{
    const Document::Bone* bone = m_document->findBone(m_boneId);
    if (nullptr == bone)
        return;
    emit renameBone(m_boneId, m_nameEdit->text());
    emit groupOperationAdded();
}
