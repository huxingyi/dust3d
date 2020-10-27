#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QPushButton>
#include "motionmanagewidget.h"
#include "motioneditwidget.h"
#include "theme.h"
#include "infolabel.h"
#include "motionpropertywidget.h"

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
    MotionPropertyWidget *motionPropertyWidget = new MotionPropertyWidget();
    //connect(m_document, &Document::resultRigChanged, [=]() {
    //    motionPropertyWidget->updateBones(m_document->resultRigBones());
    //});
    motionPropertyWidget->updateBones(m_document->resultRigBones(),
        m_document->resultRigWeights(),
        &m_document->currentRiggedOutcome());
    motionPropertyWidget->setAttribute(Qt::WA_DeleteOnClose);
    motionPropertyWidget->show();
    //showMotionDialog(QUuid());
}

void MotionManageWidget::showMotionDialog(QUuid motionId)
{
    MotionEditWidget *motionEditWidget = new MotionEditWidget(m_document);
    motionEditWidget->setAttribute(Qt::WA_DeleteOnClose);
    if (!motionId.isNull()) {
        const Motion *motion = m_document->findMotion(motionId);
        if (nullptr != motion) {
            motionEditWidget->setEditMotionId(motionId);
            motionEditWidget->setEditMotionName(motion->name);
            motionEditWidget->setEditMotionClips(motion->clips);
            motionEditWidget->clearUnsaveState();
            motionEditWidget->generatePreviews();
        }
    }
    motionEditWidget->show();
    connect(motionEditWidget, &QDialog::destroyed, [=]() {
        emit unregisterDialog((QWidget *)motionEditWidget);
    });
    emit registerDialog((QWidget *)motionEditWidget);
}
