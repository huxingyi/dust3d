#include <QGridLayout>
#include <QSizePolicy>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include "textureguidewidget.h"
#include "aboutwidget.h"
#include "version.h"
#include "theme.h"

TextureGuideWidget::TextureGuideWidget() :
    m_previewLabel(nullptr)
{
    QVBoxLayout *toolButtonLayout = new QVBoxLayout;
    toolButtonLayout->setSpacing(0);
    toolButtonLayout->setContentsMargins(5, 10, 4, 0);

    m_previewLabel = new QLabel;
    m_previewLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    
    QPushButton *regenerateButton = new QPushButton(QChar(fa::recycle));
    initAwesomeButton(regenerateButton);
    connect(regenerateButton, &QPushButton::clicked, this, &TextureGuideWidget::regenerate);
    
    toolButtonLayout->addWidget(regenerateButton);
    toolButtonLayout->addStretch();
    
    QGridLayout *containerLayout = new QGridLayout;
    containerLayout->setSpacing(0);
    containerLayout->setContentsMargins(0, 0, 0, 0);
    containerLayout->addWidget(m_previewLabel);
    containerLayout->setRowStretch(0, 1);
    containerLayout->setColumnStretch(0, 1);
    
    QHBoxLayout *mainLayout = new QHBoxLayout;
    mainLayout->setSpacing(0);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->addLayout(toolButtonLayout);
    mainLayout->addLayout(containerLayout);

    setLayout(mainLayout);
    setMinimumSize(128, 128);
    setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
    
    setWindowTitle(APP_NAME);
}

void TextureGuideWidget::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    resizeImage();
}

void TextureGuideWidget::resizeImage()
{
    QPixmap pixmap = QPixmap::fromImage(m_image);
    m_previewLabel->setPixmap(pixmap.scaled(m_previewLabel->width(), m_previewLabel->height(), Qt::KeepAspectRatio));
}

void TextureGuideWidget::updateGuideImage(const QImage &image)
{
    m_image = image;
    resizeImage();
}

void TextureGuideWidget::initAwesomeButton(QPushButton *button)
{
    button->setFont(Theme::awesome()->font(Theme::toolIconFontSize));
    button->setFixedSize(Theme::toolIconSize, Theme::toolIconSize);
    button->setStyleSheet("QPushButton {color: #f7d9c8}");
    button->setFocusPolicy(Qt::NoFocus);
}


