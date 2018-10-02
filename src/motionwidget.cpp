#include <QVBoxLayout>
#include "motionwidget.h"

MotionWidget::MotionWidget(const SkeletonDocument *document, QUuid motionId) :
    m_motionId(motionId),
    m_document(document)
{
    setObjectName("MotionFrame");

    m_previewWidget = new InterpolationGraphicsWidget(this);
    m_previewWidget->setFixedSize(Theme::motionPreviewImageSize, Theme::motionPreviewImageSize);
    m_previewWidget->setPreviewOnly(true);
    
    m_nameLabel = new QLabel;
    m_nameLabel->setAlignment(Qt::AlignCenter);
    m_nameLabel->setStyleSheet("background: qlineargradient(x1:0.5 y1:-15.5, x2:0.5 y2:1, stop:0 " + Theme::white.name() + ", stop:1 #252525);");
    
    QFont nameFont;
    nameFont.setWeight(QFont::Light);
    nameFont.setPixelSize(9);
    nameFont.setBold(false);
    m_nameLabel->setFont(nameFont);
    
    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->addStretch();
    mainLayout->addWidget(m_nameLabel);
    
    setLayout(mainLayout);
    
    setFixedSize(Theme::motionPreviewImageSize, MotionWidget::preferredHeight());
    
    connect(document, &SkeletonDocument::motionNameChanged, this, &MotionWidget::updateName);
    connect(document, &SkeletonDocument::motionControlNodesChanged, this, &MotionWidget::updatePreview);
    connect(document, &SkeletonDocument::motionKeyframesChanged, this, &MotionWidget::updatePreview);
}

void MotionWidget::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    m_previewWidget->move((width() - Theme::motionPreviewImageSize) / 2, 0);
}

int MotionWidget::preferredHeight()
{
    return Theme::motionPreviewImageSize;
}

void MotionWidget::reload()
{
    updatePreview();
    updateName();
}

void MotionWidget::updatePreview()
{
    const SkeletonMotion *motion = m_document->findMotion(m_motionId);
    if (!motion) {
        qDebug() << "Motion not found:" << m_motionId;
        return;
    }
    std::vector<std::pair<float, QString>> keyframesForGraphicsView;
    for (const auto &frame: motion->keyframes) {
        QString poseName;
        const SkeletonPose *pose = m_document->findPose(frame.second);
        if (nullptr == pose) {
            qDebug() << "Find pose failed:" << frame.second;
        } else {
            poseName = pose->name;
        }
        keyframesForGraphicsView.push_back({frame.first, poseName});
    }
    m_previewWidget->setControlNodes(motion->controlNodes);
    m_previewWidget->setKeyframes(keyframesForGraphicsView);
}

void MotionWidget::updateName()
{
    const SkeletonMotion *motion = m_document->findMotion(m_motionId);
    if (!motion) {
        qDebug() << "Motion not found:" << m_motionId;
        return;
    }
    m_nameLabel->setText(motion->name);
}

void MotionWidget::updateCheckedState(bool checked)
{
    if (checked)
        setStyleSheet("#MotionFrame {border: 1px solid " + Theme::red.name() + ";}");
    else
        setStyleSheet("#MotionFrame {border: 1px solid transparent;}");
}

InterpolationGraphicsWidget *MotionWidget::previewWidget()
{
    return m_previewWidget;
}

void MotionWidget::mouseDoubleClickEvent(QMouseEvent *event)
{
    QFrame::mouseDoubleClickEvent(event);
    emit modifyMotion(m_motionId);
}
