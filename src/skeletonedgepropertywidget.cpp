#include <QFormLayout>
#include <QObject>
#include <QComboBox>
#include <QtGlobal>
#include "skeletonedgepropertywidget.h"

SkeletonEdgePropertyWidget::SkeletonEdgePropertyWidget(const SkeletonDocument *document) :
    m_branchModeComboBox(nullptr),
    m_document(document)
{
    m_branchModeComboBox = new QComboBox;
    
    m_branchModeComboBox->insertItem(0, tr("Auto"), (int)SkeletonEdgeBranchMode::Auto);
    m_branchModeComboBox->insertItem(1, tr("No Trivial"), (int)SkeletonEdgeBranchMode::NoTrivial);
    m_branchModeComboBox->insertItem(2, tr("Trivial"), (int)SkeletonEdgeBranchMode::Trivial);
    
    QFormLayout *formLayout = new QFormLayout;
    formLayout->addRow(tr("Joint Branch Mode:"), m_branchModeComboBox);
    
    setLayout(formLayout);
    
    hide();

    connect(m_branchModeComboBox, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), [=](int index) {
        if (-1 != index) {
            int mode = m_branchModeComboBox->itemData(index).toInt();
            emit setEdgeBranchMode(m_edgeId, static_cast<SkeletonEdgeBranchMode>(mode));
        }
    });
}

void SkeletonEdgePropertyWidget::showProperties(QUuid edgeId)
{
    m_edgeId = edgeId;
    updateData();
    show();
}

void SkeletonEdgePropertyWidget::updateData()
{
    const SkeletonEdge *edge = m_document->findEdge(m_edgeId);
    if (nullptr == edge) {
        hide();
        return;
    }
    int selectIndex = m_branchModeComboBox->findData((int)edge->branchMode);
    m_branchModeComboBox->setCurrentIndex(selectIndex);
}

QUuid SkeletonEdgePropertyWidget::currentEdgeId()
{
    return m_edgeId;
}
