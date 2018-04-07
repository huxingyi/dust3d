#include <QFormLayout>
#include "skeletonnodepropertywidget.h"

SkeletonNodePropertyWidget::SkeletonNodePropertyWidget(const SkeletonDocument *document) :
    m_rootMarkModeComboBox(nullptr),
    m_document(document)
{
    m_rootMarkModeComboBox = new QComboBox;
    
    m_rootMarkModeComboBox->insertItem(0, tr("Auto"), (int)SkeletonNodeRootMarkMode::Auto);
    m_rootMarkModeComboBox->insertItem(1, tr("Mark as Root"), (int)SkeletonNodeRootMarkMode::MarkAsRoot);
    m_rootMarkModeComboBox->insertItem(2, tr("Mark as Not Root"), (int)SkeletonNodeRootMarkMode::MarkAsNotRoot);
    
    QFormLayout *formLayout = new QFormLayout;
    formLayout->addRow(tr("Root Mark:"), m_rootMarkModeComboBox);
    
    setLayout(formLayout);
    
    hide();

    connect(m_rootMarkModeComboBox, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), [=](int index) {
        if (-1 != index) {
            int mode = m_rootMarkModeComboBox->itemData(index).toInt();
            emit setNodeRootMarkMode(m_nodeId, static_cast<SkeletonNodeRootMarkMode>(mode));
        }
    });
}

void SkeletonNodePropertyWidget::showProperties(QUuid nodeId)
{
    m_nodeId = nodeId;
    updateData();
    show();
}

void SkeletonNodePropertyWidget::updateData()
{
    const SkeletonNode *node = m_document->findNode(m_nodeId);
    if (nullptr == node) {
        hide();
        return;
    }
    int selectIndex = m_rootMarkModeComboBox->findData((int)node->rootMarkMode);
    m_rootMarkModeComboBox->setCurrentIndex(selectIndex);
}

QUuid SkeletonNodePropertyWidget::currentNodeId()
{
    return m_nodeId;
}
