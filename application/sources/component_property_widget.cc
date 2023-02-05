#include "component_property_widget.h"
#include "cut_face_preview.h"
#include "float_number_widget.h"
#include "flow_layout.h"
#include "image_forever.h"
#include "image_preview_widget.h"
#include "theme.h"
#include <QColorDialog>
#include <QComboBox>
#include <QFileDialog>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QPushButton>
#include <QVBoxLayout>
#include <unordered_set>

ComponentPropertyWidget::ComponentPropertyWidget(Document* document,
    const std::vector<dust3d::Uuid>& componentIds,
    QWidget* parent)
    : QWidget(parent)
    , m_document(document)
    , m_componentIds(componentIds)
{
    preparePartIds();
    m_color = lastColor();

    QComboBox* combineModeSelectBox = nullptr;
    std::set<dust3d::CombineMode> combineModes;
    for (const auto& componentId : m_componentIds) {
        const Document::Component* oneComponent = m_document->findComponent(componentId);
        if (nullptr == oneComponent)
            continue;
        combineModes.insert(oneComponent->combineMode);
    }
    if (!combineModes.empty()) {
        int startIndex = (1 == combineModes.size()) ? 0 : 1;
        combineModeSelectBox = new QComboBox;
        if (0 != startIndex)
            combineModeSelectBox->addItem(tr("Not Change"));
        for (size_t i = 0; i < (size_t)dust3d::CombineMode::Count; ++i) {
            dust3d::CombineMode mode = (dust3d::CombineMode)i;
            combineModeSelectBox->addItem(QString::fromStdString(dust3d::CombineModeToDispName(mode)));
        }
        if (0 != startIndex)
            combineModeSelectBox->setCurrentIndex(0);
        else
            combineModeSelectBox->setCurrentIndex((int)*combineModes.begin());
        connect(combineModeSelectBox, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, [=](int index) {
            if (index < startIndex)
                return;
            for (const auto& componentId : m_componentIds) {
                emit setComponentCombineMode(componentId, (dust3d::CombineMode)(index - startIndex));
            }
            emit groupOperationAdded();
        });
    }

    QHBoxLayout* topLayout = new QHBoxLayout;

    if (!m_componentIds.empty()) {
        QPushButton* colorPreviewArea = new QPushButton;
        colorPreviewArea->setStyleSheet("QPushButton {background-color: " + m_color.name() + "; border-radius: 0;}");
        colorPreviewArea->setFixedSize(Theme::toolIconSize * 1.8, Theme::toolIconSize);

        QPushButton* colorPickerButton = new QPushButton(Theme::awesome()->icon(fa::eyedropper), "");
        Theme::initIconButton(colorPickerButton);
        connect(colorPickerButton, &QPushButton::clicked, this, &ComponentPropertyWidget::showColorDialog);

        topLayout->addWidget(colorPreviewArea);
        topLayout->addWidget(colorPickerButton);
    }

    topLayout->addStretch();
    if (nullptr != combineModeSelectBox)
        topLayout->addWidget(combineModeSelectBox);
    topLayout->setSizeConstraint(QLayout::SetFixedSize);

    QGroupBox* deformGroupBox = nullptr;
    if (nullptr != m_part && dust3d::PartTarget::Model == m_part->target) {
        FloatNumberWidget* thicknessWidget = new FloatNumberWidget;
        thicknessWidget->setItemName(tr("Thickness"));
        thicknessWidget->setRange(0, 2);
        thicknessWidget->setValue(m_part->deformThickness);

        connect(thicknessWidget, &FloatNumberWidget::valueChanged, [=](float value) {
            emit setPartDeformThickness(m_partId, value);
            emit groupOperationAdded();
        });

        FloatNumberWidget* widthWidget = new FloatNumberWidget;
        widthWidget->setItemName(tr("Width"));
        widthWidget->setRange(0, 2);
        widthWidget->setValue(m_part->deformWidth);

        connect(widthWidget, &FloatNumberWidget::valueChanged, [=](float value) {
            emit setPartDeformWidth(m_partId, value);
            emit groupOperationAdded();
        });

        QPushButton* thicknessEraser = new QPushButton(Theme::awesome()->icon(fa::eraser), "");
        Theme::initIconButton(thicknessEraser);

        connect(thicknessEraser, &QPushButton::clicked, [=]() {
            thicknessWidget->setValue(1.0);
            emit groupOperationAdded();
        });

        QPushButton* widthEraser = new QPushButton(Theme::awesome()->icon(fa::eraser), "");
        Theme::initIconButton(widthEraser);

        connect(widthEraser, &QPushButton::clicked, [=]() {
            widthWidget->setValue(1.0);
            emit groupOperationAdded();
        });

        QVBoxLayout* deformLayout = new QVBoxLayout;

        QHBoxLayout* thicknessLayout = new QHBoxLayout;
        QHBoxLayout* widthLayout = new QHBoxLayout;
        thicknessLayout->addWidget(thicknessEraser);
        thicknessLayout->addWidget(thicknessWidget);
        widthLayout->addWidget(widthEraser);
        widthLayout->addWidget(widthWidget);

        QCheckBox* deformUnifyStateBox = new QCheckBox();
        Theme::initCheckbox(deformUnifyStateBox);
        deformUnifyStateBox->setText(tr("Unified"));
        deformUnifyStateBox->setChecked(m_part->deformUnified);

        connect(deformUnifyStateBox, &QCheckBox::stateChanged, this, [=]() {
            emit setPartDeformUnified(m_partId, deformUnifyStateBox->isChecked());
            emit groupOperationAdded();
        });

        QHBoxLayout* deformUnifyLayout = new QHBoxLayout;
        deformUnifyLayout->addStretch();
        deformUnifyLayout->addWidget(deformUnifyStateBox);

        deformLayout->addLayout(thicknessLayout);
        deformLayout->addLayout(widthLayout);
        deformLayout->addLayout(deformUnifyLayout);

        deformGroupBox = new QGroupBox(tr("Deform"));
        deformGroupBox->setLayout(deformLayout);
    }

    QGroupBox* cutFaceGroupBox = nullptr;
    if (nullptr != m_part && dust3d::PartTarget::Model == m_part->target) {
        FlowLayout* cutFaceIconLayout = new FlowLayout(nullptr, 0, 0);
        m_document->collectCutFaceList(m_cutFaceList);
        m_cutFaceButtons.resize(m_cutFaceList.size());
        for (size_t i = 0; i < m_cutFaceList.size(); ++i) {
            QString cutFaceString = m_cutFaceList[i];
            dust3d::CutFace cutFace;
            dust3d::Uuid cutFacePartId(cutFaceString.toUtf8().constData());
            QPushButton* button = new QPushButton;
            button->setIconSize(QSize(Theme::toolIconSize * 0.75, Theme::toolIconSize * 0.75));
            if (cutFacePartId.isNull()) {
                cutFace = dust3d::CutFaceFromString(cutFaceString.toUtf8().constData());
                button->setIcon(QIcon(QPixmap::fromImage(*cutFacePreviewImage(cutFace))));
            } else {
                const Document::Part* part = m_document->findPart(cutFacePartId);
                if (nullptr != part) {
                    const Document::Component* component = m_document->findComponent(part->componentId);
                    if (nullptr != component)
                        button->setIcon(QIcon(component->previewPixmap));
                }
            }
            connect(button, &QPushButton::clicked, [=]() {
                updateCutFaceButtonState(i);
                if (cutFacePartId.isNull())
                    emit setPartCutFace(m_partId, cutFace);
                else
                    emit setPartCutFaceLinkedId(m_partId, cutFacePartId);
                emit groupOperationAdded();
            });
            cutFaceIconLayout->addWidget(button);
            m_cutFaceButtons[i] = button;
        }
        for (size_t i = 0; i < m_cutFaceList.size(); ++i) {
            if (dust3d::CutFace::UserDefined == m_part->cutFace) {
                if (QString(m_part->cutFaceLinkedId.toString().c_str()) == m_cutFaceList[i]) {
                    updateCutFaceButtonState(i);
                    break;
                }
            } else if (i < (int)dust3d::CutFace::UserDefined) {
                if ((size_t)m_part->cutFace == i) {
                    updateCutFaceButtonState(i);
                    break;
                }
            }
        }

        FloatNumberWidget* rotationWidget = new FloatNumberWidget;
        rotationWidget->setItemName(tr("Rotation"));
        rotationWidget->setRange(-1, 1);
        rotationWidget->setValue(m_part->cutRotation);

        connect(rotationWidget, &FloatNumberWidget::valueChanged, [=](float value) {
            emit setPartCutRotation(m_partId, value);
            emit groupOperationAdded();
        });

        QPushButton* rotationEraser = new QPushButton(Theme::awesome()->icon(fa::eraser), "");
        Theme::initIconButton(rotationEraser);

        connect(rotationEraser, &QPushButton::clicked, [=]() {
            rotationWidget->setValue(0.0);
            emit groupOperationAdded();
        });

        QPushButton* rotationMinus5Button = new QPushButton(Theme::awesome()->icon(fa::rotateleft), "");
        Theme::initIconButton(rotationMinus5Button);

        connect(rotationMinus5Button, &QPushButton::clicked, [=]() {
            rotationWidget->setValue(-0.5);
            emit groupOperationAdded();
        });

        QPushButton* rotation5Button = new QPushButton(Theme::awesome()->icon(fa::rotateright), "");
        Theme::initIconButton(rotation5Button);

        connect(rotation5Button, &QPushButton::clicked, [=]() {
            rotationWidget->setValue(0.5);
            emit groupOperationAdded();
        });

        QHBoxLayout* rotationLayout = new QHBoxLayout;
        rotationLayout->addWidget(rotationEraser);
        rotationLayout->addWidget(rotationWidget);
        rotationLayout->addWidget(rotationMinus5Button);
        rotationLayout->addWidget(rotation5Button);

        QCheckBox* subdivStateBox = new QCheckBox();
        Theme::initCheckbox(subdivStateBox);
        subdivStateBox->setText(tr("Subdivided"));
        subdivStateBox->setChecked(m_part->subdived);

        connect(subdivStateBox, &QCheckBox::stateChanged, this, [=]() {
            emit setPartSubdivState(m_partId, subdivStateBox->isChecked());
            emit groupOperationAdded();
        });

        QCheckBox* chamferStateBox = new QCheckBox();
        Theme::initCheckbox(chamferStateBox);
        chamferStateBox->setText(tr("Chamfered"));
        chamferStateBox->setChecked(m_part->chamfered);

        connect(chamferStateBox, &QCheckBox::stateChanged, this, [=]() {
            emit setPartChamferState(m_partId, chamferStateBox->isChecked());
            emit groupOperationAdded();
        });

        QCheckBox* roundEndStateBox = new QCheckBox();
        Theme::initCheckbox(roundEndStateBox);
        roundEndStateBox->setText(tr("Round end"));
        roundEndStateBox->setChecked(m_part->rounded);

        connect(roundEndStateBox, &QCheckBox::stateChanged, this, [=]() {
            emit setPartRoundState(m_partId, roundEndStateBox->isChecked());
            emit groupOperationAdded();
        });

        QHBoxLayout* optionsLayout = new QHBoxLayout;
        optionsLayout->addStretch();
        optionsLayout->addWidget(roundEndStateBox);
        optionsLayout->addWidget(chamferStateBox);
        optionsLayout->addWidget(subdivStateBox);

        QVBoxLayout* cutFaceLayout = new QVBoxLayout;
        cutFaceLayout->addLayout(cutFaceIconLayout);
        cutFaceLayout->addLayout(rotationLayout);
        cutFaceLayout->addLayout(optionsLayout);

        cutFaceGroupBox = new QGroupBox(tr("Cut Face"));
        cutFaceGroupBox->setLayout(cutFaceLayout);
    }

    QGroupBox* smoothGroupBox = nullptr;
    if (!m_componentIds.empty()) {
        FloatNumberWidget* smoothCutoffDegreesWidget = new FloatNumberWidget;
        smoothCutoffDegreesWidget->setItemName(tr("Cutoff"));
        smoothCutoffDegreesWidget->setRange(0.0, 180.0);
        smoothCutoffDegreesWidget->setValue(lastSmoothCutoffDegrees());

        connect(smoothCutoffDegreesWidget, &FloatNumberWidget::valueChanged, [=](float value) {
            for (const auto& partId : m_partIds)
                emit setPartSmoothCutoffDegrees(partId, value);
            emit groupOperationAdded();
        });

        QPushButton* smoothCutoffDegreesEraser = new QPushButton(Theme::awesome()->icon(fa::eraser), "");
        Theme::initIconButton(smoothCutoffDegreesEraser);

        connect(smoothCutoffDegreesEraser, &QPushButton::clicked, [=]() {
            smoothCutoffDegreesWidget->setValue(0.0);
            emit groupOperationAdded();
        });

        QHBoxLayout* smoothCutoffDegreesLayout = new QHBoxLayout;
        smoothCutoffDegreesLayout->addWidget(smoothCutoffDegreesEraser);
        smoothCutoffDegreesLayout->addWidget(smoothCutoffDegreesWidget);

        QVBoxLayout* smoothGroupLayout = new QVBoxLayout;
        smoothGroupLayout->addLayout(smoothCutoffDegreesLayout);

        smoothGroupBox = new QGroupBox(tr("Normal Smooth"));
        smoothGroupBox->setLayout(smoothGroupLayout);
    }

    QGroupBox* colorImageGroupBox = nullptr;
    if (!m_componentIds.empty()) {
        ImagePreviewWidget* colorImagePreviewWidget = new ImagePreviewWidget;
        colorImagePreviewWidget->setFixedSize(Theme::partPreviewImageSize * 2, Theme::partPreviewImageSize * 2);
        auto colorImageId = lastColorImageId();
        const QImage* colorImage = nullptr;
        if (!colorImageId.isNull())
            colorImage = ImageForever::get(colorImageId);
        colorImagePreviewWidget->updateImage(nullptr == colorImage ? QImage() : *colorImage);
        QPushButton* colorImageEraser = new QPushButton(Theme::awesome()->icon(fa::eraser), "");
        Theme::initIconButton(colorImageEraser);

        connect(colorImageEraser, &QPushButton::clicked, [=]() {
            for (const auto& componentId : m_componentIds)
                emit setComponentColorImage(componentId, dust3d::Uuid());
            emit groupOperationAdded();
        });

        QPushButton* colorImagePicker = new QPushButton(Theme::awesome()->icon(fa::image), "");
        Theme::initIconButton(colorImagePicker);

        connect(colorImagePicker, &QPushButton::clicked, [=]() {
            QImage* image = pickImage();
            if (nullptr == image)
                return;
            auto imageId = ImageForever::add(image);
            delete image;
            for (const auto& componentId : m_componentIds)
                emit setComponentColorImage(componentId, imageId);
            emit groupOperationAdded();
        });

        QHBoxLayout* colorImageToolsLayout = new QHBoxLayout;
        colorImageToolsLayout->addWidget(colorImageEraser);
        colorImageToolsLayout->addWidget(colorImagePicker);
        colorImageToolsLayout->addStretch();

        QVBoxLayout* colorImageLayout = new QVBoxLayout;
        colorImageLayout->addWidget(colorImagePreviewWidget);
        colorImageLayout->addLayout(colorImageToolsLayout);

        QHBoxLayout* skinLayout = new QHBoxLayout;
        skinLayout->addLayout(colorImageLayout);
        skinLayout->addStretch();

        colorImageGroupBox = new QGroupBox(tr("Texture Image"));
        colorImageGroupBox->setLayout(skinLayout);
    }

    QHBoxLayout* skinLayout = new QHBoxLayout;
    if (nullptr != colorImageGroupBox)
        skinLayout->addWidget(colorImageGroupBox);

    QGroupBox* stitchingLineGroupBox = nullptr;
    if (!m_componentIds.empty()) {
        QCheckBox* closeStateBox = new QCheckBox();
        Theme::initCheckbox(closeStateBox);
        closeStateBox->setText(tr("Closed"));
        closeStateBox->setChecked(lastClosed());

        connect(closeStateBox, &QCheckBox::stateChanged, this, [=]() {
            bool closed = closeStateBox->isChecked();
            for (const auto& componentId : m_componentIds)
                emit setComponentCloseState(componentId, closed);
            emit groupOperationAdded();
        });

        QHBoxLayout* optionsLayout = new QHBoxLayout;
        optionsLayout->addStretch();
        optionsLayout->addWidget(closeStateBox);

        QVBoxLayout* stitchingLineLayout = new QVBoxLayout;
        stitchingLineLayout->addLayout(optionsLayout);

        stitchingLineGroupBox = new QGroupBox(tr("Stitching Line"));
        stitchingLineGroupBox->setLayout(stitchingLineLayout);
    }

    QVBoxLayout* mainLayout = new QVBoxLayout;
    mainLayout->addLayout(topLayout);
    if (nullptr != deformGroupBox)
        mainLayout->addWidget(deformGroupBox);
    if (nullptr != cutFaceGroupBox)
        mainLayout->addWidget(cutFaceGroupBox);
    if (nullptr != smoothGroupBox)
        mainLayout->addWidget(smoothGroupBox);
    mainLayout->addLayout(skinLayout);
    if (nullptr != stitchingLineGroupBox)
        mainLayout->addWidget(stitchingLineGroupBox);
    mainLayout->setSizeConstraint(QLayout::SetFixedSize);

    connect(this, &ComponentPropertyWidget::setComponentColorState, m_document, &Document::setComponentColorState);
    connect(this, &ComponentPropertyWidget::endColorPicking, m_document, &Document::enableBackgroundBlur);
    connect(this, &ComponentPropertyWidget::beginColorPicking, m_document, &Document::disableBackgroundBlur);
    connect(this, &ComponentPropertyWidget::setPartDeformThickness, m_document, &Document::setPartDeformThickness);
    connect(this, &ComponentPropertyWidget::setPartDeformWidth, m_document, &Document::setPartDeformWidth);
    connect(this, &ComponentPropertyWidget::setPartDeformUnified, m_document, &Document::setPartDeformUnified);
    connect(this, &ComponentPropertyWidget::setPartCutRotation, m_document, &Document::setPartCutRotation);
    connect(this, &ComponentPropertyWidget::setPartSubdivState, m_document, &Document::setPartSubdivState);
    connect(this, &ComponentPropertyWidget::setPartChamferState, m_document, &Document::setPartChamferState);
    connect(this, &ComponentPropertyWidget::setPartRoundState, m_document, &Document::setPartRoundState);
    connect(this, &ComponentPropertyWidget::setComponentColorImage, m_document, &Document::setComponentColorImage);
    connect(this, &ComponentPropertyWidget::setComponentCloseState, m_document, &Document::setComponentCloseState);
    connect(this, &ComponentPropertyWidget::setPartSmoothCutoffDegrees, m_document, &Document::setPartSmoothCutoffDegrees);
    connect(this, &ComponentPropertyWidget::setPartCutFace, m_document, &Document::setPartCutFace);
    connect(this, &ComponentPropertyWidget::setPartCutFaceLinkedId, m_document, &Document::setPartCutFaceLinkedId);
    connect(this, &ComponentPropertyWidget::setComponentCombineMode, m_document, &Document::setComponentCombineMode);
    connect(this, &ComponentPropertyWidget::groupOperationAdded, m_document, &Document::saveSnapshot);

    setLayout(mainLayout);

    setFixedSize(minimumSizeHint());
}

void ComponentPropertyWidget::updateCutFaceButtonState(size_t index)
{
    for (size_t i = 0; i < m_cutFaceList.size(); ++i) {
        auto button = m_cutFaceButtons[i];
        if (i == index) {
            button->setFlat(true);
            button->setEnabled(false);
        } else {
            button->setFlat(false);
            button->setEnabled(true);
        }
    }
}

QImage* ComponentPropertyWidget::pickImage()
{
    QString fileName = QFileDialog::getOpenFileName(this, QString(), QString(),
        tr("Image Files (*.png *.jpg *.bmp)"))
                           .trimmed();
    if (fileName.isEmpty())
        return nullptr;
    QImage* image = new QImage();
    if (!image->load(fileName))
        return nullptr;
    return image;
}

void ComponentPropertyWidget::preparePartIds()
{
    std::unordered_set<dust3d::Uuid> addedPartIdSet;
    for (const auto& componentId : m_componentIds) {
        auto partId = m_document->componentToLinkedPartId(componentId);
        if (partId.isNull())
            continue;
        if (addedPartIdSet.insert(partId).second)
            m_partIds.emplace_back(partId);
    }
    if (1 == m_partIds.size()) {
        m_part = m_document->findPart(m_partIds.front());
        if (nullptr != m_part)
            m_partId = m_partIds.front();
    }
}

QColor ComponentPropertyWidget::lastColor()
{
    QColor color = Qt::white;
    std::map<QString, int> colorMap;
    for (const auto& componentId : m_componentIds) {
        const Document::Component* component = m_document->findComponent(componentId);
        if (nullptr == component)
            continue;
        colorMap[component->color.name()]++;
    }
    if (!colorMap.empty()) {
        color = std::max_element(colorMap.begin(), colorMap.end(),
            [](const std::map<QString, int>::value_type& a, const std::map<QString, int>::value_type& b) {
                return a.second < b.second;
            })->first;
    }
    return color;
}

bool ComponentPropertyWidget::lastClosed()
{
    bool closed = false;
    std::map<bool, int> closeStateMap;
    for (const auto& componentId : m_componentIds) {
        const Document::Component* component = m_document->findComponent(componentId);
        if (nullptr == component)
            continue;
        closeStateMap[component->closed]++;
    }
    if (!closeStateMap.empty()) {
        closed = std::max_element(closeStateMap.begin(), closeStateMap.end(),
            [](const std::map<bool, int>::value_type& a, const std::map<bool, int>::value_type& b) {
                return a.second < b.second;
            })->first;
    }
    return closed;
}

dust3d::Uuid ComponentPropertyWidget::lastColorImageId()
{
    dust3d::Uuid colorImageId;
    std::map<dust3d::Uuid, int> colorImageIdMap;
    for (const auto& componentId : m_componentIds) {
        const Document::Component* component = m_document->findComponent(componentId);
        if (nullptr == component)
            continue;
        if (component->colorImageId.isNull())
            continue;
        colorImageIdMap[component->colorImageId]++;
    }
    if (!colorImageIdMap.empty()) {
        colorImageId = std::max_element(colorImageIdMap.begin(), colorImageIdMap.end(),
            [](const std::map<dust3d::Uuid, int>::value_type& a, const std::map<dust3d::Uuid, int>::value_type& b) {
                return a.second < b.second;
            })->first;
    }
    return colorImageId;
}

float ComponentPropertyWidget::lastSmoothCutoffDegrees()
{
    float smoothCutoffDegrees = 0.0;
    std::map<std::string, int> degreesMap;
    for (const auto& partId : m_partIds) {
        const Document::Part* part = m_document->findPart(partId);
        if (nullptr == part)
            continue;
        degreesMap[std::to_string(part->smoothCutoffDegrees)]++;
    }
    if (!degreesMap.empty()) {
        smoothCutoffDegrees = dust3d::String::toFloat(std::max_element(degreesMap.begin(), degreesMap.end(),
            [](const std::map<std::string, int>::value_type& a, const std::map<std::string, int>::value_type& b) {
                return a.second < b.second;
            })->first);
    }
    return smoothCutoffDegrees;
}

void ComponentPropertyWidget::showColorDialog()
{
    emit beginColorPicking();
    QColor color = QColorDialog::getColor(m_color, this);
    emit endColorPicking();
    if (!color.isValid())
        return;

    for (const auto& componentId : m_componentIds) {
        emit setComponentColorState(componentId, true, color);
    }
    emit groupOperationAdded();
}
