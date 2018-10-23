#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QPushButton>
#include "motionmanagewidget.h"
#include "motioneditwidget.h"
#include "theme.h"
#include "infolabel.h"

MotionManageWidget::MotionManageWidget(const SkeletonDocument *document, QWidget *parent) :
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
    infoLabel->setText(tr("Missing Rig"));
    infoLabel->show();
    
    auto refreshInfoLabel = [=]() {
        if (m_document->currentRigSucceed()) {
            infoLabel->hide();
            addMotionButton->show();
        } else {
            infoLabel->show();
            addMotionButton->hide();
        }
    };
    
    connect(m_document, &SkeletonDocument::resultRigChanged, this, refreshInfoLabel);
    connect(m_document, &SkeletonDocument::cleanup, this, refreshInfoLabel);
    
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
    MotionEditWidget *motionEditWidget = new MotionEditWidget(m_document);
    motionEditWidget->setAttribute(Qt::WA_DeleteOnClose);
    if (!motionId.isNull()) {
        const SkeletonMotion *motion = m_document->findMotion(motionId);
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
