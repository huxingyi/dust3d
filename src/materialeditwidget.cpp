#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QFormLayout>
#include <QGridLayout>
#include <QMenu>
#include <QWidgetAction>
#include <QLineEdit>
#include <QMessageBox>
#include <QFileDialog>
#include <QLabel>
#include "theme.h"
#include "materialeditwidget.h"
#include "floatnumberwidget.h"
#include "version.h"
#include "imageforever.h"
#include "util.h"

ImagePreviewWidget *MaterialEditWidget::createMapButton()
{
    ImagePreviewWidget *mapButton = new ImagePreviewWidget;
    mapButton->setFixedSize(Theme::partPreviewImageSize * 2, Theme::partPreviewImageSize * 2);
    updateMapButtonBackground(mapButton, nullptr);
    return mapButton;
}

QImage *MaterialEditWidget::pickImage()
{
    QString fileName = QFileDialog::getOpenFileName(this, QString(), QString(),
        tr("Image Files (*.png *.jpg *.bmp)")).trimmed();
    if (fileName.isEmpty())
        return nullptr;
    QImage *image = new QImage();
    if (!image->load(fileName))
        return nullptr;
    return image;
}

MaterialEditWidget::MaterialEditWidget(const Document *document, QWidget *parent) :
    QDialog(parent),
    m_document(document)
{
    setWindowFlags(windowFlags() | Qt::WindowStaysOnTopHint);
    
    m_layers.resize(1);

    m_previewWidget = new ModelWidget(this);
    m_previewWidget->setMinimumSize(128, 128);
    m_previewWidget->resize(512, 512);
    m_previewWidget->move(-128, -128);
    
    QFont nameFont;
    nameFont.setWeight(QFont::Light);
    //nameFont.setPixelSize(9);
    nameFont.setBold(false);
    
    QGridLayout *mapLayout = new QGridLayout;
    int row = 0;
    int col = 0;
    for (int i = 1; i < (int)TextureType::Count; i++) {
        QVBoxLayout *textureManageLayout = new QVBoxLayout;
    
        MaterialMap item;
        item.forWhat = (TextureType)i;
        m_layers[0].maps.push_back(item);
        
        ImagePreviewWidget *imageButton = createMapButton();
        connect(imageButton, &ImagePreviewWidget::clicked, [=]() {
            QImage *image = pickImage();
            if (nullptr == image)
                return;
            m_layers[0].maps[(int)i - 1].imageId = ImageForever::add(image);
            updateMapButtonBackground(imageButton, image);
            delete image;
            emit layersAdjusted();
        });
        
        QLabel *nameLabel = new QLabel(TextureTypeToDispName(item.forWhat));
        nameLabel->setFont(nameFont);
        
        QPushButton *eraser = new QPushButton(QChar(fa::eraser));
        Theme::initAwesomeToolButton(eraser);
        
        connect(eraser, &QPushButton::clicked, [=]() {
            m_layers[0].maps[(int)i - 1].imageId = QUuid();
            updateMapButtonBackground(imageButton, nullptr);
            emit layersAdjusted();
        });
        
        QHBoxLayout *textureTitleLayout = new QHBoxLayout;
        textureTitleLayout->addWidget(eraser);
        textureTitleLayout->addWidget(nameLabel);
        textureTitleLayout->addStretch();
        
        textureManageLayout->addWidget(imageButton);
        textureManageLayout->addLayout(textureTitleLayout);
        m_textureMapButtons[i - 1] = imageButton;
        
        mapLayout->addLayout(textureManageLayout, row, col++);
        if (col == 2) {
            col = 0;
            row++;
        }
    }
    
    QVBoxLayout *rightLayout = new QVBoxLayout;
    rightLayout->addStretch();
    rightLayout->addLayout(mapLayout);
    rightLayout->addStretch();

    QHBoxLayout *paramtersLayout = new QHBoxLayout;
    paramtersLayout->setContentsMargins(256, 0, 0, 0);
    paramtersLayout->addStretch();
    paramtersLayout->addLayout(rightLayout);

    m_nameEdit = new QLineEdit;
    connect(m_nameEdit, &QLineEdit::textChanged, this, [=]() {
        m_unsaved = true;
        updateTitle();
    });
    QPushButton *saveButton = new QPushButton(tr("Save"));
    connect(saveButton, &QPushButton::clicked, this, &MaterialEditWidget::save);
    saveButton->setDefault(true);

    QHBoxLayout *baseInfoLayout = new QHBoxLayout;
    baseInfoLayout->addWidget(new QLabel(tr("Name")));
    baseInfoLayout->addWidget(m_nameEdit);
    baseInfoLayout->addStretch();
    baseInfoLayout->addWidget(saveButton);

    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->addLayout(paramtersLayout);
    mainLayout->addStretch();
    mainLayout->addWidget(Theme::createHorizontalLineWidget());
    mainLayout->addLayout(baseInfoLayout);

    setLayout(mainLayout);

    connect(this, &MaterialEditWidget::layersAdjusted, this, &MaterialEditWidget::updatePreview);
    connect(this, &MaterialEditWidget::layersAdjusted, [=]() {
        m_unsaved = true;
        updateTitle();
    });
    connect(this, &MaterialEditWidget::addMaterial, document, &Document::addMaterial);
    connect(this, &MaterialEditWidget::renameMaterial, document, &Document::renameMaterial);
    connect(this, &MaterialEditWidget::setMaterialLayers, document, &Document::setMaterialLayers);

    updatePreview();
    updateTitle();
}

void MaterialEditWidget::updateMapButtonBackground(ImagePreviewWidget *button, const QImage *image)
{
    if (nullptr == image)
        button->updateImage(QImage());
    else
        button->updateImage(*image);
}

void MaterialEditWidget::reject()
{
    close();
}

void MaterialEditWidget::closeEvent(QCloseEvent *event)
{
    if (m_unsaved && !m_closed) {
        QMessageBox::StandardButton answer = QMessageBox::question(this,
            APP_NAME,
            tr("Do you really want to close while there are unsaved changes?"),
            QMessageBox::Yes | QMessageBox::No,
            QMessageBox::No);
        if (answer != QMessageBox::Yes) {
            event->ignore();
            return;
        }
    }
    m_closed = true;
    hide();
    if (nullptr != m_materialPreviewsGenerator) {
        event->ignore();
        return;
    }
    event->accept();
}

QSize MaterialEditWidget::sizeHint() const
{
    return QSize(0, 200);
}

MaterialEditWidget::~MaterialEditWidget()
{
    Q_ASSERT(nullptr == m_materialPreviewsGenerator);
}

void MaterialEditWidget::updatePreview()
{
    if (m_closed)
        return;

    if (nullptr != m_materialPreviewsGenerator) {
        m_isPreviewDirty = true;
        return;
    }

    m_isPreviewDirty = false;

    qDebug() << "Material preview generating..";

    QThread *thread = new QThread;
    m_materialPreviewsGenerator = new MaterialPreviewsGenerator();
    m_materialPreviewsGenerator->addMaterial(QUuid(), m_layers);
    m_materialPreviewsGenerator->moveToThread(thread);
    connect(thread, &QThread::started, m_materialPreviewsGenerator, &MaterialPreviewsGenerator::process);
    connect(m_materialPreviewsGenerator, &MaterialPreviewsGenerator::finished, this, &MaterialEditWidget::previewReady);
    connect(m_materialPreviewsGenerator, &MaterialPreviewsGenerator::finished, thread, &QThread::quit);
    connect(thread, &QThread::finished, thread, &QThread::deleteLater);
    thread->start();
}

void MaterialEditWidget::previewReady()
{
    m_previewWidget->updateMesh(m_materialPreviewsGenerator->takePreview(QUuid()));

    delete m_materialPreviewsGenerator;
    m_materialPreviewsGenerator = nullptr;

    qDebug() << "Material preview generation done";

    if (m_closed) {
        close();
        return;
    }

    if (m_isPreviewDirty)
        updatePreview();
}

void MaterialEditWidget::setEditMaterialId(QUuid materialId)
{
    if (m_materialId == materialId)
        return;

    m_materialId = materialId;
    updateTitle();
}

void MaterialEditWidget::updateTitle()
{
    if (m_materialId.isNull()) {
        setWindowTitle(unifiedWindowTitle(tr("New") + (m_unsaved ? "*" : "")));
        return;
    }
    const Material *material = m_document->findMaterial(m_materialId);
    if (nullptr == material) {
        qDebug() << "Find material failed:" << m_materialId;
        return;
    }
    setWindowTitle(unifiedWindowTitle(material->name + (m_unsaved ? "*" : "")));
}

void MaterialEditWidget::setEditMaterialName(QString name)
{
    m_nameEdit->setText(name);
    updateTitle();
}

void MaterialEditWidget::setEditMaterialLayers(std::vector<MaterialLayer> layers)
{
    for (int i = 1; i < (int)TextureType::Count; i++) {
        m_layers[0].maps[i - 1].imageId = QUuid();
    }
    if (!layers.empty()) {
        for (const auto &layer: layers) {
            for (const auto &mapItem: layer.maps) {
                int index = (int)mapItem.forWhat - 1;
                if (index >= 0 && index < (int)TextureType::Count - 1) {
                    m_layers[0].maps[index].imageId = mapItem.imageId;
                }
            }
        }
    }
    for (int i = 1; i < (int)TextureType::Count; i++) {
        updateMapButtonBackground(m_textureMapButtons[i - 1], ImageForever::get(m_layers[0].maps[i - 1].imageId));
    }
    updatePreview();
}

void MaterialEditWidget::clearUnsaveState()
{
    m_unsaved = false;
    updateTitle();
}

void MaterialEditWidget::save()
{
    if (m_materialId.isNull()) {
        m_materialId = QUuid::createUuid();
        emit addMaterial(m_materialId, m_nameEdit->text(), m_layers);
    } else if (m_unsaved) {
        emit renameMaterial(m_materialId, m_nameEdit->text());
        emit setMaterialLayers(m_materialId, m_layers);
    }
    clearUnsaveState();
}
