#include <QVBoxLayout>
#include "posewidget.h"

PoseWidget::PoseWidget(const Document *document, QUuid poseId) :
    m_poseId(poseId),
    m_document(document)
{
    setObjectName("PoseFrame");

    m_previewWidget = new ModelWidget(this);
    m_previewWidget->setAttribute(Qt::WA_TransparentForMouseEvents);
    m_previewWidget->setFixedSize(Theme::posePreviewImageSize, Theme::posePreviewImageSize);
    m_previewWidget->enableMove(false);
    m_previewWidget->enableZoom(false);
    
    m_nameLabel = new QLabel;
    m_nameLabel->setAlignment(Qt::AlignCenter);
    m_nameLabel->setStyleSheet("background: qlineargradient(x1:0.5 y1:-15.5, x2:0.5 y2:1, stop:0 " + Theme::white.name() + ", stop:1 #252525);");
    
    QFont nameFont;
    nameFont.setWeight(QFont::Light);
    //nameFont.setPixelSize(9);
    nameFont.setBold(false);
    m_nameLabel->setFont(nameFont);
    
    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->addStretch();
    mainLayout->addWidget(m_nameLabel);
    
    setLayout(mainLayout);
    
    setFixedSize(Theme::posePreviewImageSize, PoseWidget::preferredHeight());
    
    connect(document, &Document::poseNameChanged, this, [=](QUuid poseId) {
        if (poseId != m_poseId)
            return;
        updateName();
    });
    connect(document, &Document::posePreviewChanged, this, [=](QUuid poseId) {
        if (poseId != m_poseId)
            return;
        updatePreview();
    });
}

void PoseWidget::setCornerButtonVisible(bool visible)
{
    if (nullptr == m_cornerButton) {
        m_cornerButton = new QPushButton(this);
        m_cornerButton->move(Theme::posePreviewImageSize - Theme::miniIconSize - 2, 2);
        Theme::initAwesomeMiniButton(m_cornerButton);
        m_cornerButton->setText(QChar(fa::plussquare));
        connect(m_cornerButton, &QPushButton::clicked, this, [=]() {
            emit cornerButtonClicked(m_poseId);
        });
    }
    m_cornerButton->setVisible(visible);
}

void PoseWidget::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    m_previewWidget->move((width() - Theme::posePreviewImageSize) / 2, 0);
}

int PoseWidget::preferredHeight()
{
    return Theme::posePreviewImageSize;
}

void PoseWidget::reload()
{
    updatePreview();
    updateName();
}

void PoseWidget::updatePreview()
{
    const Pose *pose = m_document->findPose(m_poseId);
    if (!pose) {
        qDebug() << "Pose not found:" << m_poseId;
        return;
    }
    Model *previewMesh = pose->takePreviewMesh();
    m_previewWidget->updateMesh(previewMesh);
}

void PoseWidget::updateName()
{
    const Pose *pose = m_document->findPose(m_poseId);
    if (!pose) {
        qDebug() << "Pose not found:" << m_poseId;
        return;
    }
    m_nameLabel->setText(pose->name);
}

void PoseWidget::updateCheckedState(bool checked)
{
    if (checked)
        setStyleSheet("#PoseFrame {border: 1px solid " + Theme::red.name() + ";}");
    else
        setStyleSheet("#PoseFrame {border: 1px solid transparent;}");
}

ModelWidget *PoseWidget::previewWidget()
{
    return m_previewWidget;
}

void PoseWidget::mouseDoubleClickEvent(QMouseEvent *event)
{
    QFrame::mouseDoubleClickEvent(event);
    emit modifyPose(m_poseId);
}
