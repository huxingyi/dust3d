#include "bone_property_widget.h"
#include "float_number_widget.h"
#include "image_preview_widget.h"
#include "theme.h"
#include <QComboBox>
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
        m_parentBoneComboBox = new QComboBox;
        m_parentBoneComboBox->addItem(tr("None"));

        m_parentJointComboBox = new QComboBox;
        m_parentJointComboBox->addItem(tr("1"));
        m_parentJointComboBox->setVisible(false);

        QHBoxLayout* parentBoneAndJointLayout = new QHBoxLayout;
        parentBoneAndJointLayout->addWidget(m_parentBoneComboBox);
        parentBoneAndJointLayout->addWidget(m_parentJointComboBox);

        for (size_t i = 0; i < m_document->boneIdList.size(); ++i) {
            const auto& boneId = m_document->boneIdList[i];
            if (boneId == m_boneId)
                continue;
            const Document::Bone* bone = m_document->findBone(boneId);
            if (nullptr == bone)
                continue;
            m_parentBoneComboBox->addItem(QIcon(bone->previewPixmap), bone->name, QVariant(QString::fromStdString(boneId.toString())));
            if (m_bone->attachBoneId == boneId)
                m_parentBoneComboBox->setCurrentIndex(m_parentBoneComboBox->count() - 1);
        }
        updateBoneJointComboBox();

        connect(m_parentBoneComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &BonePropertyWidget::updateBoneJointComboBox);
        connect(m_parentJointComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &BonePropertyWidget::synchronizeBoneAttachmentFromControls);

        QHBoxLayout* parentLayout = new QHBoxLayout;
        parentLayout->addWidget(new QLabel(tr("Parent")));
        parentLayout->addLayout(parentBoneAndJointLayout);
        parentLayout->addStretch();

        m_nameEdit = new QLineEdit;
        Theme::initLineEdit(m_nameEdit);
        m_nameEdit->setFixedWidth(Theme::partPreviewImageSize * 2.7);
        m_nameEdit->setText(m_bone->name);

        connect(m_nameEdit, &QLineEdit::textChanged, this, &BonePropertyWidget::nameEditChanged);

        QHBoxLayout* renameLayout = new QHBoxLayout;
        renameLayout->addWidget(new QLabel(tr("Name")));
        renameLayout->addWidget(m_nameEdit);
        renameLayout->addStretch();

        m_jointsWidget = new IntNumberWidget;
        m_jointsWidget->setRange(2, 10);

        QPushButton* nodePicker = new QPushButton(Theme::awesome()->icon(fa::eyedropper), "");
        nodePicker->setToolTip(tr("Click node one by one on canvas as joints in order"));
        Theme::initIconButton(nodePicker);

        connect(nodePicker, &QPushButton::clicked, this, [this]() {
            emit this->pickBoneJoints(this->m_boneId, (size_t)this->m_jointsWidget->value());
        });

        QHBoxLayout* jointsLayout = new QHBoxLayout;
        jointsLayout->addWidget(new QLabel(tr("Joints")));
        jointsLayout->addWidget(m_jointsWidget);
        jointsLayout->addStretch();
        jointsLayout->addWidget(nodePicker);

        mainLayout->addLayout(parentLayout);
        mainLayout->addLayout(renameLayout);
        mainLayout->addLayout(jointsLayout);
    }

    mainLayout->setSizeConstraint(QLayout::SetFixedSize);

    connect(this, &BonePropertyWidget::renameBone, m_document, &Document::renameBone);
    connect(this, &BonePropertyWidget::setBoneAttachment, m_document, &Document::setBoneAttachment);
    connect(this, &BonePropertyWidget::groupOperationAdded, m_document, &Document::saveSnapshot);

    setLayout(mainLayout);

    setFixedSize(minimumSizeHint());
}

dust3d::Uuid BonePropertyWidget::editingParentBoneId()
{
    return dust3d::Uuid(m_parentBoneComboBox->currentData().toString().toStdString());
}

int BonePropertyWidget::editingParentJointIndex()
{
    return dust3d::String::toInt(m_parentJointComboBox->currentText().toStdString()) - 1;
}

void BonePropertyWidget::synchronizeBoneAttachmentFromControls()
{
    if (nullptr == m_bone)
        return;

    auto parentBoneId = editingParentBoneId();
    int parentJointIndex = editingParentJointIndex();

    if (-1 == parentJointIndex) {
        // The joint control is initializing, ignore
        return;
    }

    if (m_bone->attachBoneId == parentBoneId && parentJointIndex == m_bone->attachBoneJointIndex)
        return;

    emit setBoneAttachment(m_boneId, parentBoneId, parentJointIndex);
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

void BonePropertyWidget::updateBoneJointComboBox()
{
    if (nullptr == m_bone)
        return;

    auto parentBoneId = editingParentBoneId();
    const Document::Bone* parentBone = m_document->findBone(parentBoneId);
    if (nullptr == parentBone || parentBone->joints.size() < 2) {
        if (1 == m_parentJointComboBox->count())
            return;
        m_parentJointComboBox->clear();
        m_parentJointComboBox->addItem(tr("1"));
        m_parentJointComboBox->setVisible(false);
        synchronizeBoneAttachmentFromControls();
        return;
    }
    m_parentJointComboBox->clear();
    for (size_t i = 0; i < parentBone->joints.size(); ++i) {
        m_parentJointComboBox->addItem(QString::number(i + 1));
        if (i == (size_t)m_bone->attachBoneJointIndex)
            m_parentJointComboBox->setCurrentIndex(m_parentJointComboBox->count() - 1);
    }
    m_parentJointComboBox->setVisible(true);
    synchronizeBoneAttachmentFromControls();
}
