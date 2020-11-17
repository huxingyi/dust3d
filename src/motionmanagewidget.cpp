#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QPushButton>
#include "motionmanagewidget.h"
#include "theme.h"
#include "infolabel.h"
#include "motioneditwidget.h"

MotionManageWidget::MotionManageWidget(const Document *document, QWidget *parent) :
    QWidget(parent),
    m_document(document)
{
    QPushButton *addMotionButton = new QPushButton(Theme::awesome()->icon(fa::plus), tr("Add Motion..."));
    addMotionButton->hide();
    
    connect(addMotionButton, &QPushButton::clicked, this, &MotionManageWidget::showAddMotionDialog);
    
    QHBoxLayout *toolsLayout = new QHBoxLayout;
    toolsLayout->addWidget(addMotionButton);
    
    m_motionListWidget = new MotionListWidget(document);
    connect(m_motionListWidget, &MotionListWidget::modifyMotion, this, &MotionManageWidget::showMotionDialog);
    
    InfoLabel *infoLabel = new InfoLabel;
    infoLabel->hide();
    
    auto refreshInfoLabel = [=]() {
        if (m_document->currentRigSucceed()) {
            if (m_document->rigType == RigType::Animal) {
                infoLabel->setText("");
                infoLabel->hide();
                addMotionButton->show();
            } else {
                infoLabel->setText(tr("Motion editor doesn't support this rig type yet: ") + RigTypeToDispName(m_document->rigType));
                infoLabel->show();
                addMotionButton->hide();
            }
        } else {
            infoLabel->setText(tr("Missing Rig"));
            infoLabel->show();
            addMotionButton->hide();
        }
    };
    
    connect(m_document, &Document::resultRigChanged, this, refreshInfoLabel);
    connect(m_document, &Document::cleanup, this, refreshInfoLabel);
    
    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->addWidget(infoLabel);
    mainLayout->addLayout(toolsLayout);
    mainLayout->addWidget(m_motionListWidget);
    
    setLayout(mainLayout);
}

QSize MotionManageWidget::sizeHint() const
{
    return QSize(Theme::sidebarPreferredWidth, 0);
}

void MotionManageWidget::showAddMotionDialog()
{
    showMotionDialog(QUuid());
}

void MotionManageWidget::showMotionDialog(QUuid motionId)
{
    MotionEditWidget *motionEditWidget = new MotionEditWidget;
    motionEditWidget->setAttribute(Qt::WA_DeleteOnClose);
    motionEditWidget->updateBones(m_document->rigType,
        m_document->resultRigBones(),
        m_document->resultRigWeights(),
        &m_document->currentRiggedObject());
    if (!motionId.isNull()) {
        const Motion *motion = m_document->findMotion(motionId);
        if (nullptr != motion) {
            motionEditWidget->setEditMotionId(motionId);
            motionEditWidget->setEditMotionName(motion->name);
            motionEditWidget->setEditMotionParameters(motion->parameters);
            motionEditWidget->clearUnsaveState();
            motionEditWidget->generatePreview();
        }
    }
    motionEditWidget->show();
    connect(motionEditWidget, &QMainWindow::destroyed, [=]() {
        emit unregisterDialog((QWidget *)motionEditWidget);
    });
    connect(motionEditWidget, &MotionEditWidget::addMotion, m_document, &Document::addMotion);
    connect(motionEditWidget, &MotionEditWidget::renameMotion, m_document, &Document::renameMotion);
    connect(motionEditWidget, &MotionEditWidget::setMotionParameters, m_document, &Document::setMotionParameters);
    emit registerDialog((QWidget *)motionEditWidget);
}
