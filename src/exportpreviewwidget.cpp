#include <QGridLayout>
#include <QSizePolicy>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QComboBox>
#include "exportpreviewwidget.h"
#include "aboutwidget.h"
#include "version.h"
#include "theme.h"
#include "util.h"

ExportPreviewWidget::ExportPreviewWidget(Document *document, QWidget *parent) :
    QDialog(parent),
    m_document(document),
    m_colorPreviewLabel(nullptr),
    m_spinnerWidget(nullptr)
{
    setWindowFlags(windowFlags() | Qt::WindowStaysOnTopHint);
    
    QHBoxLayout *toolButtonLayout = new QHBoxLayout;
    toolButtonLayout->setSpacing(0);
    //toolButtonLayout->setContentsMargins(5, 10, 4, 0);

    m_colorPreviewLabel = new QLabel;
    m_colorPreviewLabel->setMinimumSize(128, 128);
    m_colorPreviewLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    
    m_normalPreviewLabel = new QLabel;
    m_normalPreviewLabel->setMinimumSize(64, 64);
    m_normalPreviewLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    
    m_metalnessRoughnessAmbientOcclusionPreviewLabel = new QLabel;
    m_metalnessRoughnessAmbientOcclusionPreviewLabel->setMinimumSize(64, 64);
    m_metalnessRoughnessAmbientOcclusionPreviewLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    
    //QPushButton *regenerateButton = new QPushButton(QChar(fa::recycle));
    //initAwesomeButton(regenerateButton);
    QPushButton *regenerateButton = new QPushButton(tr("Regenerate"));
    connect(this, &ExportPreviewWidget::regenerate, this, &ExportPreviewWidget::checkSpinner);
    connect(regenerateButton, &QPushButton::clicked, this, &ExportPreviewWidget::regenerate);
    
    QComboBox *exportFormatSelectBox = new QComboBox;
    exportFormatSelectBox->addItem(tr(".glb"));
    exportFormatSelectBox->addItem(tr(".fbx"));
    exportFormatSelectBox->setCurrentIndex(0);
    
    m_saveButton = new QPushButton(tr("Save"));
    connect(m_saveButton, &QPushButton::clicked, this, [=]() {
        auto currentIndex = exportFormatSelectBox->currentIndex();
        if (0 == currentIndex) {
            emit saveAsGlb();
        } else if (1 == currentIndex) {
            emit saveAsFbx();
        }
    });
    m_saveButton->hide();
    m_saveButton->setDefault(true);
    
    toolButtonLayout->addWidget(exportFormatSelectBox);
    toolButtonLayout->addWidget(regenerateButton);
    toolButtonLayout->addStretch();
    toolButtonLayout->addWidget(m_saveButton);
    
    QGridLayout *containerLayout = new QGridLayout;
    containerLayout->setSpacing(0);
    containerLayout->setContentsMargins(0, 0, 0, 0);
    containerLayout->addWidget(m_colorPreviewLabel, 0, 0);
    containerLayout->addWidget(m_normalPreviewLabel, 0, 1);
    containerLayout->addWidget(m_metalnessRoughnessAmbientOcclusionPreviewLabel, 0, 2);
    //containerLayout->setRowStretch(0, 1);
    //containerLayout->setColumnStretch(0, 1);
    
    //m_textureRenderWidget = new ModelWidget;
    //m_textureRenderWidget->setMinimumSize(128, 128);
    //m_textureRenderWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    
    //QVBoxLayout *renderLayout = new QVBoxLayout;
    //renderLayout->setSpacing(0);
    //renderLayout->setContentsMargins(20, 0, 20, 0);
    //renderLayout->addWidget(m_textureRenderWidget);
    
    QWidget *hrLightWidget = Theme::createHorizontalLineWidget();
    
    QHBoxLayout *topLayout = new QHBoxLayout;
    topLayout->setSpacing(0);
    topLayout->setContentsMargins(0, 0, 0, 0);
    topLayout->addLayout(containerLayout);
    //topLayout->addLayout(renderLayout);
    
    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->addLayout(topLayout);
    mainLayout->addWidget(hrLightWidget);
    mainLayout->addLayout(toolButtonLayout);

    setLayout(mainLayout);
    setMinimumSize(256, 256);
    setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
    
    m_spinnerWidget = new WaitingSpinnerWidget(m_colorPreviewLabel);
    m_spinnerWidget->setColor(Theme::white);
    m_spinnerWidget->setInnerRadius(Theme::miniIconFontSize / 4);
    m_spinnerWidget->setNumberOfLines(12);
    m_spinnerWidget->hide();
    
    setWindowTitle(unifiedWindowTitle(tr("Export")));
    
    emit updateTexturePreview();
}

void ExportPreviewWidget::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    resizePreviewImages();
}

void ExportPreviewWidget::resizePreviewImages()
{
    if (m_colorImage.isNull()) {
        m_colorPreviewLabel->hide();
    } else {
        QPixmap pixmap = QPixmap::fromImage(m_colorImage);
        m_colorPreviewLabel->setPixmap(pixmap.scaled(m_colorPreviewLabel->width(), m_colorPreviewLabel->height(), Qt::KeepAspectRatio));
        m_colorPreviewLabel->show();
    }
    
    if (m_normalImage.isNull()) {
        m_normalPreviewLabel->hide();
    } else {
        QPixmap pixmap = QPixmap::fromImage(m_normalImage);
        m_normalPreviewLabel->setPixmap(pixmap.scaled(m_normalPreviewLabel->width(), m_normalPreviewLabel->height(), Qt::KeepAspectRatio));
        m_normalPreviewLabel->show();
    }
    
    if (m_metalnessRoughnessAmbientOcclusionImage.isNull()) {
        m_metalnessRoughnessAmbientOcclusionPreviewLabel->hide();
    } else {
        QPixmap pixmap = QPixmap::fromImage(m_metalnessRoughnessAmbientOcclusionImage);
        m_metalnessRoughnessAmbientOcclusionPreviewLabel->setPixmap(pixmap.scaled(m_metalnessRoughnessAmbientOcclusionPreviewLabel->width(), m_metalnessRoughnessAmbientOcclusionPreviewLabel->height(), Qt::KeepAspectRatio));
        m_metalnessRoughnessAmbientOcclusionPreviewLabel->show();
    }
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
    if (m_document->isPostProcessResultObsolete()) {
        m_document->postProcess();
    }
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
        m_colorImage = *m_document->textureGuideImage;
    else
        m_colorImage = QImage();
    if (m_document->textureNormalImage)
        m_normalImage = *m_document->textureNormalImage;
    else
        m_normalImage = QImage();
    if (m_document->textureMetalnessRoughnessAmbientOcclusionImage)
        m_metalnessRoughnessAmbientOcclusionImage = *m_document->textureMetalnessRoughnessAmbientOcclusionImage;
    else
        m_metalnessRoughnessAmbientOcclusionImage = QImage();
    //m_textureRenderWidget->updateMesh(m_document->takeResultTextureMesh());
    resizePreviewImages();
    checkSpinner();
}

