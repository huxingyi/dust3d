#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QAction>
#include <QMenu>
#include <QWidgetAction>
#include <QColorDialog>
#include <QSizePolicy>
#include <QFileDialog>
#include <QSizePolicy>
#include <QCheckBox>
#include <QLabel>
#include <QRadioButton>
#include "partwidget.h"
#include "theme.h"
#include "floatnumberwidget.h"
#include "materiallistwidget.h"
#include "infolabel.h"
#include "cutface.h"
#include "skeletongraphicswidget.h"
#include "shortcuts.h"
#include "graphicscontainerwidget.h"
#include "flowlayout.h"
#include "imageforever.h"
#include "imagepreviewwidget.h"
#include "document.h"

PartWidget::PartWidget(const Document *document, QUuid partId) :
    m_document(document),
    m_partId(partId),
    m_unnormal(false)
{
    QSizePolicy retainSizePolicy = sizePolicy();
    retainSizePolicy.setRetainSizeWhenHidden(true);

    m_visibleButton = new QPushButton();
    m_visibleButton->setToolTip(tr("Show/hide nodes"));
    m_visibleButton->setSizePolicy(retainSizePolicy);
    initButton(m_visibleButton);
    
    m_smoothButton = new QPushButton();
    m_smoothButton->setToolTip(tr("Toggle smooth"));
    m_smoothButton->setSizePolicy(retainSizePolicy);
    initButton(m_smoothButton);
    
    m_lockButton = new QPushButton();
    m_lockButton->setToolTip(tr("Lock/unlock nodes"));
    m_lockButton->setSizePolicy(retainSizePolicy);
    initButton(m_lockButton);
    
    m_subdivButton = new QPushButton();
    m_subdivButton->setToolTip(tr("Subdivide"));
    m_subdivButton->setSizePolicy(retainSizePolicy);
    initButton(m_subdivButton);
    
    m_disableButton = new QPushButton();
    m_disableButton->setToolTip(tr("Join/Remove from final result"));
    m_disableButton->setSizePolicy(retainSizePolicy);
    initButton(m_disableButton);
    
    m_xMirrorButton = new QPushButton();
    m_xMirrorButton->setToolTip(tr("Toggle mirror"));
    m_xMirrorButton->setSizePolicy(retainSizePolicy);
    initButton(m_xMirrorButton);
    
    m_deformButton = new QPushButton();
    m_deformButton->setToolTip(tr("Deform"));
    m_deformButton->setSizePolicy(retainSizePolicy);
    initButton(m_deformButton);
    
    m_roundButton = new QPushButton;
    m_roundButton->setToolTip(tr("Toggle round end"));
    m_roundButton->setSizePolicy(retainSizePolicy);
    initButton(m_roundButton);
    
    m_chamferButton = new QPushButton;
    m_chamferButton->setToolTip(tr("Chamfer"));
    m_chamferButton->setSizePolicy(retainSizePolicy);
    initButton(m_chamferButton);
    
    m_colorButton = new QPushButton;
    m_colorButton->setToolTip(tr("Color and material picker"));
    m_colorButton->setSizePolicy(retainSizePolicy);
    initButton(m_colorButton);
    
    m_cutRotationButton = new QPushButton;
    m_cutRotationButton->setToolTip(tr("Cut face"));
    m_cutRotationButton->setSizePolicy(retainSizePolicy);
    initButton(m_cutRotationButton);
    
    m_previewLabel = new QLabel;
    m_previewLabel->setFixedSize(Theme::partPreviewImageSize, Theme::partPreviewImageSize);
    
    //m_previewWidget = new ModelWidget;
    //m_previewWidget->setAttribute(Qt::WA_TransparentForMouseEvents);
    //m_previewWidget->enableMove(false);
    //m_previewWidget->enableZoom(false);
    //m_previewWidget->setFixedSize(Theme::partPreviewImageSize, Theme::partPreviewImageSize);
    
    QWidget *hrLightWidget = new QWidget;
    hrLightWidget->setFixedHeight(1);
    hrLightWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    //hrLightWidget->setStyleSheet(QString("background-color: #565656;"));
    hrLightWidget->setContentsMargins(0, 0, 0, 0);
    
    QWidget *hrWidget = new QWidget;
    hrWidget->setFixedHeight(1);
    hrWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    //hrWidget->setStyleSheet(QString("background-color: #1a1a1a;"));
    hrWidget->setContentsMargins(0, 0, 0, 0);
    
    QGridLayout *toolsLayout = new QGridLayout;
    toolsLayout->setSpacing(0);
    toolsLayout->setContentsMargins(0, 0, 5, 0);
    int row = 0;
    int col = 0;
    toolsLayout->addWidget(m_smoothButton, row, col++, Qt::AlignBottom);
    toolsLayout->addWidget(m_lockButton, row, col++, Qt::AlignBottom);
    toolsLayout->addWidget(m_disableButton, row, col++, Qt::AlignBottom);
    toolsLayout->addWidget(m_xMirrorButton, row, col++, Qt::AlignBottom);
    toolsLayout->addWidget(m_colorButton, row, col++, Qt::AlignBottom);
    row++;
    col = 0;
    toolsLayout->addWidget(m_subdivButton, row, col++, Qt::AlignTop);
    toolsLayout->addWidget(m_roundButton, row, col++, Qt::AlignTop);
    //toolsLayout->addWidget(m_cutTemplateButton, row, col++, Qt::AlignTop);
    toolsLayout->addWidget(m_chamferButton, row, col++, Qt::AlignTop);
    toolsLayout->addWidget(m_cutRotationButton, row, col++, Qt::AlignTop);
    toolsLayout->addWidget(m_deformButton, row, col++, Qt::AlignTop);
    
    //m_visibleButton->setContentsMargins(0, 0, 0, 0);
    
    QHBoxLayout *previewAndToolsLayout = new QHBoxLayout;
    previewAndToolsLayout->setSpacing(0);
    previewAndToolsLayout->setContentsMargins(0, 0, 0, 0);
    previewAndToolsLayout->addWidget(m_visibleButton);
    //previewAndToolsLayout->addWidget(m_previewWidget);
    previewAndToolsLayout->addWidget(m_previewLabel);
    previewAndToolsLayout->addLayout(toolsLayout);
    previewAndToolsLayout->setStretch(0, 0);
    previewAndToolsLayout->setStretch(1, 0);
    previewAndToolsLayout->setStretch(2, 1);
    
    QWidget *backgroundWidget = new QWidget;
    backgroundWidget->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
    backgroundWidget->setFixedSize(preferredSize().width(), Theme::partPreviewImageSize);
    backgroundWidget->setObjectName("background");
    m_backgroundWidget = backgroundWidget;
    backgroundWidget->setLayout(previewAndToolsLayout);
    
    QHBoxLayout *backgroundLayout = new QHBoxLayout;
    backgroundLayout->setSpacing(0);
    backgroundLayout->setContentsMargins(0, 0, 0, 0);
    backgroundLayout->addWidget(backgroundWidget);
    backgroundLayout->addSpacing(10);

    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->setSpacing(0);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->addWidget(hrLightWidget);
    mainLayout->addSpacing(3);
    mainLayout->addLayout(backgroundLayout);
    mainLayout->addSpacing(3);
    mainLayout->addWidget(hrWidget);
    
    setLayout(mainLayout);
    
    connect(this, &PartWidget::setPartLockState, m_document, &Document::setPartLockState);
    connect(this, &PartWidget::setPartVisibleState, m_document, &Document::setPartVisibleState);
    connect(this, &PartWidget::setPartSubdivState, m_document, &Document::setPartSubdivState);
    connect(this, &PartWidget::setPartDisableState, m_document, &Document::setPartDisableState);
    connect(this, &PartWidget::setPartXmirrorState, m_document, &Document::setPartXmirrorState);
    connect(this, &PartWidget::setPartDeformThickness, m_document, &Document::setPartDeformThickness);
    connect(this, &PartWidget::setPartDeformWidth, m_document, &Document::setPartDeformWidth);
    connect(this, &PartWidget::setPartDeformUnified, m_document, &Document::setPartDeformUnified);
    connect(this, &PartWidget::setPartDeformMapImageId, m_document, &Document::setPartDeformMapImageId);
    connect(this, &PartWidget::setPartDeformMapScale, m_document, &Document::setPartDeformMapScale);
    connect(this, &PartWidget::setPartRoundState, m_document, &Document::setPartRoundState);
    connect(this, &PartWidget::setPartChamferState, m_document, &Document::setPartChamferState);
    connect(this, &PartWidget::setPartCutRotation, m_document, &Document::setPartCutRotation);
    connect(this, &PartWidget::setPartCutFace, m_document, &Document::setPartCutFace);
    connect(this, &PartWidget::setPartCutFaceLinkedId, m_document, &Document::setPartCutFaceLinkedId);
    connect(this, &PartWidget::setPartColorState, m_document, &Document::setPartColorState);
    connect(this, &PartWidget::setPartMaterialId, m_document, &Document::setPartMaterialId);
    connect(this, &PartWidget::setPartColorSolubility, m_document, &Document::setPartColorSolubility);
    connect(this, &PartWidget::setPartMetalness, m_document, &Document::setPartMetalness);
    connect(this, &PartWidget::setPartRoughness, m_document, &Document::setPartRoughness);
    connect(this, &PartWidget::setPartHollowThickness, m_document, &Document::setPartHollowThickness);
    connect(this, &PartWidget::setPartCountershaded, m_document, &Document::setPartCountershaded);
    connect(this, &PartWidget::setPartSmoothState, m_document, &Document::setPartSmoothState);
    connect(this, &PartWidget::checkPart, m_document, &Document::checkPart);
    connect(this, &PartWidget::enableBackgroundBlur, m_document, &Document::enableBackgroundBlur);
    connect(this, &PartWidget::disableBackgroundBlur, m_document, &Document::disableBackgroundBlur);
    
    connect(this, &PartWidget::groupOperationAdded, m_document, &Document::saveSnapshot);
    
    connect(m_smoothButton, &QPushButton::clicked, [=]() {
        const SkeletonPart *part = m_document->findPart(m_partId);
        if (!part) {
            qDebug() << "Part not found:" << m_partId;
            return;
        }
        emit setPartSmoothState(m_partId, !part->smooth);
        emit groupOperationAdded();
    });
    
    connect(m_lockButton, &QPushButton::clicked, [=]() {
        const SkeletonPart *part = m_document->findPart(m_partId);
        if (!part) {
            qDebug() << "Part not found:" << m_partId;
            return;
        }
        emit setPartLockState(m_partId, !part->locked);
        emit groupOperationAdded();
    });
    
    connect(m_visibleButton, &QPushButton::clicked, [=]() {
        const SkeletonPart *part = m_document->findPart(m_partId);
        if (!part) {
            qDebug() << "Part not found:" << m_partId;
            return;
        }
        emit setPartVisibleState(m_partId, !part->visible);
        emit groupOperationAdded();
    });
    
    connect(m_subdivButton, &QPushButton::clicked, [=]() {
        const SkeletonPart *part = m_document->findPart(m_partId);
        if (!part) {
            qDebug() << "Part not found:" << m_partId;
            return;
        }
        if (!part->hasSubdivFunction())
            return;
        emit setPartSubdivState(m_partId, !part->subdived);
        emit groupOperationAdded();
    });
    
    connect(m_disableButton, &QPushButton::clicked, [=]() {
        const SkeletonPart *part = m_document->findPart(m_partId);
        if (!part) {
            qDebug() << "Part not found:" << m_partId;
            return;
        }
        emit setPartDisableState(m_partId, !part->disabled);
        emit groupOperationAdded();
    });
    
    connect(m_xMirrorButton, &QPushButton::clicked, [=]() {
        const SkeletonPart *part = m_document->findPart(m_partId);
        if (!part) {
            qDebug() << "Part not found:" << m_partId;
            return;
        }
        if (!part->hasMirrorFunction())
            return;
        emit setPartXmirrorState(m_partId, !part->xMirrored);
        emit groupOperationAdded();
    });
    
    connect(m_deformButton, &QPushButton::clicked, [=]() {
        const SkeletonPart *part = m_document->findPart(m_partId);
        if (!part) {
            qDebug() << "Part not found:" << m_partId;
            return;
        }
        if (!part->hasDeformFunction())
            return;
        showDeformSettingPopup(mapFromGlobal(QCursor::pos()));
    });
    
    connect(m_roundButton, &QPushButton::clicked, [=]() {
        const SkeletonPart *part = m_document->findPart(m_partId);
        if (!part) {
            qDebug() << "Part not found:" << m_partId;
            return;
        }
        if (!part->hasRoundEndFunction())
            return;
        emit setPartRoundState(m_partId, !part->rounded);
        emit groupOperationAdded();
    });
    
    connect(m_chamferButton, &QPushButton::clicked, [=]() {
        const SkeletonPart *part = m_document->findPart(m_partId);
        if (!part) {
            qDebug() << "Part not found:" << m_partId;
            return;
        }
        if (!part->hasChamferFunction())
            return;
        emit setPartChamferState(m_partId, !part->chamfered);
        emit groupOperationAdded();
    });
    
    connect(m_colorButton, &QPushButton::clicked, [=]() {
        const SkeletonPart *part = m_document->findPart(m_partId);
        if (!part) {
            qDebug() << "Part not found:" << m_partId;
            return;
        }
        if (!part->hasColorFunction())
            return;
        showColorSettingPopup(mapFromGlobal(QCursor::pos()));
    });
    
    connect(m_cutRotationButton, &QPushButton::clicked, [=]() {
        const SkeletonPart *part = m_document->findPart(m_partId);
        if (!part) {
            qDebug() << "Part not found:" << m_partId;
            return;
        }
        if (!part->hasRotationFunction())
            return;
        showCutRotationSettingPopup(mapFromGlobal(QCursor::pos()));
    });
    
    setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
    setFixedSize(preferredSize());
    
    updateAllButtons();
}

ModelWidget *PartWidget::previewWidget()
{
    //return m_previewWidget;
    return nullptr;
}

QSize PartWidget::preferredSize()
{
    return QSize(Theme::miniIconSize + Theme::partPreviewImageSize + Theme::miniIconSize * 5 + 5 + 2, Theme::partPreviewImageSize + 6);
}

void PartWidget::updateAllButtons()
{
    updateVisibleButton();
    updateSmoothButton();
    updateLockButton();
    updateSubdivButton();
    updateDisableButton();
    updateXmirrorButton();
    updateDeformButton();
    updateRoundButton();
    updateChamferButton();
    updateColorButton();
    updateCutRotationButton();
}

void PartWidget::updateCheckedState(bool checked)
{
    if (checked)
        m_backgroundWidget->setStyleSheet("QWidget#background {border: 1px solid " + (m_unnormal ? Theme::blue.name() : Theme::red.name()) + ";}");
    else
        m_backgroundWidget->setStyleSheet("QWidget#background {border: 1px solid transparent;}");
}

void PartWidget::updateUnnormalState(bool unnormal)
{
    if (m_unnormal == unnormal)
        return;
    
    m_unnormal = unnormal;
    updateAllButtons();
}

//void PartWidget::mouseDoubleClickEvent(QMouseEvent *event)
//{
//    QWidget::mouseDoubleClickEvent(event);
//    emit checkPart(m_partId);
//}

void PartWidget::initToolButtonWithoutFont(QPushButton *button)
{
    Theme::initAwesomeToolButtonWithoutFont(button);
}

void PartWidget::initToolButton(QPushButton *button)
{
    Theme::initAwesomeToolButton(button);
}

void PartWidget::showColorSettingPopup(const QPoint &pos)
{
    QMenu popupMenu;
    
    const SkeletonPart *part = m_document->findPart(m_partId);
    if (!part) {
        qDebug() << "Find part failed:" << m_partId;
        return;
    }
    
    QWidget *popup = new QWidget;
    
    QPushButton *colorEraser = new QPushButton(QChar(fa::eraser));
    initToolButton(colorEraser);
    
    QPushButton *pickButton = new QPushButton();
    initToolButtonWithoutFont(pickButton);
    QPalette palette = pickButton->palette();
    QColor choosenColor = part->color;
    palette.setColor(QPalette::Window, choosenColor);
    palette.setColor(QPalette::Button, choosenColor);
    pickButton->setPalette(palette);
    
    QCheckBox *countershadeStateBox = new QCheckBox();
    Theme::initCheckbox(countershadeStateBox);
    countershadeStateBox->setText(tr("Countershaded"));
    countershadeStateBox->setChecked(part->countershaded);
    
    connect(countershadeStateBox, &QCheckBox::stateChanged, this, [=]() {
        emit setPartCountershaded(m_partId, countershadeStateBox->isChecked());
        emit groupOperationAdded();
    });
    
    QHBoxLayout *colorLayout = new QHBoxLayout;
    colorLayout->addWidget(colorEraser);
    colorLayout->addWidget(pickButton);
    colorLayout->addStretch();
    colorLayout->addWidget(countershadeStateBox);
    
    connect(colorEraser, &QPushButton::clicked, [=]() {
        emit setPartColorState(m_partId, false, Qt::white);
        emit groupOperationAdded();
    });
    
    connect(pickButton, &QPushButton::clicked, [=]() {
        emit disableBackgroundBlur();
        QColor color = QColorDialog::getColor(part->color, this);
        emit enableBackgroundBlur();
        if (color.isValid()) {
            const SkeletonPart *part = m_document->findPart(m_partId);
            if (nullptr == part) {
                return;
            }
            color.setAlphaF(part->color.alphaF());
            emit setPartColorState(m_partId, true, color);
            emit groupOperationAdded();
        }
    });
    
    FloatNumberWidget *colorTransparencyWidget = new FloatNumberWidget;
    colorTransparencyWidget->setItemName(tr("Transparency"));
    colorTransparencyWidget->setRange(0.0, 1.0);
    colorTransparencyWidget->setValue(1.0 - part->color.alphaF());
    
    connect(colorTransparencyWidget, &FloatNumberWidget::valueChanged, [=](float value) {
        const SkeletonPart *part = m_document->findPart(m_partId);
        if (nullptr == part) {
            return;
        }
        QColor color = part->color;
        color.setAlphaF(1.0 - value);
        emit setPartColorState(m_partId, true, color);
        emit groupOperationAdded();
    });
    
    QPushButton *colorTransparencyEraser = new QPushButton(QChar(fa::eraser));
    initToolButton(colorTransparencyEraser);
    
    connect(colorTransparencyEraser, &QPushButton::clicked, [=]() {
        colorTransparencyWidget->setValue(0.0);
        emit groupOperationAdded();
    });
    
    QHBoxLayout *colorTransparencyLayout = new QHBoxLayout;
    colorTransparencyLayout->addWidget(colorTransparencyEraser);
    colorTransparencyLayout->addWidget(colorTransparencyWidget);
    
    FloatNumberWidget *colorSolubilityWidget = new FloatNumberWidget;
    colorSolubilityWidget->setItemName(tr("Solubility"));
    colorSolubilityWidget->setRange(0.0, 1.0);
    colorSolubilityWidget->setValue(part->colorSolubility);
    
    connect(colorSolubilityWidget, &FloatNumberWidget::valueChanged, [=](float value) {
        emit setPartColorSolubility(m_partId, value);
        emit groupOperationAdded();
    });
    
    QPushButton *colorSolubilityEraser = new QPushButton(QChar(fa::eraser));
    initToolButton(colorSolubilityEraser);
    
    connect(colorSolubilityEraser, &QPushButton::clicked, [=]() {
        colorSolubilityWidget->setValue(0.0);
        emit groupOperationAdded();
    });
    
    QHBoxLayout *colorSolubilityLayout = new QHBoxLayout;
    colorSolubilityLayout->addWidget(colorSolubilityEraser);
    colorSolubilityLayout->addWidget(colorSolubilityWidget);
    
    FloatNumberWidget *metalnessWidget = new FloatNumberWidget;
    metalnessWidget->setItemName(tr("Metallic"));
    metalnessWidget->setRange(0.0, 1.0);
    metalnessWidget->setValue(part->metalness);
    
    connect(metalnessWidget, &FloatNumberWidget::valueChanged, [=](float value) {
        emit setPartMetalness(m_partId, value);
        emit groupOperationAdded();
    });
    
    QPushButton *metalnessEraser = new QPushButton(QChar(fa::eraser));
    initToolButton(metalnessEraser);
    
    connect(metalnessEraser, &QPushButton::clicked, [=]() {
        metalnessWidget->setValue(0.0);
        emit groupOperationAdded();
    });
    
    QHBoxLayout *metalnessLayout = new QHBoxLayout;
    metalnessLayout->addWidget(metalnessEraser);
    metalnessLayout->addWidget(metalnessWidget);
    
    FloatNumberWidget *roughnessWidget = new FloatNumberWidget;
    roughnessWidget->setItemName(tr("Roughness"));
    roughnessWidget->setRange(0.0, 1.0);
    roughnessWidget->setValue(part->roughness);
    
    connect(roughnessWidget, &FloatNumberWidget::valueChanged, [=](float value) {
        emit setPartRoughness(m_partId, value);
        emit groupOperationAdded();
    });
    
    QPushButton *roughnessEraser = new QPushButton(QChar(fa::eraser));
    initToolButton(roughnessEraser);
    
    connect(roughnessEraser, &QPushButton::clicked, [=]() {
        roughnessWidget->setValue(1.0);
        emit groupOperationAdded();
    });
    
    QHBoxLayout *roughnessLayout = new QHBoxLayout;
    roughnessLayout->addWidget(roughnessEraser);
    roughnessLayout->addWidget(roughnessWidget);
    
    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->addLayout(colorLayout);
    mainLayout->addLayout(colorTransparencyLayout);
    mainLayout->addLayout(colorSolubilityLayout);
    mainLayout->addWidget(Theme::createHorizontalLineWidget());
    mainLayout->addLayout(metalnessLayout);
    mainLayout->addLayout(roughnessLayout);
    
    if (m_document->materialIdList.empty()) {
        InfoLabel *infoLabel = new InfoLabel;
        infoLabel->setText(tr("Missing Materials"));
        mainLayout->addWidget(infoLabel);
    } else {
        MaterialListWidget *materialListWidget = new MaterialListWidget(m_document);
        materialListWidget->enableMultipleSelection(false);
        materialListWidget->selectMaterial(part->materialId);
        connect(materialListWidget, &MaterialListWidget::currentSelectedMaterialChanged, this, [=](QUuid materialId) {
            emit setPartMaterialId(m_partId, materialId);
            emit groupOperationAdded();
        });
        mainLayout->addWidget(materialListWidget);
    }
    
    popup->setLayout(mainLayout);
    
    QWidgetAction *action = new QWidgetAction(this);
    action->setDefaultWidget(popup);
    
    popupMenu.addAction(action);
    
    popupMenu.exec(mapToGlobal(pos));
}

void PartWidget::showCutRotationSettingPopup(const QPoint &pos)
{
    QMenu popupMenu;
    
    const SkeletonPart *part = m_document->findPart(m_partId);
    if (!part) {
        qDebug() << "Find part failed:" << m_partId;
        return;
    }
    
    QWidget *popup = new QWidget;
    
    FloatNumberWidget *rotationWidget = new FloatNumberWidget;
    rotationWidget->setItemName(tr("Rotation"));
    rotationWidget->setRange(-1, 1);
    rotationWidget->setValue(part->cutRotation);
    
    connect(rotationWidget, &FloatNumberWidget::valueChanged, [=](float value) {
        emit setPartCutRotation(m_partId, value);
        emit groupOperationAdded();
    });
    
    QPushButton *rotationEraser = new QPushButton(QChar(fa::eraser));
    initToolButton(rotationEraser);
    
    connect(rotationEraser, &QPushButton::clicked, [=]() {
        rotationWidget->setValue(0.0);
        emit groupOperationAdded();
    });
    
    QPushButton *rotationMinus5Button = new QPushButton(QChar(fa::rotateleft));
    initToolButton(rotationMinus5Button);
    
    connect(rotationMinus5Button, &QPushButton::clicked, [=]() {
        rotationWidget->setValue(-0.5);
        emit groupOperationAdded();
    });
    
    QPushButton *rotation5Button = new QPushButton(QChar(fa::rotateright));
    initToolButton(rotation5Button);
    
    connect(rotation5Button, &QPushButton::clicked, [=]() {
        rotationWidget->setValue(0.5);
        emit groupOperationAdded();
    });
    
    QHBoxLayout *rotationLayout = new QHBoxLayout;
    rotationLayout->addWidget(rotationEraser);
    rotationLayout->addWidget(rotationWidget);
    rotationLayout->addWidget(rotationMinus5Button);
    rotationLayout->addWidget(rotation5Button);
    
    QHBoxLayout *hollowThicknessLayout = nullptr;
    FlowLayout *cutFaceLayout = nullptr;

    if (part->hasHollowFunction()) {
        FloatNumberWidget *hollowThicknessWidget = new FloatNumberWidget;
        hollowThicknessWidget->setItemName(tr("Hollow"));
        hollowThicknessWidget->setRange(0.0, 1.0);
        hollowThicknessWidget->setValue(part->hollowThickness);
        
        connect(hollowThicknessWidget, &FloatNumberWidget::valueChanged, [=](float value) {
            emit setPartHollowThickness(m_partId, value);
            emit groupOperationAdded();
        });
        
        QPushButton *hollowThicknessEraser = new QPushButton(QChar(fa::eraser));
        initToolButton(hollowThicknessEraser);
        
        connect(hollowThicknessEraser, &QPushButton::clicked, [=]() {
            hollowThicknessWidget->setValue(0.0);
            emit groupOperationAdded();
        });
        
        hollowThicknessLayout = new QHBoxLayout;
        hollowThicknessLayout->addWidget(hollowThicknessEraser);
        hollowThicknessLayout->addWidget(hollowThicknessWidget);
    }
    
    std::vector<QPushButton *> buttons;
    std::vector<QString> cutFaceList;
    
    if (part->hasCutFaceFunction()) {
        cutFaceLayout = new FlowLayout(nullptr, 0, 0);

        auto updateCutFaceButtonState = [&](size_t index) {
            for (size_t i = 0; i < cutFaceList.size(); ++i) {
                auto button = buttons[i];
                if (i == index) {
                    button->setFlat(true);
                    button->setEnabled(false);
                } else {
                    button->setFlat(false);
                    button->setEnabled(true);
                }
            }
        };
        m_document->collectCutFaceList(cutFaceList);
        buttons.resize(cutFaceList.size());
        for (size_t i = 0; i < cutFaceList.size(); ++i) {
            QString cutFaceString = cutFaceList[i];
            CutFace cutFace;
            QUuid cutFacePartId(cutFaceString);
            QPushButton *button = new QPushButton;
            button->setIconSize(QSize(Theme::toolIconSize * 0.75, Theme::toolIconSize * 0.75));
            if (cutFacePartId.isNull()) {
                cutFace = CutFaceFromString(cutFaceString.toUtf8().constData());
                button->setIcon(QIcon(QPixmap::fromImage(*cutFacePreviewImage(cutFace))));
            } else {
                const SkeletonPart *part = m_document->findPart(cutFacePartId);
                if (nullptr != part) {
                    button->setIcon(QIcon(part->previewPixmap));
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
            cutFaceLayout->addWidget(button);
            buttons[i] = button;
        }
        for (size_t i = 0; i < cutFaceList.size(); ++i) {
            if (CutFace::UserDefined == part->cutFace) {
                if (part->cutFaceLinkedId.toString() == cutFaceList[i]) {
                    updateCutFaceButtonState(i);
                    break;
                }
            } else if (i < (int)CutFace::UserDefined) {
                if ((size_t)part->cutFace == i) {
                    updateCutFaceButtonState(i);
                    break;
                }
            }
        }
    }
    
    QVBoxLayout *popupLayout = new QVBoxLayout;
    popupLayout->addLayout(rotationLayout);
    if (nullptr != hollowThicknessLayout)
        popupLayout->addLayout(hollowThicknessLayout);
    if (nullptr != cutFaceLayout)
        popupLayout->addWidget(Theme::createHorizontalLineWidget());
    if (nullptr != cutFaceLayout)
        popupLayout->addLayout(cutFaceLayout);
    
    popup->setLayout(popupLayout);
    
    QWidgetAction action(this);
    action.setDefaultWidget(popup);
    
    popupMenu.addAction(&action);
    
    popupMenu.exec(mapToGlobal(pos));
}

void PartWidget::showDeformSettingPopup(const QPoint &pos)
{
    QMenu popupMenu;
    
    const SkeletonPart *part = m_document->findPart(m_partId);
    if (!part) {
        qDebug() << "Find part failed:" << m_partId;
        return;
    }
    
    QWidget *popup = new QWidget;
    
    FloatNumberWidget *thicknessWidget = new FloatNumberWidget;
    thicknessWidget->setItemName(tr("Thickness"));
    thicknessWidget->setRange(0, 2);
    thicknessWidget->setValue(part->deformThickness);
    
    connect(thicknessWidget, &FloatNumberWidget::valueChanged, [=](float value) {
        emit setPartDeformThickness(m_partId, value);
        emit groupOperationAdded();
    });
    
    FloatNumberWidget *widthWidget = new FloatNumberWidget;
    widthWidget->setItemName(tr("Width"));
    widthWidget->setRange(0, 2);
    widthWidget->setValue(part->deformWidth);
    
    connect(widthWidget, &FloatNumberWidget::valueChanged, [=](float value) {
        emit setPartDeformWidth(m_partId, value);
        emit groupOperationAdded();
    });
    
    QPushButton *thicknessEraser = new QPushButton(QChar(fa::eraser));
    initToolButton(thicknessEraser);
    
    connect(thicknessEraser, &QPushButton::clicked, [=]() {
        thicknessWidget->setValue(1.0);
        emit groupOperationAdded();
    });
    
    QPushButton *widthEraser = new QPushButton(QChar(fa::eraser));
    initToolButton(widthEraser);
    
    connect(widthEraser, &QPushButton::clicked, [=]() {
        widthWidget->setValue(1.0);
        emit groupOperationAdded();
    });
    
    QVBoxLayout *mainLayout = new QVBoxLayout;
    
    QHBoxLayout *thicknessLayout = new QHBoxLayout;
    QHBoxLayout *widthLayout = new QHBoxLayout;
    thicknessLayout->addWidget(thicknessEraser);
    thicknessLayout->addWidget(thicknessWidget);
    widthLayout->addWidget(widthEraser);
    widthLayout->addWidget(widthWidget);
    
    QCheckBox *deformUnifyStateBox = new QCheckBox();
    Theme::initCheckbox(deformUnifyStateBox);
    deformUnifyStateBox->setText(tr("Unified"));
    deformUnifyStateBox->setChecked(part->deformUnified);
    
    connect(deformUnifyStateBox, &QCheckBox::stateChanged, this, [=]() {
        emit setPartDeformUnified(m_partId, deformUnifyStateBox->isChecked());
        emit groupOperationAdded();
    });
    
    QHBoxLayout *deformUnifyLayout = new QHBoxLayout;
    deformUnifyLayout->addStretch();
    deformUnifyLayout->addWidget(deformUnifyStateBox);

    mainLayout->addLayout(thicknessLayout);
    mainLayout->addLayout(widthLayout);
    mainLayout->addLayout(deformUnifyLayout);
    
    popup->setLayout(mainLayout);
    
    QWidgetAction action(this);
    action.setDefaultWidget(popup);
    
    popupMenu.addAction(&action);
    
    popupMenu.exec(mapToGlobal(pos));
}

void PartWidget::initButton(QPushButton *button)
{
    Theme::initAwesomeMiniButton(button);
}

void PartWidget::updateButton(QPushButton *button, QChar icon, bool highlighted, bool enabled)
{
    Theme::updateAwesomeMiniButton(button, icon, highlighted, enabled, m_unnormal);
}

void PartWidget::updatePreview()
{
    const SkeletonPart *part = m_document->findPart(m_partId);
    if (!part) {
        qDebug() << "Part not found:" << m_partId;
        return;
    }
    m_previewLabel->setPixmap(part->previewPixmap);
}

void PartWidget::updateLockButton()
{
    const SkeletonPart *part = m_document->findPart(m_partId);
    if (!part) {
        qDebug() << "Part not found:" << m_partId;
        return;
    }
    if (part->locked)
        updateButton(m_lockButton, QChar(fa::lock), true);
    else
        updateButton(m_lockButton, QChar(fa::unlock), false);
}

void PartWidget::updateSmoothButton()
{
    const SkeletonPart *part = m_document->findPart(m_partId);
    if (!part) {
        qDebug() << "Part not found:" << m_partId;
        return;
    }
    if (part->smooth)
        updateButton(m_smoothButton, QChar(fa::headphones), true, part->hasSmoothFunction());
    else
        updateButton(m_smoothButton, QChar(fa::headphones), false, part->hasSmoothFunction());
}

void PartWidget::updateVisibleButton()
{
    const SkeletonPart *part = m_document->findPart(m_partId);
    if (!part) {
        qDebug() << "Part not found:" << m_partId;
        return;
    }
    if (part->visible)
        updateButton(m_visibleButton, QChar(fa::eye), false);
    else
        updateButton(m_visibleButton, QChar(fa::eyeslash), true);
}

void PartWidget::updateSubdivButton()
{
    const SkeletonPart *part = m_document->findPart(m_partId);
    if (!part) {
        qDebug() << "Part not found:" << m_partId;
        return;
    }
    if (part->subdived)
        updateButton(m_subdivButton, QChar(fa::circleo), true, part->hasSubdivFunction());
    else
        updateButton(m_subdivButton, QChar(fa::squareo), false, part->hasSubdivFunction());
}

void PartWidget::updateDisableButton()
{
    const SkeletonPart *part = m_document->findPart(m_partId);
    if (!part) {
        qDebug() << "Part not found:" << m_partId;
        return;
    }
    if (part->disabled)
        updateButton(m_disableButton, QChar(fa::unlink), true);
    else
        updateButton(m_disableButton, QChar(fa::link), false);
}

void PartWidget::updateXmirrorButton()
{
    const SkeletonPart *part = m_document->findPart(m_partId);
    if (!part) {
        qDebug() << "Part not found:" << m_partId;
        return;
    }
    if (part->xMirrored)
        updateButton(m_xMirrorButton, QChar(fa::balancescale), true, part->hasMirrorFunction());
    else
        updateButton(m_xMirrorButton, QChar(fa::balancescale), false, part->hasMirrorFunction());
}

void PartWidget::updateDeformButton()
{
    const SkeletonPart *part = m_document->findPart(m_partId);
    if (!part) {
        qDebug() << "Part not found:" << m_partId;
        return;
    }
    if (part->deformAdjusted() || part->deformMapAdjusted() || part->deformUnified)
        updateButton(m_deformButton, QChar(fa::handlizardo), true, part->hasDeformFunction());
    else
        updateButton(m_deformButton, QChar(fa::handlizardo), false, part->hasDeformFunction());
}

void PartWidget::updateRoundButton()
{
    const SkeletonPart *part = m_document->findPart(m_partId);
    if (!part) {
        qDebug() << "Part not found:" << m_partId;
        return;
    }
    if (part->rounded)
        updateButton(m_roundButton, QChar(fa::magnet), true, part->hasRoundEndFunction());
    else
        updateButton(m_roundButton, QChar(fa::magnet), false, part->hasRoundEndFunction());
}

void PartWidget::updateChamferButton()
{
    const SkeletonPart *part = m_document->findPart(m_partId);
    if (!part) {
        qDebug() << "Part not found:" << m_partId;
        return;
    }
    if (part->chamfered)
        updateButton(m_chamferButton, QChar(fa::scissors), true, part->hasChamferFunction());
    else
        updateButton(m_chamferButton, QChar(fa::scissors), false, part->hasChamferFunction());
}

void PartWidget::updateColorButton()
{
    const SkeletonPart *part = m_document->findPart(m_partId);
    if (!part) {
        qDebug() << "Part not found:" << m_partId;
        return;
    }
    if (part->hasColor || 
            part->materialAdjusted() || 
            part->colorSolubilityAdjusted() || 
            part->countershaded ||
            part->metalnessAdjusted() ||
            part->roughnessAdjusted()) {
        updateButton(m_colorButton, QChar(fa::eyedropper), true, part->hasColorFunction());
    } else {
        updateButton(m_colorButton, QChar(fa::eyedropper), false, part->hasColorFunction());
    }
}

void PartWidget::updateCutRotationButton()
{
    const SkeletonPart *part = m_document->findPart(m_partId);
    if (!part) {
        qDebug() << "Part not found:" << m_partId;
        return;
    }
    if (part->cutAdjusted())
        updateButton(m_cutRotationButton, QChar(fa::spinner), true, part->hasRotationFunction());
    else
        updateButton(m_cutRotationButton, QChar(fa::spinner), false, part->hasRotationFunction());
}

void PartWidget::reload()
{
    updatePreview();
    updateAllButtons();
}
