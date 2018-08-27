#include <QGridLayout>
#include <QSizePolicy>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include "exportpreviewwidget.h"
#include "aboutwidget.h"
#include "version.h"
#include "theme.h"

ExportPreviewWidget::ExportPreviewWidget(SkeletonDocument *document, QWidget *parent) :
    QWidget(parent),
    m_document(document),
    m_previewLabel(nullptr),
    m_spinnerWidget(nullptr)
{
    QVBoxLayout *toolButtonLayout = new QVBoxLayout;
    toolButtonLayout->setSpacing(0);
    toolButtonLayout->setContentsMargins(5, 10, 4, 0);

    m_previewLabel = new QLabel;
    m_previewLabel->setMinimumSize(128, 128);
    m_previewLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    
    QPushButton *regenerateButton = new QPushButton(QChar(fa::recycle));
    initAwesomeButton(regenerateButton);
    connect(this, &ExportPreviewWidget::regenerate, this, &ExportPreviewWidget::checkSpinner);
    connect(regenerateButton, &QPushButton::clicked, this, &ExportPreviewWidget::regenerate);
    
    m_saveButton = new QPushButton(QChar(fa::save));
    initAwesomeButton(m_saveButton);
    connect(m_saveButton, &QPushButton::clicked, this, &ExportPreviewWidget::save);
    m_saveButton->hide();
    
    toolButtonLayout->addWidget(regenerateButton);
    toolButtonLayout->addWidget(m_saveButton);
    toolButtonLayout->addStretch();
    
    QGridLayout *containerLayout = new QGridLayout;
    containerLayout->setSpacing(0);
    containerLayout->setContentsMargins(0, 0, 0, 0);
    containerLayout->addWidget(m_previewLabel);
    containerLayout->setRowStretch(0, 1);
    containerLayout->setColumnStretch(0, 1);
    
    m_textureRenderWidget = new ModelWidget;
    m_textureRenderWidget->setMinimumSize(128, 128);
    m_textureRenderWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    
    QVBoxLayout *renderLayout = new QVBoxLayout;
    renderLayout->setSpacing(0);
    renderLayout->setContentsMargins(20, 0, 20, 0);
    renderLayout->addWidget(m_textureRenderWidget);
    
    QHBoxLayout *mainLayout = new QHBoxLayout;
    mainLayout->setSpacing(0);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->addLayout(toolButtonLayout);
    mainLayout->addLayout(containerLayout);
    mainLayout->addLayout(renderLayout);

    setLayout(mainLayout);
    setMinimumSize(256, 256);
    setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
    
    m_spinnerWidget = new WaitingSpinnerWidget(m_previewLabel);
    m_spinnerWidget->setColor(Theme::white);
    m_spinnerWidget->setInnerRadius(Theme::miniIconFontSize / 4);
    m_spinnerWidget->setNumberOfLines(12);
    m_spinnerWidget->hide();
    
    setWindowTitle(APP_NAME);
    
    emit updateTexturePreview();
}

void ExportPreviewWidget::updateTexturePreviewImage(const QImage &image)
{
    m_previewImage = image;
    resizePreviewImage();
    checkSpinner();
}

void ExportPreviewWidget::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    resizePreviewImage();
}

void ExportPreviewWidget::resizePreviewImage()
{
    QPixmap pixmap = QPixmap::fromImage(m_previewImage);
    m_previewLabel->setPixmap(pixmap.scaled(m_previewLabel->width(), m_previewLabel->height(), Qt::KeepAspectRatio));
}

void ExportPreviewWidget::initAwesomeButton(QPushButton *button)
{
    button->setFont(Theme::awesome()->font(Theme::toolIconFontSize));
    button->setFixedSize(Theme::toolIconSize, Theme::toolIconSize);
    button->setStyleSheet("QPushButton {color: #f7d9c8}");
    button->setFocusPolicy(Qt::NoFocus);
}

void ExportPreviewWidget::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);
    checkSpinner();
}

void ExportPreviewWidget::checkSpinner()
{
    if (m_document->isExportReady()) {
        m_spinnerWidget->stop();
        m_spinnerWidget->hide();
        m_saveButton->show();
    } else {
        m_spinnerWidget->start();
        m_spinnerWidget->show();
        m_saveButton->hide();
    }
}

void ExportPreviewWidget::updateTexturePreview()
{
    if (m_document->textureGuideImage)
        updateTexturePreviewImage(*m_document->textureGuideImage);
    m_textureRenderWidget->updateMesh(m_document->takeResultTextureMesh());
}

