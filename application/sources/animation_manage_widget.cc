#include "animation_manage_widget.h"
#include "animation_parameter_table.h"
#include "document.h"
#include "monochrome_mesh.h"
#include "theme.h"
#include "toolbar_button.h"
#include <QAudioFormat>
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
#include <QAudioSink>
#else
#include <QAudioOutput>
#endif
#include <QBuffer>
#include <QComboBox>
#include <QDebug>
#include <QFile>
#include <QFileDialog>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QListWidgetItem>
#include <QScrollArea>
#include <QVBoxLayout>
#include <algorithm>

AnimationManageWidget::AnimationManageWidget(Document* document, QWidget* parent)
    : QWidget(parent)
    , m_document(document)
    , m_modelWidget(new WorldWidget(this))
    , m_frameTimer(new QTimer(this))
{
    QVBoxLayout* mainLayout = new QVBoxLayout;
    mainLayout->setSpacing(5);
    mainLayout->setContentsMargins(5, 5, 5, 5);

    m_modelWidget->setMinimumHeight(280);
    m_modelWidget->enableZoom(true);
    m_modelWidget->enableMove(true);
    m_modelWidget->setMoveAndZoomByWindow(false);
    m_modelWidget->toggleWireframe();

    setLayout(mainLayout);

    createParameterWidgets();

    connect(m_frameTimer, &QTimer::timeout, this, &AnimationManageWidget::onAnimationFrameTimeout);

    if (m_document) {
        connect(m_document, &Document::resultRigChanged, this, &AnimationManageWidget::onResultRigChanged);
    }
}

void AnimationManageWidget::createParameterWidgets()
{
    QVBoxLayout* topLayout = qobject_cast<QVBoxLayout*>(layout());
    if (!topLayout)
        return;

    m_animationListWidget = new QListWidget;
    m_animationListWidget->setMaximumHeight(120);
    topLayout->addWidget(m_animationListWidget);
    connect(m_animationListWidget, &QListWidget::itemSelectionChanged, this, &AnimationManageWidget::onAnimationListSelectionChanged);

    QHBoxLayout* animationLayout = new QHBoxLayout;
    animationLayout->setSpacing(0);
    animationLayout->setContentsMargins(0, 0, 0, 0);
    m_animationNameCombo = new QComboBox;
    m_animationNameCombo->setSizeAdjustPolicy(QComboBox::AdjustToContents);
    m_animationNameCombo->setMinimumContentsLength(8);
    m_animationNameCombo->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    m_addAnimationButton = new ToolbarButton(":/resources/toolbar_add.svg");
    m_addAnimationButton->setToolTip(tr("Add new animation"));
    m_deleteAnimationButton = new QPushButton();
    Theme::initIconButton(m_deleteAnimationButton);
    m_deleteAnimationButton->setIcon(Theme::awesome()->icon(fa::remove));
    m_deleteAnimationButton->setToolTip(tr("Delete current animation"));
    m_duplicateAnimationButton = new QPushButton();
    Theme::initIconButton(m_duplicateAnimationButton);
    m_duplicateAnimationButton->setIcon(Theme::awesome()->icon(fa::copy));
    m_duplicateAnimationButton->setToolTip(tr("Duplicate current animation"));
    m_deleteAnimationButton->setEnabled(false);
    m_duplicateAnimationButton->setEnabled(false);

    animationLayout->addWidget(m_animationNameCombo);
    animationLayout->addWidget(m_addAnimationButton);
    animationLayout->addStretch();
    animationLayout->addWidget(m_deleteAnimationButton);
    animationLayout->addWidget(m_duplicateAnimationButton);
    topLayout->addLayout(animationLayout);

    // Groupbox containing model widget and parameter controls
    m_parametersGroupBox = new QGroupBox(tr("Parameters (New)"));
    QVBoxLayout* groupBoxLayout = new QVBoxLayout;
    m_parametersGroupBox->setLayout(groupBoxLayout);

    groupBoxLayout->addWidget(m_modelWidget);

    // Preview-specific options (not animation parameters)
    m_hideBonesCheck = new QCheckBox("Hide Bones");
    m_hideBonesCheck->setChecked(true);
    m_hidePartsCheck = new QCheckBox("Hide Parts");
    m_hidePartsCheck->setChecked(false);
    m_hideWeightsCheck = new QCheckBox("Hide Weights");
    m_hideWeightsCheck->setToolTip("Weights show when a bone is selected, unless they have no effect");
    m_hideWeightsCheck->setChecked(true);

    QWidget* previewOptionWidget = new QWidget;
    QHBoxLayout* previewOptionLayout = new QHBoxLayout(previewOptionWidget);
    previewOptionLayout->setContentsMargins(0, 0, 0, 0);
    previewOptionLayout->addWidget(m_hideBonesCheck);
    previewOptionLayout->addWidget(m_hidePartsCheck);
    previewOptionLayout->addWidget(m_hideWeightsCheck);
    previewOptionLayout->addStretch();
    groupBoxLayout->addWidget(previewOptionWidget);

    // Sound controls
    QWidget* soundOptionWidget = new QWidget;
    QHBoxLayout* soundOptionLayout = new QHBoxLayout(soundOptionWidget);
    soundOptionLayout->setContentsMargins(0, 0, 0, 0);

    m_playSoundCheck = new QCheckBox("Play Sound");
    m_playSoundCheck->setChecked(false);
    m_playSoundCheck->setToolTip(tr("Play procedural sound effects with animation preview"));
    soundOptionLayout->addWidget(m_playSoundCheck);

    m_surfaceMaterialCombo = new QComboBox;
    m_surfaceMaterialCombo->setSizeAdjustPolicy(QComboBox::AdjustToContents);
    m_surfaceMaterialCombo->addItem("Stone");
    m_surfaceMaterialCombo->addItem("Wood");
    m_surfaceMaterialCombo->addItem("Sand");
    m_surfaceMaterialCombo->addItem("Metal");
    m_surfaceMaterialCombo->addItem("Grass");
    m_surfaceMaterialCombo->addItem("Water");
    m_surfaceMaterialCombo->addItem("Dirt");
    m_surfaceMaterialCombo->addItem("Snow");
    m_surfaceMaterialCombo->setToolTip(tr("Surface material for sound generation"));
    soundOptionLayout->addWidget(m_surfaceMaterialCombo);

    m_exportAudioButton = new QPushButton(tr("Export WAV"));
    m_exportAudioButton->setToolTip(tr("Export generated sound effect as WAV file"));
    m_exportAudioButton->setEnabled(false);
    soundOptionLayout->addWidget(m_exportAudioButton);

    soundOptionLayout->addStretch();
    groupBoxLayout->addWidget(soundOptionWidget);

    // Preview timeline slider
    m_animationFrameSlider = new QSlider(Qt::Horizontal);
    m_animationFrameSlider->setMinimum(0);
    m_animationFrameSlider->setMaximum(0);
    m_animationFrameSlider->setEnabled(false);
    m_animationFrameSlider->setToolTip(tr("Drag to scrub animation frames"));

    m_playPauseButton = new QPushButton;
    m_playPauseButton->setIcon(Theme::awesome()->icon(fa::play));
    m_playPauseButton->setToolTip(tr("Play/Pause animation"));
    m_playPauseButton->setFixedSize(Theme::toolIconSize, Theme::toolIconSize);
    m_playPauseButton->setEnabled(false);

    QWidget* frameSliderWidget = new QWidget;
    QHBoxLayout* frameSliderLayout = new QHBoxLayout(frameSliderWidget);
    frameSliderLayout->setContentsMargins(0, 0, 0, 0);
    frameSliderLayout->addWidget(m_playPauseButton);
    frameSliderLayout->addWidget(m_animationFrameSlider);

    groupBoxLayout->addWidget(frameSliderWidget);

    // Parameters area in form layout
    QFormLayout* parameterLayout = new QFormLayout;
    m_parameterLayout = parameterLayout;
    parameterLayout->setLabelAlignment(Qt::AlignRight | Qt::AlignVCenter);
    parameterLayout->setFormAlignment(Qt::AlignLeft | Qt::AlignTop);
    parameterLayout->setContentsMargins(0, 0, 0, 0);
    parameterLayout->setSpacing(6);

    m_animationNameInput = new QLineEdit;
    m_animationNameInput->setPlaceholderText(tr("Enter animation name"));
    parameterLayout->addRow(new QLabel(tr("Name:")), m_animationNameInput);

    // Common parameters (visible for all animation types)
    m_sharedDurationSpinBox = new QDoubleSpinBox;
    m_sharedDurationSpinBox->setRange(0.01, 59.99);
    m_sharedDurationSpinBox->setDecimals(2);
    m_sharedDurationSpinBox->setSingleStep(0.1);
    m_sharedDurationSpinBox->setValue(3.0);
    m_sharedDurationSpinBox->setSuffix(tr(" s"));
    m_sharedDurationSpinBox->setToolTip(tr("Duration in seconds (0.01 – 60)"));
    parameterLayout->addRow(new QLabel(tr("Duration:")), m_sharedDurationSpinBox);

    m_sharedFrameCountSpinBox = new QSpinBox;
    m_sharedFrameCountSpinBox->setRange(1, 4999);
    m_sharedFrameCountSpinBox->setSingleStep(5);
    m_sharedFrameCountSpinBox->setValue(90);
    m_sharedFrameCountSpinBox->setToolTip(tr("Number of frames (1 – 4999)"));
    parameterLayout->addRow(new QLabel(tr("Frame Count:")), m_sharedFrameCountSpinBox);

    // Dynamic parameter rows will be added here by rebuildDynamicControls()

    // Add parameter layout to a scrollable area
    QWidget* scrollAreaWidget = new QWidget;
    scrollAreaWidget->setLayout(parameterLayout);
    QScrollArea* parameterScrollArea = new QScrollArea;
    parameterScrollArea->setWidget(scrollAreaWidget);
    parameterScrollArea->setWidgetResizable(true);
    groupBoxLayout->addWidget(parameterScrollArea, 1);

    topLayout->addWidget(m_parametersGroupBox, 1);
    m_parametersGroupBox->hide();

    m_bottomStretch = new QWidget;
    m_bottomStretch->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
    topLayout->addWidget(m_bottomStretch);

    // Connect non-dynamic controls
    connect(m_hideBonesCheck, &QCheckBox::toggled, this, &AnimationManageWidget::onParameterChanged);
    connect(m_hidePartsCheck, &QCheckBox::toggled, this, &AnimationManageWidget::onParameterChanged);
    connect(m_hideWeightsCheck, &QCheckBox::toggled, this, &AnimationManageWidget::onParameterChanged);

    connect(m_playSoundCheck, &QCheckBox::toggled, this, &AnimationManageWidget::onParameterChanged);
    connect(m_surfaceMaterialCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &AnimationManageWidget::onParameterChanged);
    connect(m_exportAudioButton, &QPushButton::clicked, this, &AnimationManageWidget::onExportAudioClicked);

    if (m_sharedDurationSpinBox)
        connect(m_sharedDurationSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &AnimationManageWidget::onParameterChanged);
    if (m_sharedFrameCountSpinBox)
        connect(m_sharedFrameCountSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, &AnimationManageWidget::onParameterChanged);

    if (m_animationFrameSlider) {
        connect(m_animationFrameSlider, &QSlider::sliderPressed, this, [this]() {
            m_isScrubbing = true;
            stopAnimationLoop();
        });
        connect(m_animationFrameSlider, &QSlider::sliderMoved, this, [this](int value) {
            if (value >= 0 && value < (int)m_animationFrames.size()) {
                m_currentFrame = value;
                // Update ground offset based on frame position
                if (m_movementSpeed > 0.0f && m_modelWidget) {
                    double durationSeconds = m_sharedDurationSpinBox ? m_sharedDurationSpinBox->value() : 1.0;
                    int frameCount = (int)m_animationFrames.size();
                    if (frameCount > 0 && durationSeconds > 0.0) {
                        float timeAtFrame = (float)(durationSeconds * value / frameCount);
                        m_groundOffsetX = m_movementDirectionX * m_movementSpeed * timeAtFrame;
                        m_groundOffsetZ = m_movementDirectionZ * m_movementSpeed * timeAtFrame;
                        m_modelWidget->setGroundOffset(m_groundOffsetX, m_groundOffsetZ);
                    }
                }
                displayCurrentFrame();
            }
        });
        connect(m_animationFrameSlider, &QSlider::sliderReleased, this, [this]() {
            m_isScrubbing = false;
            if (m_animationFrameSlider) {
                m_currentFrame = m_animationFrameSlider->value();
            }
        });
    }

    if (m_playPauseButton) {
        connect(m_playPauseButton, &QPushButton::clicked, this, &AnimationManageWidget::onPlayPauseClicked);
    }

    connect(m_animationNameInput, &QLineEdit::textEdited, this, &AnimationManageWidget::onAnimationNameEdited);
    connect(m_deleteAnimationButton, &QPushButton::clicked, this, &AnimationManageWidget::onDeleteAnimationClicked);
    connect(m_duplicateAnimationButton, &QPushButton::clicked, this, &AnimationManageWidget::onDuplicateAnimationClicked);
    connect(m_addAnimationButton, &QPushButton::clicked, this, &AnimationManageWidget::onAddAnimationClicked);

    if (m_document) {
        updateAnimationNameForRigType(m_document->getRigType());
        connect(m_document, &Document::rigTypeChanged, this, &AnimationManageWidget::onRigTypeChanged);
        connect(m_document, &Document::animationAdded, this, &AnimationManageWidget::refreshAnimationList);
        connect(m_document, &Document::animationRemoved, this, &AnimationManageWidget::refreshAnimationList);
        connect(m_document, &Document::animationNameChanged, this, &AnimationManageWidget::refreshAnimationList);
        connect(m_document, &Document::animationsChanged, this, &AnimationManageWidget::refreshAnimationList);
    } else {
        updateAnimationNameForRigType(QString());
    }

    updateAnimationParamsFromWidgets();
}

void AnimationManageWidget::rebuildDynamicControls(const QString& animationType)
{
    // Remove existing dynamic controls from the form layout
    for (auto& ctrl : m_dynamicControls) {
        if (ctrl.rowLabel) {
            m_parameterLayout->removeWidget(ctrl.rowLabel);
            delete ctrl.rowLabel;
        }
        if (ctrl.rowWidget) {
            m_parameterLayout->removeWidget(ctrl.rowWidget);
            delete ctrl.rowWidget;
        }
    }
    m_dynamicControls.clear();

    const auto& defs = getAnimationParameterDefs(animationType.toStdString());
    for (const auto& def : defs) {
        QSlider* slider = new QSlider;
        slider->setOrientation(Qt::Horizontal);
        slider->setRange(def.sliderMin, def.sliderMax);
        slider->setValue(def.defaultSliderValue);
        slider->setSingleStep(1);
        slider->setFocusPolicy(Qt::NoFocus);

        QWidget* rowWidget = new QWidget;
        QHBoxLayout* rowLayout = new QHBoxLayout(rowWidget);
        rowLayout->setContentsMargins(0, 0, 0, 0);
        rowLayout->addWidget(slider);

        QLabel* valueLabel = new QLabel;
        valueLabel->setFixedWidth(36);
        valueLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        if (def.toParam)
            valueLabel->setText(QString::number(def.toParam(def.defaultSliderValue), 'f', 2));
        rowLayout->addWidget(valueLabel);

        QLabel* label = new QLabel(QString::fromStdString(def.displayName));
        m_parameterLayout->addRow(label, rowWidget);

        DynamicParameterControl ctrl;
        ctrl.paramName = def.paramName;
        ctrl.slider = slider;
        ctrl.rowWidget = rowWidget;
        ctrl.rowLabel = label;
        ctrl.valueLabel = valueLabel;
        ctrl.toParam = def.toParam;
        ctrl.fromParam = def.fromParam;
        ctrl.defaultParamValue = def.defaultParamValue;
        m_dynamicControls.push_back(ctrl);

        connect(slider, &QSlider::valueChanged, this, [this, valueLabel, toParam = def.toParam](int value) {
            if (valueLabel && toParam)
                valueLabel->setText(QString::number(toParam(value), 'f', 2));
        });
        connect(slider, &QSlider::valueChanged, this, &AnimationManageWidget::onParameterChanged);
    }
}

void AnimationManageWidget::displayCurrentFrame()
{
    if (m_animationFrames.empty() || !m_modelWidget)
        return;

    if (m_currentFrame < 0 || m_currentFrame >= (int)m_animationFrames.size())
        m_currentFrame = 0;

    ModelMesh& frameSource = m_animationFrames[m_currentFrame];
    if (m_modelWidget->isWireframeVisible()) {
        int skeletonCount = frameSource.skeletonVertexCount();
        int totalCount = frameSource.triangleVertexCount();
        int skinnedCount = totalCount > skeletonCount ? totalCount - skeletonCount : 0;

        if (skinnedCount > 0) {
            m_modelWidget->updateWireframeMesh(
                new MonochromeMesh(frameSource.triangleVertices() + skeletonCount, skinnedCount, 1.0, 1.0, 1.0, 0.5));
        } else {
            m_modelWidget->updateWireframeMesh(nullptr);
        }
    }
    m_modelWidget->updateMesh(new ModelMesh(frameSource));

    if (m_animationFrameSlider && !m_isScrubbing) {
        m_animationFrameSlider->blockSignals(true);
        m_animationFrameSlider->setValue(m_currentFrame);
        m_animationFrameSlider->blockSignals(false);
    }
}

void AnimationManageWidget::updateAnimationNameForRigType(const QString& rigType)
{
    if (!m_animationNameCombo || !m_addAnimationButton)
        return;

    m_animationNameCombo->clear();
    if (rigType.compare("Insect", Qt::CaseInsensitive) == 0) {
        m_animationNameCombo->addItem("InsectIdle");
        m_animationNameCombo->addItem("InsectRubHands");
        m_animationNameCombo->addItem("InsectWalk");
        m_animationNameCombo->addItem("InsectForward");
        m_animationNameCombo->addItem("InsectAttack");
        m_animationNameCombo->addItem("InsectDie");
        m_animationNameCombo->setEnabled(true);
        m_addAnimationButton->setEnabled(true);
    } else if (rigType.compare("Bird", Qt::CaseInsensitive) == 0) {
        m_animationNameCombo->addItem("BirdIdle");
        m_animationNameCombo->addItem("BirdForward");
        m_animationNameCombo->addItem("BirdGlide");
        m_animationNameCombo->addItem("BirdWalk");
        m_animationNameCombo->addItem("BirdRun");
        m_animationNameCombo->addItem("BirdAttack");
        m_animationNameCombo->addItem("BirdEat");
        m_animationNameCombo->addItem("BirdDie");
        m_animationNameCombo->setEnabled(true);
        m_addAnimationButton->setEnabled(true);
    } else if (rigType.compare("Fish", Qt::CaseInsensitive) == 0) {
        m_animationNameCombo->addItem("FishIdle");
        m_animationNameCombo->addItem("FishForward");
        m_animationNameCombo->addItem("FishDie");
        m_animationNameCombo->setEnabled(true);
        m_addAnimationButton->setEnabled(true);
    } else if (rigType.compare("Biped", Qt::CaseInsensitive) == 0) {
        m_animationNameCombo->addItem("BipedIdle");
        m_animationNameCombo->addItem("BipedWalk");
        m_animationNameCombo->addItem("BipedRun");
        m_animationNameCombo->addItem("BipedJump");
        m_animationNameCombo->addItem("BipedRoar");
        m_animationNameCombo->addItem("BipedDie");
        m_animationNameCombo->setEnabled(true);
        m_addAnimationButton->setEnabled(true);
    } else if (rigType.compare("Quadruped", Qt::CaseInsensitive) == 0) {
        m_animationNameCombo->addItem("QuadrupedIdle");
        m_animationNameCombo->addItem("QuadrupedWalk");
        m_animationNameCombo->addItem("QuadrupedRun");
        m_animationNameCombo->addItem("QuadrupedDie");
        m_animationNameCombo->addItem("QuadrupedAttack");
        m_animationNameCombo->addItem("QuadrupedHurt");
        m_animationNameCombo->addItem("QuadrupedEat");
        m_animationNameCombo->setEnabled(true);
        m_addAnimationButton->setEnabled(true);
    } else if (rigType.compare("Spider", Qt::CaseInsensitive) == 0) {
        m_animationNameCombo->addItem("SpiderIdle");
        m_animationNameCombo->addItem("SpiderWalk");
        m_animationNameCombo->addItem("SpiderRun");
        m_animationNameCombo->addItem("SpiderDie");
        m_animationNameCombo->setEnabled(true);
        m_addAnimationButton->setEnabled(true);
    } else if (rigType.compare("Snake", Qt::CaseInsensitive) == 0) {
        m_animationNameCombo->addItem("SnakeIdle");
        m_animationNameCombo->addItem("SnakeForward");
        m_animationNameCombo->addItem("SnakeDie");
        m_animationNameCombo->setEnabled(true);
        m_addAnimationButton->setEnabled(true);
    } else {
        m_animationNameCombo->addItem("N/A");
        m_animationNameCombo->setEnabled(false);
        m_addAnimationButton->setEnabled(false);
    }
}

void AnimationManageWidget::onRigTypeChanged(const QString& rigType)
{
    updateAnimationNameForRigType(rigType);
    triggerPreviewRegeneration();
}

void AnimationManageWidget::updateAnimationParamsFromWidgets()
{
    // Common parameters
    if (m_sharedDurationSpinBox)
        m_animationParams.setValue("durationSeconds", m_sharedDurationSpinBox->value());
    if (m_sharedFrameCountSpinBox)
        m_animationParams.setValue("frameCount", m_sharedFrameCountSpinBox->value());

    // Surface material
    if (m_surfaceMaterialCombo)
        m_animationParams.values["surfaceMaterial"] = m_surfaceMaterialCombo->currentText().toStdString();

    // Dynamic parameters
    for (const auto& ctrl : m_dynamicControls) {
        if (ctrl.slider && ctrl.toParam)
            m_animationParams.setValue(ctrl.paramName, ctrl.toParam(ctrl.slider->value()));
    }
}

void AnimationManageWidget::triggerPreviewRegeneration()
{
    if (!m_hideBonesCheck || !m_hidePartsCheck || !m_hideWeightsCheck)
        return;

    updateAnimationParamsFromWidgets();

    onResultRigChanged();
}

AnimationManageWidget::~AnimationManageWidget()
{
    stopSoundPlayback();
    stopAnimationLoop();
}

void AnimationManageWidget::setWireframeVisible(bool visible)
{
    if (m_modelWidget)
        m_modelWidget->setWireframeVisible(visible);
}

void AnimationManageWidget::updateParametersGroupBoxTitle()
{
    QString name = m_animationNameInput ? m_animationNameInput->text().trimmed() : QString();
    QString type = m_currentAnimationType;
    if (!name.isEmpty() && !type.isEmpty())
        m_parametersGroupBox->setTitle(QString("%1 - %2").arg(name, type));
    else if (!name.isEmpty())
        m_parametersGroupBox->setTitle(name);
    else if (!type.isEmpty())
        m_parametersGroupBox->setTitle(type);
    else
        m_parametersGroupBox->setTitle(tr("Parameters"));
}

void AnimationManageWidget::updatePlayPauseIcon()
{
    if (!m_playPauseButton)
        return;

    if (m_frameTimer && m_frameTimer->isActive())
        m_playPauseButton->setIcon(Theme::awesome()->icon(fa::pause));
    else
        m_playPauseButton->setIcon(Theme::awesome()->icon(fa::play));
}

void AnimationManageWidget::onPlayPauseClicked()
{
    if (!m_frameTimer)
        return;

    if (m_frameTimer->isActive()) {
        stopAnimationLoop();
    } else {
        startAnimationLoop();
    }
    updatePlayPauseIcon();
}

void AnimationManageWidget::onResultRigChanged()
{
    qDebug() << "AnimationManageWidget: resultRigChanged";

    if (!m_document) {
        qWarning() << "AnimationManageWidget: missing document";
        return;
    }

    stopAnimationLoop();
    m_animationFrames.clear();
    m_currentFrame = 0;

    updateAnimationParamsFromWidgets();

    if (m_animationWorkerBusy) {
        m_animationRegenerationPending = true;
        return;
    }

    if (!m_animationWorker) {
        m_animationWorker = std::make_unique<AnimationPreviewWorker>();
    }

    const RigStructure& actualRig = m_document->getActualRigStructure();
    if (actualRig.bones.empty()) {
        qWarning() << "AnimationManageWidget: no rig bones available";
        return;
    }

    const auto* anim = m_document->findAnimation(m_currentAnimationId);
    QString animationType;
    if (anim) {
        animationType = anim->type;
    }
    if (animationType.isEmpty()) {
        qWarning() << "AnimationManageWidget: no animation available for preview";
        return;
    }

    m_animationWorker->setParameters(actualRig, animationType.toStdString(), m_animationParams);
    m_animationWorker->setHideBones(m_hideBonesCheck ? m_hideBonesCheck->isChecked() : false);
    m_animationWorker->setHideParts(m_hidePartsCheck ? m_hidePartsCheck->isChecked() : false);
    m_animationWorker->setSelectedBoneName(
        (m_hideWeightsCheck && m_hideWeightsCheck->isChecked()) ? QString() : m_selectedBoneName);

    // Sound settings
    bool soundEnabled = m_playSoundCheck && m_playSoundCheck->isChecked();
    m_animationWorker->setSoundEnabled(soundEnabled);
    if (soundEnabled && m_surfaceMaterialCombo) {
        m_animationWorker->setSurfaceMaterial(
            dust3d::surfaceMaterialFromName(m_surfaceMaterialCombo->currentText().toStdString()));
    }

    dust3d::Object* rigObject = m_document->takeRigObject();
    m_animationWorker->setRigObject(std::unique_ptr<dust3d::Object>(rigObject));

    auto thread = new QThread;
    m_animationWorker->moveToThread(thread);

    m_animationWorkerBusy = true;

    connect(thread, &QThread::started, m_animationWorker.get(), &AnimationPreviewWorker::process);
    connect(m_animationWorker.get(), &AnimationPreviewWorker::finished, this, &AnimationManageWidget::onAnimationPreviewReady);
    connect(m_animationWorker.get(), &AnimationPreviewWorker::finished, thread, &QThread::quit);
    connect(thread, &QThread::finished, thread, &QThread::deleteLater);

    thread->start();
}

void AnimationManageWidget::onAnimationPreviewReady()
{
    m_animationWorkerBusy = false;

    if (!m_animationWorker) {
        qWarning() << "AnimationManageWidget: preview worker finished but missing worker";
    } else {
        m_animationFrames = m_animationWorker->takePreviewMeshes();
        m_soundData = m_animationWorker->takeSoundData();
        m_movementSpeed = m_animationWorker->movementSpeed();
        m_movementDirectionX = m_animationWorker->movementDirectionX();
        m_movementDirectionZ = m_animationWorker->movementDirectionZ();
        m_groundOffsetX = 0.0f;
        m_groundOffsetZ = 0.0f;
        m_animationWorker.reset();
    }

    // Enable export button if we have sound data
    if (m_exportAudioButton) {
        m_exportAudioButton->setEnabled(!m_soundData.pcmSamples.empty());
    }

    if (m_animationFrames.empty()) {
        qWarning() << "AnimationManageWidget: no preview frames generated";
        if (m_playPauseButton)
            m_playPauseButton->setEnabled(false);
        if (m_animationFrameSlider)
            m_animationFrameSlider->setEnabled(false);
        if (m_animationRegenerationPending) {
            m_animationRegenerationPending = false;
            onResultRigChanged();
        }
        return;
    }

    qDebug() << "AnimationManageWidget: received" << m_animationFrames.size() << "frames";

    if (m_animationFrameSlider) {
        m_animationFrameSlider->setEnabled(true);
        m_animationFrameSlider->setRange(0, static_cast<int>(m_animationFrames.size()) - 1);
        m_animationFrameSlider->blockSignals(true);
        m_animationFrameSlider->setValue(0);
        m_animationFrameSlider->blockSignals(false);
    }
    if (m_playPauseButton)
        m_playPauseButton->setEnabled(true);

    m_currentFrame = 0;
    displayCurrentFrame();

    startAnimationLoop();

    if (m_animationRegenerationPending) {
        m_animationRegenerationPending = false;
        onResultRigChanged();
    }
}

void AnimationManageWidget::onAnimationFrameTimeout()
{
    if (m_isScrubbing)
        return;

    if (m_animationFrames.empty() || !m_modelWidget)
        return;

    // Accumulate ground offset in the inverse direction of movement
    if (m_movementSpeed > 0.0f && !m_animationFrames.empty()) {
        double durationSeconds = m_sharedDurationSpinBox ? m_sharedDurationSpinBox->value() : 1.0;
        int frameCount = (int)m_animationFrames.size();
        if (frameCount > 0 && durationSeconds > 0.0) {
            float dt = (float)(durationSeconds / frameCount);
            m_groundOffsetX += m_movementDirectionX * m_movementSpeed * dt;
            m_groundOffsetZ += m_movementDirectionZ * m_movementSpeed * dt;
        }
        m_modelWidget->setGroundOffset(m_groundOffsetX, m_groundOffsetZ);
    }

    displayCurrentFrame();

    m_currentFrame = (m_currentFrame + 1) % (int)m_animationFrames.size();
}

void AnimationManageWidget::startAnimationLoop()
{
    if (!m_frameTimer)
        return;

    double durationSeconds = m_sharedDurationSpinBox ? m_sharedDurationSpinBox->value() : 1.0;
    int frameCount = m_sharedFrameCountSpinBox ? m_sharedFrameCountSpinBox->value() : 30;
    if (frameCount < 1)
        frameCount = 1;
    int intervalMs = std::max(1, static_cast<int>((durationSeconds * 1000.0) / frameCount));
    m_frameTimer->setInterval(intervalMs);

    if (!m_frameTimer->isActive()) {
        m_frameTimer->start();
    }

    // Start sound playback if enabled
    if (m_playSoundCheck && m_playSoundCheck->isChecked() && !m_soundData.pcmSamples.empty()) {
        startSoundPlayback();
    }

    updatePlayPauseIcon();
}

void AnimationManageWidget::stopAnimationLoop()
{
    if (m_frameTimer && m_frameTimer->isActive()) {
        m_frameTimer->stop();
    }
    stopSoundPlayback();
    m_currentFrame = 0;
    updatePlayPauseIcon();
}

void AnimationManageWidget::autoSaveCurrentAnimation()
{
    if (m_isUpdatingForm || !m_document)
        return;

    QString animationName = m_animationNameInput->text().trimmed();
    const QString animationType = m_currentAnimationType;

    if (animationName.isEmpty() || animationType.isEmpty()) {
        return;
    }

    updateAnimationParamsFromWidgets();

    std::map<std::string, std::string> paramsMap = m_animationParams.values;

    if (m_currentAnimationId.isNull()) {
        const auto newId = dust3d::Uuid::createUuid();
        m_document->createAnimation(newId, animationName, animationType);
        m_currentAnimationId = newId;
    } else {
        m_document->setAnimationName(m_currentAnimationId, animationName);
        m_document->setAnimationType(m_currentAnimationId, animationType);
    }

    m_document->setAnimationParams(m_currentAnimationId, paramsMap);
    m_document->saveSnapshot();
    updateParametersGroupBoxTitle();
    refreshAnimationList();
}

void AnimationManageWidget::onParameterChanged()
{
    if (m_isUpdatingForm)
        return;

    autoSaveCurrentAnimation();
    triggerPreviewRegeneration();
}

void AnimationManageWidget::onAnimationNameEdited(const QString&)
{
    if (m_isUpdatingForm)
        return;

    autoSaveCurrentAnimation();
}

void AnimationManageWidget::onDeleteAnimationClicked()
{
    if (!m_document || m_currentAnimationId.isNull())
        return;

    m_document->deleteAnimation(m_currentAnimationId);
    m_document->saveSnapshot();
    m_currentAnimationId = dust3d::Uuid();
    m_animationNameInput->clear();
    m_currentAnimationType.clear();
    m_parametersGroupBox->setTitle(tr("Parameters"));
    m_parametersGroupBox->hide();
    m_bottomStretch->show();
}

void AnimationManageWidget::onDuplicateAnimationClicked()
{
    if (!m_document || m_currentAnimationId.isNull())
        return;

    m_document->duplicateAnimation(m_currentAnimationId);
    m_document->saveSnapshot();
}

void AnimationManageWidget::onAddAnimationClicked()
{
    m_animationListWidget->clearSelection();
    m_currentAnimationId = dust3d::Uuid();

    QString type = m_animationNameCombo->currentText();

    // Derive base name by stripping the rig type prefix and lowercasing
    QString baseName = type;
    if (m_document) {
        QString rigType = m_document->getRigType();
        if (!rigType.isEmpty() && type.startsWith(rigType, Qt::CaseInsensitive)) {
            baseName = type.mid(rigType.length());
        }
    }
    if (!baseName.isEmpty()) {
        baseName[0] = baseName[0].toLower();
    }

    // Find a unique name by checking existing animations
    QString candidateName = baseName;
    int suffix = 2;
    bool conflict = true;
    while (conflict) {
        conflict = false;
        std::vector<dust3d::Uuid> animationIds;
        m_document->getAllAnimationIds(animationIds);
        for (const auto& id : animationIds) {
            const auto* anim = m_document->findAnimation(id);
            if (anim && anim->name.compare(candidateName, Qt::CaseInsensitive) == 0) {
                conflict = true;
                break;
            }
        }
        if (conflict) {
            candidateName = baseName + QString::number(suffix);
            ++suffix;
        }
    }

    m_isUpdatingForm = true;
    m_animationNameInput->setText(candidateName);
    m_currentAnimationType = type;
    m_isUpdatingForm = false;

    m_parametersGroupBox->setTitle(tr("Parameters"));
    m_parametersGroupBox->show();
    m_bottomStretch->hide();
    m_animationNameInput->setFocus();

    rebuildDynamicControls(type);

    autoSaveCurrentAnimation();

    triggerPreviewRegeneration();
}

void AnimationManageWidget::onAnimationListSelectionChanged()
{
    QListWidgetItem* currentItem = m_animationListWidget->currentItem();
    if (!currentItem) {
        m_currentAnimationId = dust3d::Uuid();
        m_animationNameInput->clear();
        m_currentAnimationType.clear();
        m_parametersGroupBox->setTitle(tr("Parameters"));
        m_parametersGroupBox->hide();
        m_bottomStretch->show();
        m_deleteAnimationButton->setEnabled(false);
        m_duplicateAnimationButton->setEnabled(false);
        return;
    }

    m_deleteAnimationButton->setEnabled(true);
    m_duplicateAnimationButton->setEnabled(true);
    m_parametersGroupBox->show();
    m_bottomStretch->hide();

    QString itemId = currentItem->data(Qt::UserRole).toString();
    m_currentAnimationId = dust3d::Uuid(itemId.toUtf8().constData());

    loadAnimationIntoForm(m_currentAnimationId);
}

void AnimationManageWidget::refreshAnimationList()
{
    if (!m_document)
        return;

    m_animationListWidget->blockSignals(true);
    m_animationListWidget->clear();
    std::vector<dust3d::Uuid> animationIds;
    m_document->getAllAnimationIds(animationIds);

    std::vector<std::pair<QString, dust3d::Uuid>> sortedAnims;
    for (const auto& id : animationIds) {
        const auto* anim = m_document->findAnimation(id);
        if (anim) {
            sortedAnims.push_back({ anim->name, id });
        }
    }
    std::sort(sortedAnims.begin(), sortedAnims.end(), [](const auto& a, const auto& b) {
        const QString& sa = a.first;
        const QString& sb = b.first;
        int ia = 0, ib = 0;
        while (ia < sa.size() && ib < sb.size()) {
            QChar ca = sa[ia], cb = sb[ib];
            if (ca.isDigit() && cb.isDigit()) {
                // Compare numeric segments by value
                int startA = ia, startB = ib;
                while (ia < sa.size() && sa[ia].isDigit())
                    ++ia;
                while (ib < sb.size() && sb[ib].isDigit())
                    ++ib;
                int numA = sa.mid(startA, ia - startA).toInt();
                int numB = sb.mid(startB, ib - startB).toInt();
                if (numA != numB)
                    return numA < numB;
            } else {
                int cmp = ca.toLower().unicode() - cb.toLower().unicode();
                if (cmp != 0)
                    return cmp < 0;
                ++ia;
                ++ib;
            }
        }
        return sa.size() < sb.size();
    });

    for (const auto& entry : sortedAnims) {
        const auto* anim = m_document->findAnimation(entry.second);
        QListWidgetItem* item = new QListWidgetItem();
        item->setData(Qt::UserRole, QString::fromStdString(entry.second.toString()));
        m_animationListWidget->addItem(item);

        QWidget* widget = new QWidget;
        QHBoxLayout* hLayout = new QHBoxLayout;
        hLayout->setContentsMargins(2, 0, 2, 0);
        hLayout->setSpacing(4);
        QLabel* nameLabel = new QLabel(entry.first);
        hLayout->addWidget(nameLabel);
        if (anim && !anim->type.isEmpty()) {
            QLabel* typeLabel = new QLabel(QString("(%1)").arg(anim->type));
            typeLabel->setStyleSheet(QString("color: %1;").arg(Theme::gray.name()));
            hLayout->addWidget(typeLabel);
        }
        hLayout->addStretch();
        widget->setLayout(hLayout);
        item->setSizeHint(widget->sizeHint());
        m_animationListWidget->setItemWidget(item, widget);
    }

    if (!m_currentAnimationId.isNull()) {
        for (int i = 0; i < m_animationListWidget->count(); i++) {
            auto item = m_animationListWidget->item(i);
            QString itemId = item->data(Qt::UserRole).toString();
            if (itemId == QString::fromStdString(m_currentAnimationId.toString())) {
                m_animationListWidget->setCurrentItem(item);
                break;
            }
        }
    }
    m_animationListWidget->blockSignals(false);
}

void AnimationManageWidget::loadAnimationIntoForm(const dust3d::Uuid& animationId)
{
    const auto* anim = m_document->findAnimation(animationId);
    if (!anim) {
        m_animationNameInput->clear();
        m_currentAnimationType.clear();
        m_parametersGroupBox->setTitle(tr("Parameters"));
        return;
    }

    m_isUpdatingForm = true;

    m_animationNameInput->setText(anim->name);
    m_currentAnimationType = anim->type;

    // Rebuild controls for this animation type
    rebuildDynamicControls(anim->type);

    // Convert stored params
    dust3d::AnimationParams params;
    params.values = anim->params;

    // Load common parameters
    if (m_sharedDurationSpinBox) {
        m_sharedDurationSpinBox->blockSignals(true);
        m_sharedDurationSpinBox->setValue(params.getValue("durationSeconds", 3.0));
        m_sharedDurationSpinBox->blockSignals(false);
    }
    if (m_sharedFrameCountSpinBox) {
        m_sharedFrameCountSpinBox->blockSignals(true);
        m_sharedFrameCountSpinBox->setValue(static_cast<int>(params.getValue("frameCount", 90.0)));
        m_sharedFrameCountSpinBox->blockSignals(false);
    }

    // Load dynamic parameter values from document (or use defaults from table)
    for (auto& ctrl : m_dynamicControls) {
        if (!ctrl.slider || !ctrl.fromParam)
            continue;
        ctrl.slider->blockSignals(true);
        double paramValue = params.getValue(ctrl.paramName, ctrl.defaultParamValue);
        ctrl.slider->setValue(ctrl.fromParam(paramValue));
        ctrl.slider->blockSignals(false);
        if (ctrl.valueLabel)
            ctrl.valueLabel->setText(QString::number(paramValue, 'f', 2));
    }

    // Load surface material
    if (m_surfaceMaterialCombo) {
        auto it = params.values.find("surfaceMaterial");
        if (it != params.values.end()) {
            int idx = m_surfaceMaterialCombo->findText(QString::fromStdString(it->second));
            if (idx >= 0) {
                m_surfaceMaterialCombo->blockSignals(true);
                m_surfaceMaterialCombo->setCurrentIndex(idx);
                m_surfaceMaterialCombo->blockSignals(false);
            }
        }
    }

    m_isUpdatingForm = false;

    // Update title and trigger preview
    updateParametersGroupBoxTitle();

    m_animationParams = params;
    triggerPreviewRegeneration();
}

void AnimationManageWidget::onSelectedBoneChanged(const QString& boneName)
{
    m_selectedBoneName = boneName;
    triggerPreviewRegeneration();
}

void AnimationManageWidget::startSoundPlayback()
{
    stopSoundPlayback();

    if (m_soundData.pcmSamples.empty())
        return;

    auto wavData = dust3d::SoundGenerator::encodeWav(m_soundData);
    m_soundWavData = QByteArray(reinterpret_cast<const char*>(wavData.data()), static_cast<int>(wavData.size()));

    QAudioFormat format;
    format.setSampleRate(m_soundData.sampleRate);
    format.setChannelCount(m_soundData.channels);
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    format.setSampleFormat(QAudioFormat::Int16);

    auto* sink = new QAudioSink(format, this);
#else
    format.setSampleSize(16);
    format.setCodec("audio/pcm");
    format.setByteOrder(QAudioFormat::LittleEndian);
    format.setSampleType(QAudioFormat::SignedInt);

    auto* sink = new QAudioOutput(format, this);
#endif
    m_soundBuffer = new QBuffer(&m_soundWavData, this);
    m_soundBuffer->open(QIODevice::ReadOnly);
    // Skip WAV header (44 bytes) to feed raw PCM
    m_soundBuffer->seek(44);

    m_audioOutput = static_cast<QObject*>(sink);
    sink->start(m_soundBuffer);

    // Loop: when sound finishes, restart if animation is still playing
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    connect(sink, &QAudioSink::stateChanged, this, [this, sink](QAudio::State state) {
#else
    connect(sink, &QAudioOutput::stateChanged, this, [this, sink](QAudio::State state) {
#endif
        if (state == QAudio::IdleState && m_frameTimer && m_frameTimer->isActive()) {
            if (m_soundBuffer) {
                m_soundBuffer->seek(44);
                sink->start(m_soundBuffer);
            }
        }
    });
}

void AnimationManageWidget::stopSoundPlayback()
{
    if (m_audioOutput) {
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
        auto* sink = qobject_cast<QAudioSink*>(static_cast<QObject*>(m_audioOutput));
#else
        auto* sink = qobject_cast<QAudioOutput*>(static_cast<QObject*>(m_audioOutput));
#endif
        if (sink) {
            sink->stop();
        }
        m_audioOutput->deleteLater();
        m_audioOutput = nullptr;
    }
    if (m_soundBuffer) {
        m_soundBuffer->close();
        m_soundBuffer->deleteLater();
        m_soundBuffer = nullptr;
    }
    m_soundWavData.clear();
}

void AnimationManageWidget::onExportAudioClicked()
{
    if (m_soundData.pcmSamples.empty()) {
        qWarning() << "No sound data to export";
        return;
    }

    QString fileName = QFileDialog::getSaveFileName(this,
        tr("Export Sound Effect"),
        QString(),
        tr("WAV Audio (*.wav)"));

    if (fileName.isEmpty())
        return;

    if (!fileName.endsWith(".wav", Qt::CaseInsensitive))
        fileName += ".wav";

    auto wavData = dust3d::SoundGenerator::encodeWav(m_soundData);

    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly)) {
        qWarning() << "Failed to open file for writing:" << fileName;
        return;
    }
    file.write(reinterpret_cast<const char*>(wavData.data()), static_cast<qint64>(wavData.size()));
    file.close();

    qDebug() << "Exported audio to:" << fileName;
}
