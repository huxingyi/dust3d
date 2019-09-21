#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include "motionclipwidget.h"
#include "posewidget.h"
#include "motionwidget.h"

MotionClipWidget::MotionClipWidget(const Document *document, QWidget *parent) :
    QFrame(parent),
    m_document(document)
{
    setObjectName("MotionClipFrame");
}

QSize MotionClipWidget::preferredSize()
{
    int preferredWidth = 0;
    switch (m_clip.clipType) {
    case MotionClipType::Motion:
        preferredWidth = Theme::motionPreviewImageSize;
        break;
    case MotionClipType::Pose:
        preferredWidth = Theme::posePreviewImageSize;
        break;
    case MotionClipType::ProceduralAnimation:
        {
            QPushButton testButton(ProceduralAnimationToDispName(m_clip.proceduralAnimation));
            preferredWidth = testButton.sizeHint().width();
        }
        break;
    case MotionClipType::Interpolation:
        preferredWidth = Theme::normalButtonSize;
        break;
    default:
        break;
    }
    return QSize(preferredWidth, maxSize().height());
}

QSize MotionClipWidget::maxSize()
{
    auto maxWidth = std::max(Theme::posePreviewImageSize, Theme::motionPreviewImageSize);
    auto maxHeight = std::max(PoseWidget::preferredHeight(), MotionWidget::preferredHeight());
    return QSize(maxWidth, maxHeight);
}

void MotionClipWidget::setClip(MotionClip clip)
{
    m_clip = clip;
    reload();
}

void MotionClipWidget::reload()
{
    if (nullptr != m_reloadToWidget)
        m_reloadToWidget->deleteLater();
    m_reloadToWidget = new QWidget(this);
    m_reloadToWidget->setContentsMargins(1, 0, 0, 0);
    m_reloadToWidget->setFixedSize(preferredSize());
    m_reloadToWidget->show();
    QVBoxLayout *layout = new QVBoxLayout;
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    layout->setSizeConstraint(QLayout::SetMinimumSize);
    layout->addStretch();
    
    switch (m_clip.clipType) {
    case MotionClipType::Motion:
        {
            MotionWidget *motionWidget = new MotionWidget(m_document, m_clip.linkToId);
            motionWidget->reload();
            layout->addWidget(motionWidget);
        }
        break;
    case MotionClipType::Pose:
        {
            PoseWidget *poseWidget = new PoseWidget(m_document, m_clip.linkToId);
            poseWidget->reload();
            layout->addWidget(poseWidget);
        }
        break;
    case MotionClipType::ProceduralAnimation:
        {
            QPushButton *proceduralAnimationButton = new QPushButton(ProceduralAnimationToDispName(m_clip.proceduralAnimation));
            proceduralAnimationButton->setFocusPolicy(Qt::NoFocus);
            layout->addWidget(proceduralAnimationButton);
        }
        break;
    case MotionClipType::Interpolation:
        {
            QHBoxLayout *interpolationButtonLayout = new QHBoxLayout;
            QPushButton *interpolationButton = new QPushButton(QChar(fa::arrowsh));
            Theme::initAwesomeButton(interpolationButton);
            interpolationButtonLayout->setContentsMargins(0, 0, 0, 0);
            interpolationButtonLayout->setSpacing(0);
            interpolationButtonLayout->addStretch();
            interpolationButtonLayout->addWidget(interpolationButton);
            interpolationButtonLayout->addStretch();
            
            layout->addLayout(interpolationButtonLayout);
            
            connect(interpolationButton, &QPushButton::clicked, this, &MotionClipWidget::modifyInterpolation);
            
            QHBoxLayout *interpolationDurationLayout = new QHBoxLayout;
            QLabel *durationLabel = new QLabel;
            durationLabel->setText(QString::number(m_clip.duration) + "s");
            interpolationDurationLayout->setContentsMargins(0, 0, 0, 0);
            interpolationDurationLayout->setSpacing(0);
            interpolationDurationLayout->addStretch();
            interpolationDurationLayout->addWidget(durationLabel);
            interpolationDurationLayout->addStretch();
            
            layout->addLayout(interpolationDurationLayout);
        }
        break;
    default:
        break;
    }
    
    layout->addStretch();
    m_reloadToWidget->setLayout(layout);
}

void MotionClipWidget::updateCheckedState(bool checked)
{
    if (checked)
        setStyleSheet("#MotionClipFrame {border: 1px solid " + Theme::red.name() + ";}");
    else
        setStyleSheet("#MotionClipFrame {border: 1px solid transparent;}");
}

