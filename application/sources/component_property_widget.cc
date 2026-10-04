#include "component_property_widget.h"
#include "cut_face_preview.h"
#include "float_number_widget.h"
#include "flow_layout.h"
#include "glb_forever.h"
#include "image_forever.h"
#include "image_preview_widget.h"
#include "int_number_widget.h"
#include "theme.h"
#include <QColorDialog>
#include <QComboBox>
#include <QFile>
#include <QFileDialog>
#include <QGraphicsOpacityEffect>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QPointer>
#include <QPushButton>
#include <QSpinBox>
#include <QVBoxLayout>
#include <QtGlobal>
#include <set>
#include <unordered_set>

ComponentPropertyWidget::ComponentPropertyWidget(Document* document,
    const std::vector<dust3d::Uuid>& componentIds,
    QWidget* parent)
    : QWidget(parent)
    , m_document(document)
    , m_componentIds(componentIds)
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 9, 0)
    const auto checkboxStateChangedSignal = &QCheckBox::checkStateChanged;
#else
    const auto checkboxStateChangedSignal = &QCheckBox::stateChanged;
#endif

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
        combineModeSelectBox->setSizeAdjustPolicy(QComboBox::AdjustToContents);
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
    if (nullptr != combineModeSelectBox && !(nullptr != m_part && dust3d::PartTarget::CutFace == m_part->target))
        topLayout->addWidget(combineModeSelectBox);
    topLayout->setSizeConstraint(QLayout::SetFixedSize);

    QGroupBox* partRoleGroupBox = nullptr;
    if (nullptr != m_part) {
        QComboBox* partRoleComboBox = new QComboBox;
        partRoleComboBox->setSizeAdjustPolicy(QComboBox::AdjustToContents);
        partRoleComboBox->addItem(tr("Model"), static_cast<int>(dust3d::PartTarget::Model));
        partRoleComboBox->addItem(tr("Cut Face"), static_cast<int>(dust3d::PartTarget::CutFace));
        partRoleComboBox->addItem(tr("Stitching Line"), static_cast<int>(dust3d::PartTarget::StitchingLine));
        partRoleComboBox->addItem(tr("Stitching Loop"), static_cast<int>(dust3d::PartTarget::StitchingLoop));
        partRoleComboBox->addItem(tr("Imported Model"), static_cast<int>(dust3d::PartTarget::ImportedModel));
        partRoleComboBox->setCurrentIndex(static_cast<int>(m_part->target));
        connect(partRoleComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [=](int index) {
            emit setPartTarget(m_partId, static_cast<dust3d::PartTarget>(partRoleComboBox->itemData(index).toInt()));
            emit groupOperationAdded();
            QWidget* widget = this;
            while (nullptr != widget) {
                QMenu* menu = qobject_cast<QMenu*>(widget);
                if (nullptr != menu) {
                    menu->close();
                    break;
                }
                widget = widget->parentWidget();
            }
        });
        QHBoxLayout* partRoleLayout = new QHBoxLayout;
        partRoleLayout->addWidget(partRoleComboBox);
        partRoleLayout->addStretch();

        QVBoxLayout* partRoleVLayout = new QVBoxLayout;
        partRoleVLayout->addLayout(partRoleLayout);

        if (dust3d::PartTarget::CutFace == m_part->target) {
            size_t usageCount = 0;
            std::string cutFacePartIdString = m_partId.toString();
            for (const auto& it : m_document->partMap) {
                if (it.second.cutFace == dust3d::CutFace::UserDefined
                    && it.second.cutFaceLinkedId.toString() == cutFacePartIdString) {
                    ++usageCount;
                }
                for (const auto& nodeId : it.second.nodeIds) {
                    const Document::Node* node = m_document->findNode(nodeId);
                    if (nullptr != node && node->hasCutFaceSettings
                        && node->cutFace == dust3d::CutFace::UserDefined
                        && node->cutFaceLinkedId.toString() == cutFacePartIdString) {
                        ++usageCount;
                    }
                }
            }
            QLabel* usageLabel = new QLabel;
            if (0 == usageCount)
                usageLabel->setText(tr("Not used by any part"));
            else if (1 == usageCount)
                usageLabel->setText(tr("Used by 1 part"));
            else
                usageLabel->setText(tr("Used by %1 parts").arg(usageCount));
            usageLabel->setStyleSheet("color: gray; font-style: italic;");
            partRoleVLayout->addWidget(usageLabel);
        }

        partRoleGroupBox = new QGroupBox(tr("Part Role"));
        partRoleGroupBox->setLayout(partRoleVLayout);
    }

    QGroupBox* deformGroupBox = nullptr;
    if (nullptr != m_part && (dust3d::PartTarget::Model == m_part->target || dust3d::PartTarget::ImportedModel == m_part->target)) {
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

        connect(deformUnifyStateBox, checkboxStateChangedSignal, this, [=]() {
            emit setPartDeformUnified(m_partId, deformUnifyStateBox->isChecked());
            emit groupOperationAdded();
        });

        QCheckBox* mirrorStateBox = new QCheckBox();
        Theme::initCheckbox(mirrorStateBox);
        mirrorStateBox->setText(tr("Mirrored"));
        mirrorStateBox->setChecked(m_part->xMirrored);

        connect(mirrorStateBox, checkboxStateChangedSignal, this, [=]() {
            emit setPartXmirrorState(m_partId, mirrorStateBox->isChecked());
            emit groupOperationAdded();
        });

        QHBoxLayout* deformUnifyLayout = new QHBoxLayout;
        deformUnifyLayout->addStretch();
        deformUnifyLayout->addWidget(mirrorStateBox);
        if (dust3d::PartTarget::Model == m_part->target)
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
            dust3d::Uuid cutFacePartId(cutFaceString.toUtf8().constData());
            QPushButton* button = new QPushButton;
            button->setIconSize(QSize(Theme::toolIconSize * 0.75, Theme::toolIconSize * 0.75));
            if (cutFacePartId.isNull()) {
                dust3d::CutFace cutFace = dust3d::CutFaceFromString(cutFaceString.toUtf8().constData());
                button->setIcon(QIcon(QPixmap::fromImage(*cutFacePreviewImage(cutFace))));
                connect(button, &QPushButton::clicked, [=]() {
                    updateCutFaceButtonState(i);
                    emit setPartCutFace(m_partId, cutFace);
                    emit groupOperationAdded();
                });
            } else {
                const Document::Part* part = m_document->findPart(cutFacePartId);
                if (nullptr != part) {
                    const Document::Component* component = m_document->findComponent(part->componentId);
                    if (nullptr != component)
                        button->setIcon(QIcon(component->previewPixmap));
                }
                connect(button, &QPushButton::clicked, [=]() {
                    updateCutFaceButtonState(i);
                    emit setPartCutFaceLinkedId(m_partId, cutFacePartId);
                    emit groupOperationAdded();
                });
            }
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

        connect(subdivStateBox, checkboxStateChangedSignal, this, [=]() {
            emit setPartSubdivState(m_partId, subdivStateBox->isChecked());
            emit groupOperationAdded();
        });

        QCheckBox* chamferStateBox = new QCheckBox();
        Theme::initCheckbox(chamferStateBox);
        chamferStateBox->setText(tr("Chamfered"));
        chamferStateBox->setChecked(m_part->chamfered);

        connect(chamferStateBox, checkboxStateChangedSignal, this, [=]() {
            emit setPartChamferState(m_partId, chamferStateBox->isChecked());
            emit groupOperationAdded();
        });

        QCheckBox* roundEndStateBox = new QCheckBox();
        Theme::initCheckbox(roundEndStateBox);
        roundEndStateBox->setText(tr("Round end"));
        roundEndStateBox->setChecked(m_part->rounded);

        connect(roundEndStateBox, checkboxStateChangedSignal, this, [=]() {
            emit setPartRoundState(m_partId, roundEndStateBox->isChecked());
            emit groupOperationAdded();
        });

        QCheckBox* hardStateBox = new QCheckBox();
        Theme::initCheckbox(hardStateBox);
        hardStateBox->setText(tr("Hard edges"));
        hardStateBox->setToolTip(tr("Join other parts with a crisp boolean edge instead of a smooth blend (machines, props)"));
        hardStateBox->setChecked(m_part->hard);

        connect(hardStateBox, checkboxStateChangedSignal, this, [=]() {
            emit setPartHardState(m_partId, hardStateBox->isChecked());
            emit groupOperationAdded();
        });

        QCheckBox* interpolatedStateBox = new QCheckBox();
        Theme::initCheckbox(interpolatedStateBox);
        interpolatedStateBox->setText(tr("Extra rings"));
        interpolatedStateBox->setToolTip(tr("Add rings along long edges so the part bends smoothly; turn off for rigid, low-poly parts"));
        interpolatedStateBox->setChecked(m_part->interpolated);

        connect(interpolatedStateBox, checkboxStateChangedSignal, this, [=]() {
            emit setPartInterpolatedState(m_partId, interpolatedStateBox->isChecked());
            emit groupOperationAdded();
        });

        QHBoxLayout* optionsLayout = new QHBoxLayout;
        optionsLayout->addStretch();
        optionsLayout->addWidget(roundEndStateBox);
        optionsLayout->addWidget(chamferStateBox);
        optionsLayout->addWidget(subdivStateBox);

        QHBoxLayout* hardSurfaceLayout = new QHBoxLayout;
        hardSurfaceLayout->addStretch();
        hardSurfaceLayout->addWidget(hardStateBox);
        hardSurfaceLayout->addWidget(interpolatedStateBox);

        QVBoxLayout* cutFaceLayout = new QVBoxLayout;
        cutFaceLayout->addLayout(cutFaceIconLayout);
        cutFaceLayout->addLayout(rotationLayout);
        cutFaceLayout->addLayout(optionsLayout);
        cutFaceLayout->addLayout(hardSurfaceLayout);

        cutFaceGroupBox = new QGroupBox(tr("Cut Face"));
        cutFaceGroupBox->setLayout(cutFaceLayout);
    }

    QGroupBox* smoothGroupBox = nullptr;
    if (!m_componentIds.empty() && !(nullptr != m_part && dust3d::PartTarget::CutFace == m_part->target)) {
        FloatNumberWidget* smoothCutoffDegreesWidget = new FloatNumberWidget;
        smoothCutoffDegreesWidget->setItemName(tr("Cutoff"));
        smoothCutoffDegreesWidget->setRange(0.0, 180.0);
        smoothCutoffDegreesWidget->setValue(lastSmoothCutoffDegrees());

        connect(smoothCutoffDegreesWidget, &FloatNumberWidget::valueChanged, [=](float value) {
            for (const auto& componentId : m_componentIds)
                emit setComponentSmoothCutoffDegrees(componentId, value);
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

    QGroupBox* materialGroupBox = nullptr;
    if (nullptr != m_part && dust3d::PartTarget::Model == m_part->target) {
        // metallic, roughness and glow: baked into the exported texture maps per part
        auto addSlider = [&](QVBoxLayout* layout, const QString& name, float value, float reset,
                             std::function<void(float)> apply, float maxValue = 1.0) {
            FloatNumberWidget* widget = new FloatNumberWidget;
            widget->setItemName(name);
            widget->setRange(0.0, maxValue);
            widget->setValue(value);
            connect(widget, &FloatNumberWidget::valueChanged, [=](float v) {
                apply(v);
                emit groupOperationAdded();
            });
            QPushButton* eraser = new QPushButton(Theme::awesome()->icon(fa::eraser), "");
            Theme::initIconButton(eraser);
            connect(eraser, &QPushButton::clicked, [=]() {
                widget->setValue(reset);
                emit groupOperationAdded();
            });
            QHBoxLayout* row = new QHBoxLayout;
            row->addWidget(eraser);
            row->addWidget(widget);
            layout->addLayout(row);
        };
        QVBoxLayout* materialLayout = new QVBoxLayout;
        addSlider(materialLayout, tr("Metallic"), m_part->metalness, 0.0, [=](float v) { emit setPartMetalness(m_partId, v); });
        addSlider(materialLayout, tr("Roughness"), m_part->roughness, 1.0, [=](float v) { emit setPartRoughness(m_partId, v); });
        addSlider(materialLayout, tr("Glow"), m_part->emissive, 0.0, [=](float v) { emit setPartEmissive(m_partId, v); }, 2.0);
        materialGroupBox = new QGroupBox(tr("Material"));
        materialGroupBox->setLayout(materialLayout);
    }

    // Equipment slot: a part tagged "armor/2" is exported as part of the "armor" slot's
    // variant "2" (glTF mesh extras); game tools split each variant into its own mesh on the
    // same skeleton, so a game can show the equipped one. Stored as a suffix of the
    // component name ("vest @armor/2"), which is also how it shows in the part list.
    QGroupBox* slotGroupBox = nullptr;
    if (1 == m_componentIds.size() && nullptr != m_part && dust3d::PartTarget::Model == m_part->target) {
        const Document::Component* component = m_document->findComponent(m_componentIds.front());
        if (nullptr != component) {
            QString fullName = component->name;
            int mark = fullName.lastIndexOf(" @");
            QString baseName = mark >= 0 ? fullName.left(mark) : fullName;
            QString slot = mark >= 0 ? fullName.mid(mark + 2) : QString();
            QLineEdit* slotEdit = new QLineEdit(slot);
            slotEdit->setPlaceholderText(tr("slot/variant, e.g. armor/2"));
            slotEdit->setToolTip(tr("Equipment slot and variant. Leave empty for a part that is always shown."));
            dust3d::Uuid componentId = m_componentIds.front();
            connect(slotEdit, &QLineEdit::editingFinished, this, [=]() {
                QString value = slotEdit->text().trimmed();
                QString name = value.isEmpty() ? baseName : baseName + " @" + value;
                const Document::Component* current = m_document->findComponent(componentId);
                if (nullptr == current || current->name == name)
                    return;
                m_document->renameComponent(componentId, name);
                emit groupOperationAdded();
            });
            QHBoxLayout* slotLayout = new QHBoxLayout;
            slotLayout->addWidget(new QLabel(tr("Slot")));
            slotLayout->addWidget(slotEdit);
            slotGroupBox = new QGroupBox(tr("Equipment"));
            slotGroupBox->setLayout(slotLayout);
        }
    }

    QGroupBox* colorImageGroupBox = nullptr;
    if (!m_componentIds.empty() && !(nullptr != m_part && (dust3d::PartTarget::ImportedModel == m_part->target || dust3d::PartTarget::CutFace == m_part->target))) {
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
            colorImagePreviewWidget->updateImage(QImage());
            emit groupOperationAdded();
        });

        QPushButton* colorImagePicker = new QPushButton(Theme::awesome()->icon(fa::image), "");
        Theme::initIconButton(colorImagePicker);

        connect(colorImagePicker, &QPushButton::clicked, [=]() {
            pickColorImageForComponents(m_componentIds);
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
    if (!m_componentIds.empty() && hasStitchingLineConfigure()) {
        QCheckBox* frontCloseStateBox = new QCheckBox();
        Theme::initCheckbox(frontCloseStateBox);
        frontCloseStateBox->setText(tr("Front Closed"));
        frontCloseStateBox->setChecked(lastFrontClosed());

        QCheckBox* backCloseStateBox = new QCheckBox();
        Theme::initCheckbox(backCloseStateBox);
        backCloseStateBox->setText(tr("Back Closed"));
        backCloseStateBox->setChecked(lastBackClosed());

        QCheckBox* sideCloseStateBox = new QCheckBox();
        Theme::initCheckbox(sideCloseStateBox);
        sideCloseStateBox->setText(tr("Side Closed"));
        sideCloseStateBox->setChecked(lastSideClosed());

        connect(frontCloseStateBox, checkboxStateChangedSignal, this, [=]() {
            bool closed = frontCloseStateBox->isChecked();
            for (const auto& componentId : m_componentIds)
                emit setComponentFrontCloseState(componentId, closed);
            emit groupOperationAdded();
        });

        connect(backCloseStateBox, checkboxStateChangedSignal, this, [=]() {
            bool closed = backCloseStateBox->isChecked();
            for (const auto& componentId : m_componentIds)
                emit setComponentBackCloseState(componentId, closed);
            emit groupOperationAdded();
        });

        connect(sideCloseStateBox, checkboxStateChangedSignal, this, [=]() {
            bool closed = sideCloseStateBox->isChecked();
            for (const auto& componentId : m_componentIds)
                emit setComponentSideCloseState(componentId, closed);
            emit groupOperationAdded();
        });

        QHBoxLayout* optionsLayout = new QHBoxLayout;
        optionsLayout->addStretch();
        optionsLayout->addWidget(frontCloseStateBox);
        optionsLayout->addWidget(backCloseStateBox);
        optionsLayout->addWidget(sideCloseStateBox);

        QLabel* targetSegmentsLabel = new QLabel;
        targetSegmentsLabel->setText(tr("Target Segments:"));

        QSpinBox* targetSegmentsBox = new QSpinBox;
        targetSegmentsBox->setRange(0, 100);
        targetSegmentsBox->setValue(lastTargetSegments());

        connect(targetSegmentsBox, QOverload<int>::of(&QSpinBox::valueChanged), this, [=](int value) {
            for (const auto& componentId : m_componentIds)
                emit setComponentTargetSegments(componentId, (size_t)value);
            emit groupOperationAdded();
        });

        QHBoxLayout* targetSegmentsLayout = new QHBoxLayout;
        targetSegmentsLayout->addStretch();
        targetSegmentsLayout->addWidget(targetSegmentsLabel);
        targetSegmentsLayout->addWidget(targetSegmentsBox);

        QVBoxLayout* stitchingLineLayout = new QVBoxLayout;
        stitchingLineLayout->addLayout(optionsLayout);
        stitchingLineLayout->addLayout(targetSegmentsLayout);

        stitchingLineGroupBox = new QGroupBox(tr("Stitching Line"));
        stitchingLineGroupBox->setLayout(stitchingLineLayout);
    }

    QGroupBox* stitchingLoopGroupBox = nullptr;
    if (!m_componentIds.empty() && (hasStitchingLoopConfigure() || (nullptr != m_part && dust3d::PartTarget::StitchingLoop == m_part->target))) {
        QVBoxLayout* stitchingLoopLayout = new QVBoxLayout;

        if (nullptr == m_part) {
            QCheckBox* loopBackCloseStateBox = new QCheckBox();
            Theme::initCheckbox(loopBackCloseStateBox);
            loopBackCloseStateBox->setText(tr("Back Closed"));
            loopBackCloseStateBox->setToolTip(tr("Requires side profile facing left for correct cap normal"));
            loopBackCloseStateBox->setChecked(lastBackClosed());

            FloatNumberWidget* depthRatioWidget = new FloatNumberWidget;
            depthRatioWidget->setItemName(tr("Depth"));
            depthRatioWidget->setRange(0.0, 2.0);
            depthRatioWidget->setValue(lastBackCloseDepthRatio());

            FloatNumberWidget* sharpnessWidget = new FloatNumberWidget;
            sharpnessWidget->setItemName(tr("Shape"));
            sharpnessWidget->setRange(0.0, 1.0);
            sharpnessWidget->setValue(lastBackCloseSharpness());

            QWidget* backCloseParamsWidget = new QWidget;
            QVBoxLayout* backCloseParamsLayout = new QVBoxLayout;
            backCloseParamsLayout->setContentsMargins(0, 0, 0, 0);

            QHBoxLayout* depthLayout = new QHBoxLayout;
            depthLayout->addWidget(depthRatioWidget);
            backCloseParamsLayout->addLayout(depthLayout);

            QHBoxLayout* sharpnessLayout = new QHBoxLayout;
            sharpnessLayout->addWidget(sharpnessWidget);
            backCloseParamsLayout->addLayout(sharpnessLayout);

            backCloseParamsWidget->setLayout(backCloseParamsLayout);
            bool initiallyEnabled = loopBackCloseStateBox->isChecked();
            backCloseParamsWidget->setEnabled(initiallyEnabled);
            auto* opacityEffect = new QGraphicsOpacityEffect(backCloseParamsWidget);
            opacityEffect->setOpacity(initiallyEnabled ? 1.0 : 0.35);
            backCloseParamsWidget->setGraphicsEffect(opacityEffect);

            connect(loopBackCloseStateBox, checkboxStateChangedSignal, this, [=]() {
                bool closed = loopBackCloseStateBox->isChecked();
                backCloseParamsWidget->setEnabled(closed);
                opacityEffect->setOpacity(closed ? 1.0 : 0.35);
                for (const auto& componentId : m_componentIds)
                    emit setComponentBackCloseState(componentId, closed);
                emit groupOperationAdded();
            });

            connect(depthRatioWidget, &FloatNumberWidget::valueChanged, [=](float value) {
                for (const auto& componentId : m_componentIds)
                    emit setComponentBackCloseDepthRatio(componentId, value);
                emit groupOperationAdded();
            });

            connect(sharpnessWidget, &FloatNumberWidget::valueChanged, [=](float value) {
                for (const auto& componentId : m_componentIds)
                    emit setComponentBackCloseSharpness(componentId, value);
                emit groupOperationAdded();
            });

            QHBoxLayout* loopOptionsLayout = new QHBoxLayout;
            loopOptionsLayout->addStretch();
            loopOptionsLayout->addWidget(loopBackCloseStateBox);
            stitchingLoopLayout->addLayout(loopOptionsLayout);
            stitchingLoopLayout->addWidget(backCloseParamsWidget);
        }

        if (nullptr != m_part && dust3d::PartTarget::StitchingLoop == m_part->target) {
            QCheckBox* fillLoopInteriorBox = new QCheckBox();
            Theme::initCheckbox(fillLoopInteriorBox);
            fillLoopInteriorBox->setText(tr("Fill Loop Interior"));
            fillLoopInteriorBox->setChecked(m_part->fillLoopInterior);
            connect(fillLoopInteriorBox, checkboxStateChangedSignal, this, [=]() {
                emit setPartFillLoopInteriorState(m_partId, fillLoopInteriorBox->isChecked());
                emit groupOperationAdded();
            });
            QHBoxLayout* fillLayout = new QHBoxLayout;
            fillLayout->addStretch();
            fillLayout->addWidget(fillLoopInteriorBox);
            stitchingLoopLayout->addLayout(fillLayout);
        }

        stitchingLoopGroupBox = new QGroupBox(tr("Stitching Loop"));
        stitchingLoopGroupBox->setLayout(stitchingLoopLayout);
    }

    QGroupBox* wrapGroupBox = nullptr;
    if (!m_componentIds.empty() && nullptr == m_part && hasGroupsOnly()) {
        // The wrap modifier: one surface wrapped around everything the group generates.
        QComboBox* wrapModeComboBox = new QComboBox;
        wrapModeComboBox->setSizeAdjustPolicy(QComboBox::AdjustToContents);
        wrapModeComboBox->addItem(tr("None"), QString());
        wrapModeComboBox->addItem(tr("Creature Skin"), QString("Skin"));
        wrapModeComboBox->addItem(tr("Cloth"), QString("Cloth"));
        wrapModeComboBox->setToolTip(tr("Creature Skin replaces the children with one seamless skin over them.\n"
                                        "Cloth keeps the children and adds a loose garment over them."));
        QString mode = lastWrapAttribute("wrap");
        wrapModeComboBox->setCurrentIndex("Skin" == mode ? 1 : ("Cloth" == mode ? 2 : 0));
        bool cloth = "Cloth" == mode;

        auto floatValue = [&](const std::string& name, float defaultValue) {
            QString value = lastWrapAttribute(name);
            return value.isEmpty() ? defaultValue : value.toFloat();
        };
        struct Setting {
            const char* name;
            QString label;
            float minValue;
            float maxValue;
            float defaultValue;
            bool clothOnly;
        };
        std::vector<Setting> settings = {
            { "wrapOffset", tr("Offset"), 0.0f, 0.1f, cloth ? 0.012f : 0.0f, false },
            { "wrapSmoothness", tr("Smoothness"), 0.0f, 0.15f, cloth ? 0.05f : 0.02f, false },
            { "wrapDrape", tr("Drape"), 0.0f, 1.0f, cloth ? 0.5f : 0.0f, true },
            { "wrapDrapeLength", tr("Drape Length"), 0.0f, 0.5f, 0.0f, true },
            { "wrapOpenTop", tr("Open Top"), 0.0f, 0.45f, 0.0f, true },
            { "wrapOpenBottom", tr("Open Bottom"), 0.0f, 0.45f, 0.0f, true },
            { "wrapThickness", tr("Thickness"), 0.0f, 0.02f, cloth ? 0.004f : 0.0f, true },
        };
        QWidget* wrapSettingsWidget = new QWidget;
        QVBoxLayout* wrapSettingsLayout = new QVBoxLayout;
        wrapSettingsLayout->setContentsMargins(0, 0, 0, 0);
        for (const auto& setting : settings) {
            if (setting.clothOnly && !cloth)
                continue;
            FloatNumberWidget* widget = new FloatNumberWidget;
            widget->setItemName(setting.label);
            widget->setRange(setting.minValue, setting.maxValue);
            widget->setValue(floatValue(setting.name, setting.defaultValue));
            std::string name = setting.name;
            connect(widget, &FloatNumberWidget::valueChanged, [=](float value) {
                for (const auto& componentId : m_componentIds)
                    emit setComponentWrapAttribute(componentId, QString::fromStdString(name), QString::number(value));
                emit groupOperationAdded();
            });
            wrapSettingsLayout->addWidget(widget);
        }
        QCheckBox* keepBox = new QCheckBox();
        Theme::initCheckbox(keepBox);
        keepBox->setText(tr("Keep Children"));
        keepBox->setToolTip(tr("Show the children under the wrap (a garment over the body).\n"
                               "Off: the children only shape the wrap (a skin, or a garment over guide shapes)."));
        QString keepValue = lastWrapAttribute("wrapKeep");
        keepBox->setChecked(keepValue.isEmpty() ? cloth : ("true" == keepValue));
        connect(keepBox, checkboxStateChangedSignal, this, [=]() {
            for (const auto& componentId : m_componentIds)
                emit setComponentWrapAttribute(componentId, "wrapKeep", keepBox->isChecked() ? "true" : "false");
            emit groupOperationAdded();
        });
        QHBoxLayout* keepLayout = new QHBoxLayout;
        keepLayout->addStretch();
        keepLayout->addWidget(keepBox);
        wrapSettingsLayout->addLayout(keepLayout);

        // Weights From: a garment can take its skin weights from the body it is worn over
        QComboBox* bindToComboBox = new QComboBox;
        bindToComboBox->setSizeAdjustPolicy(QComboBox::AdjustToContents);
        bindToComboBox->setToolTip(tr("Where the skin weights come from.\n"
                                      "Pick the body a garment is worn over: both then bend alike and the body stays inside."));
        bindToComboBox->addItem(tr("Weights: Own Children"), QString());
        {
            std::set<dust3d::Uuid> excluded(m_componentIds.begin(), m_componentIds.end());
            std::vector<dust3d::Uuid> stack(m_componentIds.begin(), m_componentIds.end());
            while (!stack.empty()) {
                const Document::Component* component = m_document->findComponent(stack.back());
                stack.pop_back();
                if (nullptr == component)
                    continue;
                for (const auto& childId : component->childrenIds) {
                    excluded.insert(childId);
                    stack.push_back(childId);
                }
            }
            QString current = lastWrapAttribute("wrapBindTo");
            for (const auto& it : m_document->componentMap) {
                if (!it.second.linkToPartId.isNull() || excluded.count(it.first))
                    continue;
                QString name = it.second.name.isEmpty() ? tr("Group") : it.second.name;
                QString idString = QString::fromStdString(it.first.toString());
                bindToComboBox->addItem(tr("Weights: %1").arg(name), idString);
                if (idString == current)
                    bindToComboBox->setCurrentIndex(bindToComboBox->count() - 1);
            }
        }
        connect(bindToComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [=](int index) {
            QString value = bindToComboBox->itemData(index).toString();
            for (const auto& componentId : m_componentIds)
                emit setComponentWrapAttribute(componentId, "wrapBindTo", value);
            emit groupOperationAdded();
        });
        QHBoxLayout* bindToLayout = new QHBoxLayout;
        bindToLayout->addWidget(bindToComboBox);
        bindToLayout->addStretch();
        wrapSettingsLayout->addLayout(bindToLayout);

        IntNumberWidget* facesWidget = new IntNumberWidget;
        facesWidget->setItemName(tr("Faces"));
        facesWidget->setRange(64, 20000);
        facesWidget->setValue((int)floatValue("wrapFaces", cloth ? 1200.0f : 1600.0f));
        connect(facesWidget, &IntNumberWidget::valueChanged, [=](int value) {
            for (const auto& componentId : m_componentIds)
                emit setComponentWrapAttribute(componentId, "wrapFaces", QString::number(value));
            emit groupOperationAdded();
        });
        wrapSettingsLayout->addWidget(facesWidget);

        // Animal skin pattern, painted into the texture from the 3D surface (no seams)
        QComboBox* patternComboBox = new QComboBox;
        patternComboBox->setSizeAdjustPolicy(QComboBox::AdjustToContents);
        patternComboBox->setToolTip(tr("An animal coat painted into the texture: spots (cheetah), rosettes (leopard),\n"
                                       "stripes (tiger, zebra), patches (giraffe) or mottled (frog, camouflage)."));
        patternComboBox->addItem(tr("Pattern: None"), QString());
        patternComboBox->addItem(tr("Pattern: Spots"), QString("Spots"));
        patternComboBox->addItem(tr("Pattern: Rosettes"), QString("Rosettes"));
        patternComboBox->addItem(tr("Pattern: Stripes"), QString("Stripes"));
        patternComboBox->addItem(tr("Pattern: Patches"), QString("Patches"));
        patternComboBox->addItem(tr("Pattern: Mottled"), QString("Mottled"));
        QString patternValue = lastWrapAttribute("wrapPattern");
        for (int i = 0; i < patternComboBox->count(); ++i) {
            if (patternComboBox->itemData(i).toString() == patternValue)
                patternComboBox->setCurrentIndex(i);
        }
        QWidget* patternDetailsWidget = new QWidget;
        QVBoxLayout* patternDetailsLayout = new QVBoxLayout;
        patternDetailsLayout->setContentsMargins(0, 0, 0, 0);

        QPushButton* patternColorButton = new QPushButton;
        patternColorButton->setToolTip(tr("The colour of the pattern (Auto: a dark tone of the base colour)"));
        auto showPatternColor = [patternColorButton](const QString& value) {
            if (value.isEmpty()) {
                patternColorButton->setText(tr("Pattern Colour: Auto"));
                patternColorButton->setStyleSheet(QString());
            } else {
                patternColorButton->setText(tr("Pattern Colour"));
                QColor color(value);
                patternColorButton->setStyleSheet("QPushButton {background-color: " + color.name() + "; color: "
                    + (color.lightness() > 128 ? "black" : "white") + ";}");
            }
        };
        showPatternColor(lastWrapAttribute("wrapPatternColor"));
        connect(patternColorButton, &QPushButton::clicked, this, [=]() {
            // the dialog lives on the main window and talks to the document directly: this
            // widget sits in a popup menu that closes as soon as the dialog takes focus
            QPointer<Document> document = m_document;
            std::vector<dust3d::Uuid> componentIds = m_componentIds;
            QString initial = lastWrapAttribute("wrapPatternColor");
            QWidget* host = nullptr != window()->parentWidget() ? window()->parentWidget()->window() : nullptr;
            QColorDialog* dialog = new QColorDialog(initial.isEmpty() ? QColor(40, 30, 20) : QColor(initial), host);
            dialog->setAttribute(Qt::WA_DeleteOnClose);
            dialog->setWindowTitle(tr("Pattern Colour"));
            connect(dialog, &QColorDialog::currentColorChanged, dialog, [=](const QColor& color) {
                if (document.isNull())
                    return;
                for (const auto& componentId : componentIds)
                    document->setComponentWrapAttribute(componentId, "wrapPatternColor", color.name());
            });
            connect(dialog, &QColorDialog::rejected, dialog, [=]() {
                if (document.isNull())
                    return;
                for (const auto& componentId : componentIds)
                    document->setComponentWrapAttribute(componentId, "wrapPatternColor", initial);
            });
            connect(dialog, &QColorDialog::colorSelected, dialog, [=](const QColor& color) {
                if (document.isNull())
                    return;
                for (const auto& componentId : componentIds)
                    document->setComponentWrapAttribute(componentId, "wrapPatternColor", color.name());
                document->saveSnapshot();
            });
            dialog->show();
        });
        QPushButton* patternColorAutoButton = new QPushButton(Theme::awesome()->icon(fa::eraser), "");
        Theme::initIconButton(patternColorAutoButton);
        patternColorAutoButton->setToolTip(tr("Automatic pattern colour"));
        connect(patternColorAutoButton, &QPushButton::clicked, this, [=]() {
            for (const auto& componentId : m_componentIds)
                emit setComponentWrapAttribute(componentId, "wrapPatternColor", QString());
            emit groupOperationAdded();
            showPatternColor(QString());
        });
        QHBoxLayout* patternColorLayout = new QHBoxLayout;
        patternColorLayout->addWidget(patternColorAutoButton);
        patternColorLayout->addWidget(patternColorButton);
        patternColorLayout->addStretch();
        patternDetailsLayout->addLayout(patternColorLayout);

        FloatNumberWidget* patternScaleWidget = new FloatNumberWidget;
        patternScaleWidget->setItemName(tr("Pattern Size"));
        patternScaleWidget->setRange(0.01f, 0.3f);
        patternScaleWidget->setValue(floatValue("wrapPatternScale", 0.06f));
        patternScaleWidget->setToolTip(tr("The size of one spot, rosette or stripe, in model units"));
        connect(patternScaleWidget, &FloatNumberWidget::valueChanged, [=](float value) {
            for (const auto& componentId : m_componentIds)
                emit setComponentWrapAttribute(componentId, "wrapPatternScale", QString::number(value));
            emit groupOperationAdded();
        });
        patternDetailsLayout->addWidget(patternScaleWidget);
        patternDetailsWidget->setLayout(patternDetailsLayout);
        patternDetailsWidget->setVisible(!patternValue.isEmpty());

        FloatNumberWidget* bellyWidget = new FloatNumberWidget;
        bellyWidget->setItemName(tr("Lighter Belly"));
        bellyWidget->setRange(0.0f, 1.0f);
        bellyWidget->setValue(floatValue("wrapBelly", 0.0f));
        bellyWidget->setToolTip(tr("Countershading: how much lighter the underside is (the pattern fades there too)"));
        connect(bellyWidget, &FloatNumberWidget::valueChanged, [=](float value) {
            for (const auto& componentId : m_componentIds)
                emit setComponentWrapAttribute(componentId, "wrapBelly", QString::number(value));
            emit groupOperationAdded();
        });

        connect(patternComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [=](int index) {
            QString value = patternComboBox->itemData(index).toString();
            for (const auto& componentId : m_componentIds)
                emit setComponentWrapAttribute(componentId, "wrapPattern", value);
            emit groupOperationAdded();
            patternDetailsWidget->setVisible(!value.isEmpty());
        });
        QHBoxLayout* patternLayout = new QHBoxLayout;
        patternLayout->addWidget(patternComboBox);
        patternLayout->addStretch();
        wrapSettingsLayout->addLayout(patternLayout);
        wrapSettingsLayout->addWidget(patternDetailsWidget);
        wrapSettingsLayout->addWidget(bellyWidget);
        wrapSettingsWidget->setLayout(wrapSettingsLayout);
        wrapSettingsWidget->setVisible(!mode.isEmpty());

        connect(wrapModeComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [=](int index) {
            QString newMode = wrapModeComboBox->itemData(index).toString();
            for (const auto& componentId : m_componentIds)
                emit setComponentWrapAttribute(componentId, "wrap", newMode);
            emit groupOperationAdded();
            // the settings differ per mode: close the menu, it is rebuilt on the next open
            QWidget* widget = this;
            while (nullptr != widget) {
                QMenu* menu = qobject_cast<QMenu*>(widget);
                if (nullptr != menu) {
                    menu->close();
                    break;
                }
                widget = widget->parentWidget();
            }
        });

        QHBoxLayout* wrapModeLayout = new QHBoxLayout;
        wrapModeLayout->addWidget(wrapModeComboBox);
        wrapModeLayout->addStretch();
        QVBoxLayout* wrapLayout = new QVBoxLayout;
        wrapLayout->addLayout(wrapModeLayout);
        wrapLayout->addWidget(wrapSettingsWidget);
        wrapGroupBox = new QGroupBox(tr("Wrap Modifier"));
        wrapGroupBox->setLayout(wrapLayout);
    }

    QGroupBox* importedModelGroupBox = nullptr;
    if (nullptr != m_part && dust3d::PartTarget::ImportedModel == m_part->target) {
        QPushButton* importGlbButton = new QPushButton(tr("Import GLB File..."));
        importGlbButton->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Fixed);
        connect(importGlbButton, &QPushButton::clicked, this, [=]() {
            QString filename = QFileDialog::getOpenFileName(this, tr("Import GLB File"), QString(), tr("GLB Files (*.glb)"));
            if (filename.isEmpty())
                return;
            QFile file(filename);
            if (!file.open(QIODevice::ReadOnly))
                return;
            QByteArray glbData = file.readAll();
            dust3d::Uuid glbId = GlbForever::add(&glbData);
            if (glbId.isNull())
                return;
            m_document->setPartImportedModelId(m_partId, glbId);
            m_document->saveSnapshot();
        });
        FloatNumberWidget* importedModelRotationWidget = new FloatNumberWidget;
        importedModelRotationWidget->setItemName(tr("Rotation"));
        importedModelRotationWidget->setRange(-1, 1);
        importedModelRotationWidget->setValue(m_part->cutRotation);

        connect(importedModelRotationWidget, &FloatNumberWidget::valueChanged, [=](float value) {
            emit setPartCutRotation(m_partId, value);
            emit groupOperationAdded();
        });

        QPushButton* importedModelRotationEraser = new QPushButton(Theme::awesome()->icon(fa::eraser), "");
        Theme::initIconButton(importedModelRotationEraser);

        connect(importedModelRotationEraser, &QPushButton::clicked, [=]() {
            importedModelRotationWidget->setValue(0.0);
            emit groupOperationAdded();
        });

        QHBoxLayout* importedModelRotationLayout = new QHBoxLayout;
        importedModelRotationLayout->addWidget(importedModelRotationEraser);
        importedModelRotationLayout->addWidget(importedModelRotationWidget);

        QVBoxLayout* importedModelLayout = new QVBoxLayout;
        importedModelLayout->addWidget(importGlbButton);
        importedModelLayout->addLayout(importedModelRotationLayout);
        importedModelGroupBox = new QGroupBox(tr("Imported Model"));
        importedModelGroupBox->setLayout(importedModelLayout);
    }

    QVBoxLayout* mainLayout = new QVBoxLayout;
    mainLayout->addLayout(topLayout);
    if (nullptr != partRoleGroupBox)
        mainLayout->addWidget(partRoleGroupBox);
    if (nullptr != importedModelGroupBox)
        mainLayout->addWidget(importedModelGroupBox);
    if (nullptr != deformGroupBox)
        mainLayout->addWidget(deformGroupBox);
    if (nullptr != cutFaceGroupBox)
        mainLayout->addWidget(cutFaceGroupBox);
    if (nullptr != smoothGroupBox)
        mainLayout->addWidget(smoothGroupBox);
    if (nullptr != materialGroupBox)
        mainLayout->addWidget(materialGroupBox);
    if (nullptr != slotGroupBox)
        mainLayout->addWidget(slotGroupBox);
    mainLayout->addLayout(skinLayout);
    if (nullptr != stitchingLineGroupBox)
        mainLayout->addWidget(stitchingLineGroupBox);
    if (nullptr != stitchingLoopGroupBox)
        mainLayout->addWidget(stitchingLoopGroupBox);
    if (nullptr != wrapGroupBox)
        mainLayout->addWidget(wrapGroupBox);
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
    connect(this, &ComponentPropertyWidget::setPartHardState, m_document, &Document::setPartHardState);
    connect(this, &ComponentPropertyWidget::setPartMetalness, m_document, &Document::setPartMetalness);
    connect(this, &ComponentPropertyWidget::setPartRoughness, m_document, &Document::setPartRoughness);
    connect(this, &ComponentPropertyWidget::setPartEmissive, m_document, &Document::setPartEmissive);
    connect(this, &ComponentPropertyWidget::setPartInterpolatedState, m_document, &Document::setPartInterpolatedState);
    connect(this, &ComponentPropertyWidget::setPartRoundState, m_document, &Document::setPartRoundState);
    connect(this, &ComponentPropertyWidget::setComponentColorImage, m_document, &Document::setComponentColorImage);
    connect(this, &ComponentPropertyWidget::setComponentSideCloseState, m_document, &Document::setComponentSideCloseState);
    connect(this, &ComponentPropertyWidget::setComponentFrontCloseState, m_document, &Document::setComponentFrontCloseState);
    connect(this, &ComponentPropertyWidget::setComponentBackCloseState, m_document, &Document::setComponentBackCloseState);
    connect(this, &ComponentPropertyWidget::setComponentBackCloseDepthRatio, m_document, &Document::setComponentBackCloseDepthRatio);
    connect(this, &ComponentPropertyWidget::setComponentBackCloseSharpness, m_document, &Document::setComponentBackCloseSharpness);
    connect(this, &ComponentPropertyWidget::setPartFillLoopInteriorState, m_document, &Document::setPartFillLoopInteriorState);
    connect(this, &ComponentPropertyWidget::setComponentTargetSegments, m_document, &Document::setComponentTargetSegments);
    connect(this, &ComponentPropertyWidget::setComponentSmoothCutoffDegrees, m_document, &Document::setComponentSmoothCutoffDegrees);
    connect(this, &ComponentPropertyWidget::setPartCutFace, m_document, &Document::setPartCutFace);
    connect(this, &ComponentPropertyWidget::setPartCutFaceLinkedId, m_document, &Document::setPartCutFaceLinkedId);
    connect(this, &ComponentPropertyWidget::setPartXmirrorState, m_document, &Document::setPartXmirrorState);
    connect(this, &ComponentPropertyWidget::setPartTarget, m_document, &Document::setPartTarget);
    connect(this, &ComponentPropertyWidget::setComponentCombineMode, m_document, &Document::setComponentCombineMode);
    connect(this, &ComponentPropertyWidget::setComponentWrapAttribute, m_document, &Document::setComponentWrapAttribute);
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

void ComponentPropertyWidget::pickColorImageForComponents(const std::vector<dust3d::Uuid>& componentIds)
{
#if defined(Q_OS_WASM)
    QFileDialog::getOpenFileContent(tr("Image Files (*.png *.jpg *.jpeg *.bmp)"),
        [=](const QString& fileName, const QByteArray& fileContent) {
            if (fileName.isEmpty())
                return;
            QImage* image = new QImage();
            if (!image->loadFromData(fileContent)) {
                delete image;
                return;
            }
            auto imageId = ImageForever::add(image);
            delete image;
            for (const auto& componentId : componentIds)
                emit setComponentColorImage(componentId, imageId);
            emit groupOperationAdded();
        });
#else
    QString fileName = QFileDialog::getOpenFileName(this, QString(), QString(),
        tr("Image Files (*.png *.jpg *.jpeg *.bmp)"))
                           .trimmed();
    if (fileName.isEmpty())
        return;
    QImage* image = new QImage();
    if (!image->load(fileName)) {
        delete image;
        return;
    }
    auto imageId = ImageForever::add(image);
    delete image;
    for (const auto& componentId : componentIds)
        emit setComponentColorImage(componentId, imageId);
    emit groupOperationAdded();
#endif
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

bool ComponentPropertyWidget::lastSideClosed()
{
    bool closed = false;
    std::map<bool, int> closeStateMap;
    for (const auto& componentId : m_componentIds) {
        const Document::Component* component = m_document->findComponent(componentId);
        if (nullptr == component)
            continue;
        closeStateMap[component->sideClosed]++;
    }
    if (!closeStateMap.empty()) {
        closed = std::max_element(closeStateMap.begin(), closeStateMap.end(),
            [](const std::map<bool, int>::value_type& a, const std::map<bool, int>::value_type& b) {
                return a.second < b.second;
            })->first;
    }
    return closed;
}

size_t ComponentPropertyWidget::lastTargetSegments()
{
    size_t targetSegments = 0;
    std::map<size_t, int> targetSegmentsMap;
    for (const auto& componentId : m_componentIds) {
        const Document::Component* component = m_document->findComponent(componentId);
        if (nullptr == component)
            continue;
        targetSegmentsMap[component->targetSegments]++;
    }
    if (!targetSegmentsMap.empty()) {
        targetSegments = std::max_element(targetSegmentsMap.begin(), targetSegmentsMap.end(),
            [](const std::map<size_t, int>::value_type& a, const std::map<size_t, int>::value_type& b) {
                return a.second < b.second;
            })->first;
    }
    return targetSegments;
}

bool ComponentPropertyWidget::lastFrontClosed()
{
    bool closed = false;
    std::map<bool, int> closeStateMap;
    for (const auto& componentId : m_componentIds) {
        const Document::Component* component = m_document->findComponent(componentId);
        if (nullptr == component)
            continue;
        closeStateMap[component->frontClosed]++;
    }
    if (!closeStateMap.empty()) {
        closed = std::max_element(closeStateMap.begin(), closeStateMap.end(),
            [](const std::map<bool, int>::value_type& a, const std::map<bool, int>::value_type& b) {
                return a.second < b.second;
            })->first;
    }
    return closed;
}

bool ComponentPropertyWidget::lastBackClosed()
{
    bool closed = false;
    std::map<bool, int> closeStateMap;
    for (const auto& componentId : m_componentIds) {
        const Document::Component* component = m_document->findComponent(componentId);
        if (nullptr == component)
            continue;
        closeStateMap[component->backClosed]++;
    }
    if (!closeStateMap.empty()) {
        closed = std::max_element(closeStateMap.begin(), closeStateMap.end(),
            [](const std::map<bool, int>::value_type& a, const std::map<bool, int>::value_type& b) {
                return a.second < b.second;
            })->first;
    }
    return closed;
}

float ComponentPropertyWidget::lastBackCloseDepthRatio()
{
    float value = 1.0f;
    for (const auto& componentId : m_componentIds) {
        const Document::Component* component = m_document->findComponent(componentId);
        if (nullptr == component)
            continue;
        value = component->backCloseDepthRatio;
        break;
    }
    return value;
}

bool ComponentPropertyWidget::hasGroupsOnly()
{
    for (const auto& componentId : m_componentIds) {
        const Document::Component* component = m_document->findComponent(componentId);
        if (nullptr == component || !component->linkToPartId.isNull())
            return false;
    }
    return !m_componentIds.empty();
}

QString ComponentPropertyWidget::lastWrapAttribute(const std::string& name)
{
    for (auto it = m_componentIds.rbegin(); it != m_componentIds.rend(); ++it) {
        const Document::Component* component = m_document->findComponent(*it);
        if (nullptr == component)
            continue;
        auto found = component->wrap.find(name);
        if (found != component->wrap.end())
            return QString::fromStdString(found->second);
        return QString();
    }
    return QString();
}

float ComponentPropertyWidget::lastBackCloseSharpness()
{
    float value = 0.0f;
    for (const auto& componentId : m_componentIds) {
        const Document::Component* component = m_document->findComponent(componentId);
        if (nullptr == component)
            continue;
        value = component->backCloseSharpness;
        break;
    }
    return value;
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
    for (const auto& componentId : m_componentIds) {
        const Document::Component* component = m_document->findComponent(componentId);
        if (nullptr == component)
            continue;
        degreesMap[dust3d::String::fromDouble(component->smoothCutoffDegrees)]++;
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
    if (nullptr == m_colorDialog) {
        m_colorDialog.reset(new QColorDialog(this));
        connect(m_colorDialog.get(), &QColorDialog::currentColorChanged, [this](const QColor& color) {
            if (!color.isValid())
                return;
            for (const auto& componentId : m_componentIds) {
                emit setComponentColorState(componentId, true, color);
            }
        });
        connect(m_colorDialog.get(), &QColorDialog::colorSelected, [this](const QColor& color) {
            if (!color.isValid())
                return;
            for (const auto& componentId : m_componentIds) {
                emit setComponentColorState(componentId, true, color);
            }
            emit endColorPicking();
            emit groupOperationAdded();
        });
        connect(m_colorDialog.get(), &QColorDialog::rejected, [this]() {
            for (const auto& componentId : m_componentIds) {
                emit setComponentColorState(componentId, true, m_componentsColorsBeforeColorPicking[componentId]);
                emit endColorPicking();
                emit groupOperationAdded();
            }
        });
    }
    m_componentsColorsBeforeColorPicking.clear();
    for (const auto& componentId : m_componentIds) {
        const Document::Component* component = m_document->findComponent(componentId);
        if (nullptr == component)
            continue;
        m_componentsColorsBeforeColorPicking[componentId] = component->color;
    }
    emit beginColorPicking();
    m_colorDialog->setCurrentColor(m_color);
    m_colorDialog->show();
    m_colorDialog->raise();
}

bool ComponentPropertyWidget::hasStitchingLineConfigure()
{
    for (const auto& componentId : m_componentIds) {
        const Document::Component* component = m_document->findComponent(componentId);
        if (nullptr != component) {
            for (const auto& childId : component->childrenIds) {
                const Document::Component* child = m_document->findComponent(childId);
                if (nullptr != child) {
                    const Document::Part* part = m_document->findPart(child->linkToPartId);
                    if (nullptr != part && dust3d::PartTarget::StitchingLine == part->target)
                        return true;
                }
            }
        }
    }
    return false;
}

bool ComponentPropertyWidget::hasStitchingLoopConfigure()
{
    for (const auto& componentId : m_componentIds) {
        const Document::Component* component = m_document->findComponent(componentId);
        if (nullptr != component) {
            for (const auto& childId : component->childrenIds) {
                const Document::Component* child = m_document->findComponent(childId);
                if (nullptr != child) {
                    const Document::Part* part = m_document->findPart(child->linkToPartId);
                    if (nullptr != part && dust3d::PartTarget::StitchingLoop == part->target)
                        return true;
                }
            }
        }
    }
    return false;
}
