#include <QFormLayout>
#include <QComboBox>
#include "rigwidget.h"
#include "rigtype.h"
#include "infolabel.h"

RigWidget::RigWidget(const SkeletonDocument *document, QWidget *parent) :
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
    
    formLayout->addRow(tr("Type"), m_rigTypeBox);
    
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
    
    m_missingMarksInfoLabel = new InfoLabel;
    m_missingMarksInfoLabel->hide();
    
    m_errorMarksInfoLabel = new InfoLabel;
    m_errorMarksInfoLabel->hide();
    
    QVBoxLayout *layout = new QVBoxLayout;
    layout->addLayout(formLayout);
    layout->addWidget(m_rigWeightRenderWidget);
    layout->addWidget(m_missingMarksInfoLabel);
    layout->addWidget(m_errorMarksInfoLabel);
    layout->addStretch();
    
    setLayout(layout);
}

void RigWidget::rigTypeChanged()
{
    m_rigTypeBox->setCurrentIndex((int)m_document->rigType);
}

void RigWidget::updateResultInfo()
{
    QStringList missingMarkNames;
    for (const auto &markName: m_document->resultRigMissingMarkNames()) {
        missingMarkNames.append(markName);
    }
    QString missingNamesString = missingMarkNames.join(tr(", "));
    if (missingNamesString.isEmpty()) {
        m_missingMarksInfoLabel->hide();
    } else {
        m_missingMarksInfoLabel->setText(tr("Missing marks: ") + missingNamesString);
        m_missingMarksInfoLabel->setMaximumWidth(width() * 0.8);
        m_missingMarksInfoLabel->show();
    }
    
    QStringList errorMarkNames;
    for (const auto &markName: m_document->resultRigErrorMarkNames()) {
        errorMarkNames.append(markName);
    }
    QString errorNamesString = errorMarkNames.join(tr(","));
    if (errorNamesString.isEmpty()) {
        m_errorMarksInfoLabel->hide();
    } else {
        m_errorMarksInfoLabel->setText(tr("Error marks: ") + errorNamesString);
        m_errorMarksInfoLabel->setMaximumWidth(width() * 0.8);
        m_errorMarksInfoLabel->show();
    }
}

ModelWidget *RigWidget::rigWeightRenderWidget()
{
    return m_rigWeightRenderWidget;
}
