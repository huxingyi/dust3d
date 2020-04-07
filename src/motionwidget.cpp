#include <QVBoxLayout>
#include "motionwidget.h"

MotionWidget::MotionWidget(const Document *document, QUuid motionId) :
    m_motionId(motionId),
    m_document(document)
{
    setObjectName("MotionFrame");

    m_previewWidget = new ModelWidget(this);
    m_previewWidget->setAttribute(Qt::WA_TransparentForMouseEvents);
    m_previewWidget->setFixedSize(Theme::motionPreviewImageSize, Theme::motionPreviewImageSize);
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
    
    setFixedSize(Theme::motionPreviewImageSize, MotionWidget::preferredHeight());
    
    connect(document, &Document::motionNameChanged, this, [=](QUuid motionId) {
        if (motionId != m_motionId)
            return;
        updateName();
    });
    connect(document, &Document::motionPreviewChanged, this, [=](QUuid motionId) {
        if (motionId != m_motionId)
            return;
        updatePreview();
    });
}

void MotionWidget::setCornerButtonVisible(bool visible)
{
    if (nullptr == m_cornerButton) {
        m_cornerButton = new QPushButton(this);
        m_cornerButton->move(Theme::motionPreviewImageSize - Theme::miniIconSize - 2, 2);
        Theme::initAwesomeMiniButton(m_cornerButton);
        m_cornerButton->setText(QChar(fa::plussquare));
        connect(m_cornerButton, &QPushButton::clicked, this, [=]() {
            emit cornerButtonClicked(m_motionId);
        });
    }
    m_cornerButton->setVisible(visible);
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
    const Motion *motion = m_document->findMotion(m_motionId);
    if (!motion) {
        qDebug() << "Motion not found:" << m_motionId;
        return;
    }
    Model *previewMesh = motion->takePreviewMesh();
    m_previewWidget->updateMesh(previewMesh);
}

void MotionWidget::updateName()
{
    const Motion *motion = m_document->findMotion(m_motionId);
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

ModelWidget *MotionWidget::previewWidget()
{
    return m_previewWidget;
}

void MotionWidget::mouseDoubleClickEvent(QMouseEvent *event)
{
    QFrame::mouseDoubleClickEvent(event);
    emit modifyMotion(m_motionId);
}
