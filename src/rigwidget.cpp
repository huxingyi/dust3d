#include <QFormLayout>
#include <QComboBox>
#include <QHBoxLayout>
#include "rigwidget.h"
#include "rigtype.h"
#include "infolabel.h"
#include "theme.h"

RigWidget::RigWidget(const Document *document, QWidget *parent) :
    QWidget(parent),
    m_document(document)
{
    QFormLayout *formLayout = new QFormLayout;
    m_rigTypeBox = new QComboBox;
    m_rigTypeBox->setEditable(false);
    
    for (int i = 0; i < (int)RigType::Count; i++) {
        RigType rigType = (RigType)(i);
        m_rigTypeBox->addItem(RigTypeToDispName(rigType));
    }
    
    QHBoxLayout *controlsLayout = new QHBoxLayout;
    controlsLayout->addWidget(m_rigTypeBox);
    
    formLayout->addRow(tr("Type"), controlsLayout);
    
    m_rigTypeBox->setCurrentIndex((int)m_document->rigType);
    
    connect(m_rigTypeBox, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), [=](int index) {
        RigType currentRigType = (RigType)index;
        emit setRigType(currentRigType);
    });
    
    m_rigWeightRenderWidget = new ModelWidget(this);
    m_rigWeightRenderWidget->setMinimumSize(128, 128);
    m_rigWeightRenderWidget->setXRotation(0);
    m_rigWeightRenderWidget->setYRotation(0);
    m_rigWeightRenderWidget->setZRotation(0);
    //m_rigWeightRenderWidget->toggleWireframe();
    
    m_infoLabel = new InfoLabel;
    m_infoLabel->hide();
    
    QVBoxLayout *layout = new QVBoxLayout;
    layout->addLayout(formLayout);
    layout->addWidget(m_rigWeightRenderWidget);
    layout->addWidget(m_infoLabel);
    layout->addStretch();
    
    setLayout(layout);
}

void RigWidget::rigTypeChanged()
{
    m_rigTypeBox->setCurrentIndex((int)m_document->rigType);
}

void RigWidget::updateResultInfo()
{
    const auto &messages = m_document->resultRigMessages();
    if (messages.empty()) {
        m_infoLabel->hide();
    } else {
        QStringList messageList;
        for (const auto &item: messages)
            messageList.append(item.second);
        m_infoLabel->setText(messageList.join("<br><br>"));
        m_infoLabel->setMaximumWidth(width() * 0.90);
        m_infoLabel->show();
    }
}

ModelWidget *RigWidget::rigWeightRenderWidget()
{
    return m_rigWeightRenderWidget;
}
