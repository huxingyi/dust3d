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
#include <QThread>
#include "theme.h"
#include "material_edit_widget.h"
#include "float_number_widget.h"
#include "version.h"
#include "image_forever.h"
#include "document.h"
#include "horizontal_line_widget.h"

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
    m_previewWidget->setNotGraphics(true);
    
    QFont nameFont;
    nameFont.setWeight(QFont::Light);
    nameFont.setBold(false);
    
    QGridLayout *mapLayout = new QGridLayout;
    int row = 0;
    int col = 0;
    for (int i = 1; i < (int)dust3d::TextureType::Count; i++) {
        QVBoxLayout *textureManageLayout = new QVBoxLayout;
    
        MaterialMap item;
        item.forWhat = (dust3d::TextureType)i;
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
        
        QLabel *nameLabel = new QLabel(tr(dust3d::TextureTypeToDispName(item.forWhat).c_str()));
        nameLabel->setFont(nameFont);
        
        QPushButton *eraser = new QPushButton(QChar(fa::eraser));
        Theme::initAwesomeToolButton(eraser);
        
        connect(eraser, &QPushButton::clicked, [=]() {
            m_layers[0].maps[(int)i - 1].imageId = dust3d::Uuid();
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
    
    FloatNumberWidget *tileScaleWidget = new FloatNumberWidget;
    tileScaleWidget->setItemName(tr("Tile Scale"));
    tileScaleWidget->setRange(0.01, 1.0);
    tileScaleWidget->setValue(m_layers[0].tileScale);
    
    m_tileScaleSlider = tileScaleWidget;
    
    connect(tileScaleWidget, &FloatNumberWidget::valueChanged, [=](float value) {
        m_layers[0].tileScale = value;
        emit layersAdjusted();
    });
    
    QPushButton *tileScaleEraser = new QPushButton(QChar(fa::eraser));
    Theme::initAwesomeToolButton(tileScaleEraser);
    
    connect(tileScaleEraser, &QPushButton::clicked, [=]() {
        tileScaleWidget->setValue(1.0);
    });
    
    QHBoxLayout *tileScaleLayout = new QHBoxLayout;
    tileScaleLayout->addWidget(tileScaleEraser);
    tileScaleLayout->addWidget(tileScaleWidget);
    tileScaleLayout->addStretch();

    QHBoxLayout *baseInfoLayout = new QHBoxLayout;
    baseInfoLayout->addWidget(new QLabel(tr("Name")));
    baseInfoLayout->addWidget(m_nameEdit);
    baseInfoLayout->addStretch();
    baseInfoLayout->addWidget(saveButton);

    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->addLayout(paramtersLayout);
    mainLayout->addStretch();
    mainLayout->addWidget(new HorizontalLineWidget());
    mainLayout->addLayout(tileScaleLayout);
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
    m_materialPreviewsGenerator->addMaterial(dust3d::Uuid(), m_layers);
    m_materialPreviewsGenerator->moveToThread(thread);
    connect(thread, &QThread::started, m_materialPreviewsGenerator, &MaterialPreviewsGenerator::process);
    connect(m_materialPreviewsGenerator, &MaterialPreviewsGenerator::finished, this, &MaterialEditWidget::previewReady);
    connect(m_materialPreviewsGenerator, &MaterialPreviewsGenerator::finished, thread, &QThread::quit);
    connect(thread, &QThread::finished, thread, &QThread::deleteLater);
    thread->start();
}

void MaterialEditWidget::previewReady()
{
    m_previewWidget->updateMesh(m_materialPreviewsGenerator->takePreview(dust3d::Uuid()));

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

void MaterialEditWidget::setEditMaterialId(dust3d::Uuid materialId)
{
    if (m_materialId == materialId)
        return;

    m_materialId = materialId;
    updateTitle();
}

void MaterialEditWidget::updateTitle()
{
    if (m_materialId.isNull()) {
        setWindowTitle(applicationTitle(tr("New") + (m_unsaved ? "*" : "")));
        return;
    }
    const Material *material = m_document->findMaterial(m_materialId);
    if (nullptr == material) {
        qDebug() << "Find material failed:" << m_materialId;
        return;
    }
    setWindowTitle(applicationTitle(material->name + (m_unsaved ? "*" : "")));
}

void MaterialEditWidget::setEditMaterialName(QString name)
{
    m_nameEdit->setText(name);
    updateTitle();
}

void MaterialEditWidget::setEditMaterialLayers(std::vector<MaterialLayer> layers)
{
    for (int i = 1; i < (int)dust3d::TextureType::Count; i++) {
        m_layers[0].maps[i - 1].imageId = dust3d::Uuid();
    }
    if (!layers.empty()) {
        for (const auto &layer: layers) {
            m_layers[0].tileScale = layer.tileScale;
            for (const auto &mapItem: layer.maps) {
                int index = (int)mapItem.forWhat - 1;
                if (index >= 0 && index < (int)dust3d::TextureType::Count - 1) {
                    m_layers[0].maps[index].imageId = mapItem.imageId;
                }
            }
        }
        m_tileScaleSlider->setValue(m_layers[0].tileScale);
    }
    for (int i = 1; i < (int)dust3d::TextureType::Count; i++) {
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
        m_materialId = dust3d::Uuid::createUuid();
        emit addMaterial(m_materialId, m_nameEdit->text(), m_layers);
    } else if (m_unsaved) {
        emit renameMaterial(m_materialId, m_nameEdit->text());
        emit setMaterialLayers(m_materialId, m_layers);
    }
    clearUnsaveState();
}
