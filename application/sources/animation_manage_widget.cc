#include "animation_manage_widget.h"
#include "document.h"
#include "theme.h"
#include "toolbar_button.h"
#include <QComboBox>
#include <QDebug>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QListWidgetItem>
#include <QVBoxLayout>

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

    setLayout(mainLayout);

    createParameterWidgets();

    m_frameTimer->setInterval(100);
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
    m_hideBonesCheck->setChecked(false);
    m_hidePartsCheck = new QCheckBox("Hide Parts");
    m_hidePartsCheck->setChecked(false);

    QWidget* previewOptionWidget = new QWidget;
    QHBoxLayout* previewOptionLayout = new QHBoxLayout(previewOptionWidget);
    previewOptionLayout->setContentsMargins(0, 0, 0, 0);
    previewOptionLayout->addWidget(m_hideBonesCheck);
    previewOptionLayout->addWidget(m_hidePartsCheck);
    previewOptionLayout->addStretch();
    groupBoxLayout->addWidget(previewOptionWidget);

    // Visual splitter line between preview options and parameter controls
    QFrame* separatorLine = new QFrame;
    separatorLine->setObjectName("separatorLine");
    separatorLine->setFrameShape(QFrame::HLine);
    separatorLine->setFrameShadow(QFrame::Sunken);
    groupBoxLayout->addWidget(separatorLine);

    // Parameters area in form layout (label right aligned, control left aligned)
    QFormLayout* parameterLayout = new QFormLayout;
    m_parameterLayout = parameterLayout;
    parameterLayout->setLabelAlignment(Qt::AlignRight | Qt::AlignVCenter);
    parameterLayout->setFormAlignment(Qt::AlignLeft | Qt::AlignTop);
    parameterLayout->setContentsMargins(0, 0, 0, 0);
    parameterLayout->setSpacing(6);

    m_animationNameInput = new QLineEdit;
    m_animationNameInput->setPlaceholderText(tr("Enter animation name"));
    parameterLayout->addRow(new QLabel(tr("Name:")), m_animationNameInput);

    m_animationTypeInput = new QLineEdit;
    m_animationTypeInput->setReadOnly(true);
    parameterLayout->addRow(new QLabel(tr("Type:")), m_animationTypeInput);

    m_stepLengthSlider = new QSlider;
    m_stepHeightSlider = new QSlider;
    m_bodyBobSlider = new QSlider;
    m_gaitSpeedSlider = new QSlider;
    m_rubForwardOffsetSlider = new QSlider;
    m_rubUpOffsetSlider = new QSlider;

    // Fish animation parameter sliders
    m_swimSpeedSlider = new QSlider;
    m_swimFrequencySlider = new QSlider;
    m_spineAmplitudeSlider = new QSlider;
    m_waveLengthSlider = new QSlider;
    m_tailAmplitudeRatioSlider = new QSlider;
    m_bodyRollSlider = new QSlider;
    m_forwardThrustSlider = new QSlider;
    m_pectoralFlapPowerSlider = new QSlider;
    m_pelvicFlapPowerSlider = new QSlider;
    m_dorsalSwayPowerSlider = new QSlider;
    m_ventralSwayPowerSlider = new QSlider;
    m_pectoralPhaseOffsetSlider = new QSlider;
    m_pelvicPhaseOffsetSlider = new QSlider;

    auto makeSliderRow = [&](const QString& labelText, QSlider* slider, int value, int rangeMin = 25, int rangeMax = 200) {
        slider->setOrientation(Qt::Horizontal);
        slider->setRange(rangeMin, rangeMax);
        slider->setValue(value);
        slider->setSingleStep(1);

        QWidget* rowWidget = new QWidget;
        QHBoxLayout* rowLayout = new QHBoxLayout(rowWidget);
        rowLayout->setContentsMargins(0, 0, 0, 0);
        rowLayout->addWidget(slider);

        QLabel* label = new QLabel(labelText);
        parameterLayout->addRow(label, rowWidget);
        return std::pair<QWidget*, QLabel*>(rowWidget, label);
    };

    auto stepLengthPair = makeSliderRow(tr("Step Length"), m_stepLengthSlider, 100);
    m_stepLengthRow = stepLengthPair.first;
    m_stepLengthLabel = stepLengthPair.second;
    auto stepHeightPair = makeSliderRow(tr("Step Height"), m_stepHeightSlider, 100);
    m_stepHeightRow = stepHeightPair.first;
    m_stepHeightLabel = stepHeightPair.second;
    auto bodyBobPair = makeSliderRow(tr("Body Bob"), m_bodyBobSlider, 100);
    m_bodyBobRow = bodyBobPair.first;
    m_bodyBobLabel = bodyBobPair.second;
    auto gaitSpeedPair = makeSliderRow(tr("Gait Speed"), m_gaitSpeedSlider, 100);
    m_gaitSpeedRow = gaitSpeedPair.first;
    m_gaitSpeedLabel = gaitSpeedPair.second;
    auto rubForwardOffsetPair = makeSliderRow(tr("Rub Forward Offset"), m_rubForwardOffsetSlider, 100);
    m_rubForwardOffsetRow = rubForwardOffsetPair.first;
    m_rubForwardOffsetLabel = rubForwardOffsetPair.second;
    auto rubUpOffsetPair = makeSliderRow(tr("Rub Up Offset"), m_rubUpOffsetSlider, 100);
    m_rubUpOffsetRow = rubUpOffsetPair.first;
    m_rubUpOffsetLabel = rubUpOffsetPair.second;

    // Fish animation parameters
    auto swimSpeedPair = makeSliderRow(tr("Swim Speed"), m_swimSpeedSlider, 100, 25, 300);
    m_swimSpeedRow = swimSpeedPair.first;
    m_swimSpeedLabel = swimSpeedPair.second;

    auto swimFrequencyPair = makeSliderRow(tr("Swim Frequency"), m_swimFrequencySlider, 100, 25, 400);
    m_swimFrequencyRow = swimFrequencyPair.first;
    m_swimFrequencyLabel = swimFrequencyPair.second;

    auto spineAmplitudePair = makeSliderRow(tr("Spine Amplitude"), m_spineAmplitudeSlider, 100, 10, 300);
    m_spineAmplitudeRow = spineAmplitudePair.first;
    m_spineAmplitudeLabel = spineAmplitudePair.second;

    auto waveLengthPair = makeSliderRow(tr("Wave Length"), m_waveLengthSlider, 100, 50, 200);
    m_waveLengthRow = waveLengthPair.first;
    m_waveLengthLabel = waveLengthPair.second;

    auto tailAmplitudeRatioPair = makeSliderRow(tr("Tail Amplitude Ratio"), m_tailAmplitudeRatioSlider, 100, 50, 500);
    m_tailAmplitudeRatioRow = tailAmplitudeRatioPair.first;
    m_tailAmplitudeRatioLabel = tailAmplitudeRatioPair.second;

    auto bodyRollPair = makeSliderRow(tr("Body Roll"), m_bodyRollSlider, 100, 0, 300);
    m_bodyRollRow = bodyRollPair.first;
    m_bodyRollLabel = bodyRollPair.second;

    auto forwardThrustPair = makeSliderRow(tr("Forward Thrust"), m_forwardThrustSlider, 100, 0, 300);
    m_forwardThrustRow = forwardThrustPair.first;
    m_forwardThrustLabel = forwardThrustPair.second;

    auto pectoralFlapPowerPair = makeSliderRow(tr("Pectoral Flap Power"), m_pectoralFlapPowerSlider, 100, 0, 300);
    m_pectoralFlapPowerRow = pectoralFlapPowerPair.first;
    m_pectoralFlapPowerLabel = pectoralFlapPowerPair.second;

    auto pelvicFlapPowerPair = makeSliderRow(tr("Pelvic Flap Power"), m_pelvicFlapPowerSlider, 100, 0, 300);
    m_pelvicFlapPowerRow = pelvicFlapPowerPair.first;
    m_pelvicFlapPowerLabel = pelvicFlapPowerPair.second;

    auto dorsalSwayPowerPair = makeSliderRow(tr("Dorsal Sway Power"), m_dorsalSwayPowerSlider, 100, 0, 300);
    m_dorsalSwayPowerRow = dorsalSwayPowerPair.first;
    m_dorsalSwayPowerLabel = dorsalSwayPowerPair.second;

    auto ventralSwayPowerPair = makeSliderRow(tr("Ventral Sway Power"), m_ventralSwayPowerSlider, 100, 0, 300);
    m_ventralSwayPowerRow = ventralSwayPowerPair.first;
    m_ventralSwayPowerLabel = ventralSwayPowerPair.second;

    auto pectoralPhaseOffsetPair = makeSliderRow(tr("Pectoral Phase Offset"), m_pectoralPhaseOffsetSlider, 0, -200, 200);
    m_pectoralPhaseOffsetRow = pectoralPhaseOffsetPair.first;
    m_pectoralPhaseOffsetLabel = pectoralPhaseOffsetPair.second;

    auto pelvicPhaseOffsetPair = makeSliderRow(tr("Pelvic Phase Offset"), m_pelvicPhaseOffsetSlider, 50, -200, 200);
    m_pelvicPhaseOffsetRow = pelvicPhaseOffsetPair.first;
    m_pelvicPhaseOffsetLabel = pelvicPhaseOffsetPair.second;

    groupBoxLayout->addLayout(parameterLayout);

    m_useFabrikCheck = new QCheckBox("Use FABRIK IK");
    m_useFabrikCheck->setChecked(true);
    m_planeStabilizationCheck = new QCheckBox("Plane Stabilization");
    m_planeStabilizationCheck->setChecked(true);

    groupBoxLayout->addWidget(m_useFabrikCheck);
    groupBoxLayout->addWidget(m_planeStabilizationCheck);

    topLayout->addWidget(m_parametersGroupBox);
    m_parametersGroupBox->hide();

    topLayout->addStretch();

    auto connectAll = [&]() {
        connect(m_stepLengthSlider, &QSlider::valueChanged, this, &AnimationManageWidget::onParameterChanged);
        connect(m_stepHeightSlider, &QSlider::valueChanged, this, &AnimationManageWidget::onParameterChanged);
        connect(m_bodyBobSlider, &QSlider::valueChanged, this, &AnimationManageWidget::onParameterChanged);
        connect(m_gaitSpeedSlider, &QSlider::valueChanged, this, &AnimationManageWidget::onParameterChanged);
        connect(m_rubForwardOffsetSlider, &QSlider::valueChanged, this, &AnimationManageWidget::onParameterChanged);
        connect(m_rubUpOffsetSlider, &QSlider::valueChanged, this, &AnimationManageWidget::onParameterChanged);

        // Fish animation parameter connections
        connect(m_swimSpeedSlider, &QSlider::valueChanged, this, &AnimationManageWidget::onParameterChanged);
        connect(m_swimFrequencySlider, &QSlider::valueChanged, this, &AnimationManageWidget::onParameterChanged);
        connect(m_spineAmplitudeSlider, &QSlider::valueChanged, this, &AnimationManageWidget::onParameterChanged);
        connect(m_waveLengthSlider, &QSlider::valueChanged, this, &AnimationManageWidget::onParameterChanged);
        connect(m_tailAmplitudeRatioSlider, &QSlider::valueChanged, this, &AnimationManageWidget::onParameterChanged);
        connect(m_bodyRollSlider, &QSlider::valueChanged, this, &AnimationManageWidget::onParameterChanged);
        connect(m_forwardThrustSlider, &QSlider::valueChanged, this, &AnimationManageWidget::onParameterChanged);
        connect(m_pectoralFlapPowerSlider, &QSlider::valueChanged, this, &AnimationManageWidget::onParameterChanged);
        connect(m_pelvicFlapPowerSlider, &QSlider::valueChanged, this, &AnimationManageWidget::onParameterChanged);
        connect(m_dorsalSwayPowerSlider, &QSlider::valueChanged, this, &AnimationManageWidget::onParameterChanged);
        connect(m_ventralSwayPowerSlider, &QSlider::valueChanged, this, &AnimationManageWidget::onParameterChanged);
        connect(m_pectoralPhaseOffsetSlider, &QSlider::valueChanged, this, &AnimationManageWidget::onParameterChanged);
        connect(m_pelvicPhaseOffsetSlider, &QSlider::valueChanged, this, &AnimationManageWidget::onParameterChanged);

        connect(m_useFabrikCheck, &QCheckBox::toggled, this, &AnimationManageWidget::onParameterChanged);
        connect(m_planeStabilizationCheck, &QCheckBox::toggled, this, &AnimationManageWidget::onParameterChanged);
        connect(m_hideBonesCheck, &QCheckBox::toggled, this, &AnimationManageWidget::onParameterChanged);
        connect(m_hidePartsCheck, &QCheckBox::toggled, this, &AnimationManageWidget::onParameterChanged);
        connect(m_animationNameInput, &QLineEdit::textEdited, this, &AnimationManageWidget::onAnimationNameEdited);
        connect(m_deleteAnimationButton, &QPushButton::clicked, this, &AnimationManageWidget::onDeleteAnimationClicked);
        connect(m_duplicateAnimationButton, &QPushButton::clicked, this, &AnimationManageWidget::onDuplicateAnimationClicked);
        connect(m_addAnimationButton, &QPushButton::clicked, this, &AnimationManageWidget::onAddAnimationClicked);
    };

    connectAll();

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

    // Initialize internal parameter structure from widgets
    updateAnimationParamsFromWidgets();
}

void AnimationManageWidget::setParameterRowVisible(QWidget* rowWidget, QLabel* rowLabel, bool visible)
{
    if (rowWidget)
        rowWidget->setVisible(visible);
    if (rowLabel)
        rowLabel->setVisible(visible);
}

void AnimationManageWidget::updateVisibleParameters(const QString& animationType)
{
    // Hide all parameter rows first
    setParameterRowVisible(m_stepLengthRow, m_stepLengthLabel, false);
    setParameterRowVisible(m_stepHeightRow, m_stepHeightLabel, false);
    setParameterRowVisible(m_bodyBobRow, m_bodyBobLabel, false);
    setParameterRowVisible(m_gaitSpeedRow, m_gaitSpeedLabel, false);
    setParameterRowVisible(m_rubForwardOffsetRow, m_rubForwardOffsetLabel, false);
    setParameterRowVisible(m_rubUpOffsetRow, m_rubUpOffsetLabel, false);

    // Hide fish parameter rows
    setParameterRowVisible(m_swimSpeedRow, m_swimSpeedLabel, false);
    setParameterRowVisible(m_swimFrequencyRow, m_swimFrequencyLabel, false);
    setParameterRowVisible(m_spineAmplitudeRow, m_spineAmplitudeLabel, false);
    setParameterRowVisible(m_waveLengthRow, m_waveLengthLabel, false);
    setParameterRowVisible(m_tailAmplitudeRatioRow, m_tailAmplitudeRatioLabel, false);
    setParameterRowVisible(m_bodyRollRow, m_bodyRollLabel, false);
    setParameterRowVisible(m_forwardThrustRow, m_forwardThrustLabel, false);
    setParameterRowVisible(m_pectoralFlapPowerRow, m_pectoralFlapPowerLabel, false);
    setParameterRowVisible(m_pelvicFlapPowerRow, m_pelvicFlapPowerLabel, false);
    setParameterRowVisible(m_dorsalSwayPowerRow, m_dorsalSwayPowerLabel, false);
    setParameterRowVisible(m_ventralSwayPowerRow, m_ventralSwayPowerLabel, false);
    setParameterRowVisible(m_pectoralPhaseOffsetRow, m_pectoralPhaseOffsetLabel, false);
    setParameterRowVisible(m_pelvicPhaseOffsetRow, m_pelvicPhaseOffsetLabel, false);

    if (animationType == "InsectWalk" || animationType == "InsectForward" || animationType == "InsectAttack") {
        setParameterRowVisible(m_stepLengthRow, m_stepLengthLabel, true);
        setParameterRowVisible(m_stepHeightRow, m_stepHeightLabel, true);
        setParameterRowVisible(m_bodyBobRow, m_bodyBobLabel, true);
        setParameterRowVisible(m_gaitSpeedRow, m_gaitSpeedLabel, true);
    } else if (animationType == "BirdForward") {
        setParameterRowVisible(m_stepLengthRow, m_stepLengthLabel, true);
        setParameterRowVisible(m_stepHeightRow, m_stepHeightLabel, true);
        setParameterRowVisible(m_bodyBobRow, m_bodyBobLabel, true);
        setParameterRowVisible(m_gaitSpeedRow, m_gaitSpeedLabel, true);
    } else if (animationType == "FishForward") {
        // Show fish-specific parameters
        setParameterRowVisible(m_swimSpeedRow, m_swimSpeedLabel, true);
        setParameterRowVisible(m_swimFrequencyRow, m_swimFrequencyLabel, true);
        setParameterRowVisible(m_spineAmplitudeRow, m_spineAmplitudeLabel, true);
        setParameterRowVisible(m_waveLengthRow, m_waveLengthLabel, true);
        setParameterRowVisible(m_tailAmplitudeRatioRow, m_tailAmplitudeRatioLabel, true);
        setParameterRowVisible(m_bodyRollRow, m_bodyRollLabel, true);
        setParameterRowVisible(m_forwardThrustRow, m_forwardThrustLabel, true);
        setParameterRowVisible(m_pectoralFlapPowerRow, m_pectoralFlapPowerLabel, true);
        setParameterRowVisible(m_pelvicFlapPowerRow, m_pelvicFlapPowerLabel, true);
        setParameterRowVisible(m_dorsalSwayPowerRow, m_dorsalSwayPowerLabel, true);
        setParameterRowVisible(m_ventralSwayPowerRow, m_ventralSwayPowerLabel, true);
        setParameterRowVisible(m_pectoralPhaseOffsetRow, m_pectoralPhaseOffsetLabel, true);
        setParameterRowVisible(m_pelvicPhaseOffsetRow, m_pelvicPhaseOffsetLabel, true);
    } else if (animationType == "InsectRubHands") {
        setParameterRowVisible(m_stepLengthRow, m_stepLengthLabel, true);
        setParameterRowVisible(m_stepHeightRow, m_stepHeightLabel, true);
        setParameterRowVisible(m_bodyBobRow, m_bodyBobLabel, true);
        setParameterRowVisible(m_gaitSpeedRow, m_gaitSpeedLabel, true);
        setParameterRowVisible(m_rubForwardOffsetRow, m_rubForwardOffsetLabel, true);
        setParameterRowVisible(m_rubUpOffsetRow, m_rubUpOffsetLabel, true);
    }
}

void AnimationManageWidget::updateAnimationNameForRigType(const QString& rigType)
{
    if (!m_animationNameCombo || !m_addAnimationButton)
        return;

    m_animationNameCombo->clear();
    if (rigType.compare("Insect", Qt::CaseInsensitive) == 0) {
        m_animationNameCombo->addItem("InsectRubHands");
        m_animationNameCombo->addItem("InsectWalk");
        m_animationNameCombo->addItem("InsectForward");
        m_animationNameCombo->addItem("InsectAttack");
        m_animationNameCombo->setEnabled(true);
        m_addAnimationButton->setEnabled(true);
    } else if (rigType.compare("Bird", Qt::CaseInsensitive) == 0) {
        m_animationNameCombo->addItem("BirdForward");
        m_animationNameCombo->setEnabled(true);
        m_addAnimationButton->setEnabled(true);
    } else if (rigType.compare("Fish", Qt::CaseInsensitive) == 0) {
        m_animationNameCombo->addItem("FishForward");
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
    if (!m_stepLengthSlider || !m_stepHeightSlider || !m_bodyBobSlider || !m_gaitSpeedSlider || !m_rubForwardOffsetSlider || !m_rubUpOffsetSlider || !m_useFabrikCheck || !m_planeStabilizationCheck)
        return;

    // Standard parameters
    m_animationParams.setValue("stepLengthFactor", m_stepLengthSlider->value() / 100.0);
    m_animationParams.setValue("stepHeightFactor", m_stepHeightSlider->value() / 100.0);
    m_animationParams.setValue("bodyBobFactor", m_bodyBobSlider->value() / 100.0);
    m_animationParams.setValue("gaitSpeedFactor", m_gaitSpeedSlider->value() / 10.0);
    m_animationParams.setValue("rubForwardOffsetFactor", m_rubForwardOffsetSlider->value() / 100.0);
    m_animationParams.setValue("rubUpOffsetFactor", m_rubUpOffsetSlider->value() / 100.0);
    m_animationParams.setBool("useFabrikIk", m_useFabrikCheck->isChecked());
    m_animationParams.setBool("planeStabilization", m_planeStabilizationCheck->isChecked());

    // Fish animation parameters - only save if sliders exist
    if (m_swimSpeedSlider)
        m_animationParams.setValue("swimSpeed", m_swimSpeedSlider->value() / 100.0);
    if (m_swimFrequencySlider)
        m_animationParams.setValue("swimFrequency", m_swimFrequencySlider->value() / 50.0);
    if (m_spineAmplitudeSlider)
        m_animationParams.setValue("spineAmplitude", m_spineAmplitudeSlider->value() / 667.0); // 0.15 at value 100
    if (m_waveLengthSlider)
        m_animationParams.setValue("waveLength", m_waveLengthSlider->value() / 100.0);
    if (m_tailAmplitudeRatioSlider)
        m_animationParams.setValue("tailAmplitudeRatio", m_tailAmplitudeRatioSlider->value() / 40.0); // 2.5 at value 100
    if (m_bodyRollSlider)
        m_animationParams.setValue("bodyRoll", m_bodyRollSlider->value() / 2000.0); // 0.05 at value 100
    if (m_forwardThrustSlider)
        m_animationParams.setValue("forwardThrust", m_forwardThrustSlider->value() / 1250.0); // 0.08 at value 100
    if (m_pectoralFlapPowerSlider)
        m_animationParams.setValue("pectoralFlapPower", m_pectoralFlapPowerSlider->value() / 250.0); // 0.4 at value 100
    if (m_pelvicFlapPowerSlider)
        m_animationParams.setValue("pelvicFlapPower", m_pelvicFlapPowerSlider->value() / 400.0); // 0.25 at value 100
    if (m_dorsalSwayPowerSlider)
        m_animationParams.setValue("dorsalSwayPower", m_dorsalSwayPowerSlider->value() / 500.0); // 0.2 at value 100
    if (m_ventralSwayPowerSlider)
        m_animationParams.setValue("ventralSwayPower", m_ventralSwayPowerSlider->value() / 500.0); // 0.2 at value 100
    if (m_pectoralPhaseOffsetSlider)
        m_animationParams.setValue("pectoralPhaseOffset", m_pectoralPhaseOffsetSlider->value() / 100.0);
    if (m_pelvicPhaseOffsetSlider)
        m_animationParams.setValue("pelvicPhaseOffset", m_pelvicPhaseOffsetSlider->value() / 100.0);
}

void AnimationManageWidget::triggerPreviewRegeneration()
{
    if (!m_stepLengthSlider || !m_stepHeightSlider || !m_bodyBobSlider || !m_gaitSpeedSlider || !m_rubForwardOffsetSlider || !m_rubUpOffsetSlider || !m_useFabrikCheck || !m_planeStabilizationCheck || !m_hideBonesCheck || !m_hidePartsCheck)
        return;

    updateAnimationParamsFromWidgets();

    onResultRigChanged();
}

AnimationManageWidget::~AnimationManageWidget()
{
    stopAnimationLoop();
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

    m_animationWorker->setParameters(actualRig, animationType.toStdString(), 30, 1.0f, m_animationParams);
    m_animationWorker->setHideBones(m_hideBonesCheck ? m_hideBonesCheck->isChecked() : false);
    m_animationWorker->setHideParts(m_hidePartsCheck ? m_hidePartsCheck->isChecked() : false);

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
        m_animationWorker.reset();
    }

    if (m_animationFrames.empty()) {
        qWarning() << "AnimationManageWidget: no preview frames generated";
        if (m_animationRegenerationPending) {
            m_animationRegenerationPending = false;
            onResultRigChanged();
        }
        return;
    }

    qDebug() << "AnimationManageWidget: received" << m_animationFrames.size() << "frames";

    startAnimationLoop();

    if (m_animationRegenerationPending) {
        m_animationRegenerationPending = false;
        onResultRigChanged();
    }
}

void AnimationManageWidget::onAnimationFrameTimeout()
{
    if (m_animationFrames.empty() || !m_modelWidget)
        return;

    if (m_currentFrame < 0 || m_currentFrame >= (int)m_animationFrames.size())
        m_currentFrame = 0;

    ModelMesh* frameMesh = new ModelMesh(m_animationFrames[m_currentFrame]);
    m_modelWidget->updateMesh(frameMesh);

    m_currentFrame = (m_currentFrame + 1) % (int)m_animationFrames.size();
}

void AnimationManageWidget::startAnimationLoop()
{
    if (!m_frameTimer)
        return;

    if (!m_frameTimer->isActive()) {
        m_frameTimer->start();
    }
}

void AnimationManageWidget::stopAnimationLoop()
{
    if (m_frameTimer && m_frameTimer->isActive()) {
        m_frameTimer->stop();
    }
    m_currentFrame = 0;
}

void AnimationManageWidget::autoSaveCurrentAnimation()
{
    if (m_isUpdatingForm || !m_document)
        return;

    QString animationName = m_animationNameInput->text().trimmed();
    const QString animationType = m_animationTypeInput->text().trimmed();

    if (animationName.isEmpty() || animationType.isEmpty()) {
        return;
    }

    updateAnimationParamsFromWidgets();

    // Convert AnimationParams struct to key-value string map
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
    m_parametersGroupBox->setTitle(tr("Parameters (") + animationName + ")");
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
    m_animationTypeInput->clear();
    m_parametersGroupBox->setTitle(tr("Parameters (New)"));
    m_parametersGroupBox->hide();
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
    m_isUpdatingForm = true;
    m_animationNameInput->setText(type);
    m_animationTypeInput->setText(type);
    m_isUpdatingForm = false;

    m_parametersGroupBox->setTitle(tr("Parameters (New)"));
    m_parametersGroupBox->show();
    m_animationNameInput->setFocus();

    updateVisibleParameters(type);

    autoSaveCurrentAnimation();

    triggerPreviewRegeneration();
}

void AnimationManageWidget::onAnimationListSelectionChanged()
{
    QListWidgetItem* currentItem = m_animationListWidget->currentItem();
    if (!currentItem) {
        m_currentAnimationId = dust3d::Uuid();
        m_animationNameInput->clear();
        m_animationTypeInput->clear();
        m_parametersGroupBox->setTitle(tr("Parameters (New)"));
        m_parametersGroupBox->hide();
        m_deleteAnimationButton->setEnabled(false);
        m_duplicateAnimationButton->setEnabled(false);
        return;
    }

    m_deleteAnimationButton->setEnabled(true);
    m_duplicateAnimationButton->setEnabled(true);
    m_parametersGroupBox->show();

    QString itemId = currentItem->data(Qt::UserRole).toString();
    m_currentAnimationId = dust3d::Uuid(itemId.toUtf8().constData());

    loadAnimationIntoForm(m_currentAnimationId);
}

void AnimationManageWidget::refreshAnimationList()
{
    if (!m_document)
        return;

    m_animationListWidget->clear();
    std::vector<dust3d::Uuid> animationIds;
    m_document->getAllAnimationIds(animationIds);

    // Sort by animation name for consistent display
    std::vector<std::pair<QString, dust3d::Uuid>> sortedAnims;
    for (const auto& id : animationIds) {
        const auto* anim = m_document->findAnimation(id);
        if (anim) {
            sortedAnims.push_back({ anim->name, id });
        }
    }
    std::sort(sortedAnims.begin(), sortedAnims.end());

    for (const auto& anim : sortedAnims) {
        QListWidgetItem* item = new QListWidgetItem(anim.first);
        item->setData(Qt::UserRole, QString::fromStdString(anim.second.toString()));
        m_animationListWidget->addItem(item);
    }

    // Re-select if current animation still exists
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
}

void AnimationManageWidget::loadAnimationIntoForm(const dust3d::Uuid& animationId)
{
    const auto* anim = m_document->findAnimation(animationId);
    if (!anim) {
        m_animationNameInput->clear();
        m_animationTypeInput->clear();
        m_parametersGroupBox->setTitle(tr("Parameters (New)"));
        return;
    }

    m_animationNameInput->setText(anim->name);
    m_animationTypeInput->setText(anim->type);

    updateVisibleParameters(anim->type);

    // Convert key-value string map to AnimationParams struct
    dust3d::AnimationParams params;
    params.values = anim->params;

    // Load parameters into UI
    m_stepLengthSlider->blockSignals(true);
    m_stepHeightSlider->blockSignals(true);
    m_bodyBobSlider->blockSignals(true);
    m_gaitSpeedSlider->blockSignals(true);
    m_useFabrikCheck->blockSignals(true);
    m_planeStabilizationCheck->blockSignals(true);
    m_rubForwardOffsetSlider->blockSignals(true);
    m_rubUpOffsetSlider->blockSignals(true);

    // Block signals for fish parameters
    if (m_swimSpeedSlider)
        m_swimSpeedSlider->blockSignals(true);
    if (m_swimFrequencySlider)
        m_swimFrequencySlider->blockSignals(true);
    if (m_spineAmplitudeSlider)
        m_spineAmplitudeSlider->blockSignals(true);
    if (m_waveLengthSlider)
        m_waveLengthSlider->blockSignals(true);
    if (m_tailAmplitudeRatioSlider)
        m_tailAmplitudeRatioSlider->blockSignals(true);
    if (m_bodyRollSlider)
        m_bodyRollSlider->blockSignals(true);
    if (m_forwardThrustSlider)
        m_forwardThrustSlider->blockSignals(true);
    if (m_pectoralFlapPowerSlider)
        m_pectoralFlapPowerSlider->blockSignals(true);
    if (m_pelvicFlapPowerSlider)
        m_pelvicFlapPowerSlider->blockSignals(true);
    if (m_dorsalSwayPowerSlider)
        m_dorsalSwayPowerSlider->blockSignals(true);
    if (m_ventralSwayPowerSlider)
        m_ventralSwayPowerSlider->blockSignals(true);
    if (m_pectoralPhaseOffsetSlider)
        m_pectoralPhaseOffsetSlider->blockSignals(true);
    if (m_pelvicPhaseOffsetSlider)
        m_pelvicPhaseOffsetSlider->blockSignals(true);

    m_stepLengthSlider->setValue(static_cast<int>(params.getValue("stepLengthFactor", 1.0) * 100));
    m_stepHeightSlider->setValue(static_cast<int>(params.getValue("stepHeightFactor", 1.0) * 100));
    m_bodyBobSlider->setValue(static_cast<int>(params.getValue("bodyBobFactor", 1.0) * 100));
    m_gaitSpeedSlider->setValue(static_cast<int>(params.getValue("gaitSpeedFactor", 1.0) * 10));
    m_rubForwardOffsetSlider->setValue(static_cast<int>(params.getValue("rubForwardOffsetFactor", 1.0) * 100));
    m_rubUpOffsetSlider->setValue(static_cast<int>(params.getValue("rubUpOffsetFactor", 1.0) * 100));
    m_useFabrikCheck->setChecked(params.getBool("useFabrikIk", true));
    m_planeStabilizationCheck->setChecked(params.getBool("planeStabilization", true));

    // Load fish parameter values (using default values matching the animation implementation)
    if (m_swimSpeedSlider)
        m_swimSpeedSlider->setValue(static_cast<int>(params.getValue("swimSpeed", 1.0) * 100));
    if (m_swimFrequencySlider)
        m_swimFrequencySlider->setValue(static_cast<int>(params.getValue("swimFrequency", 2.0) * 50));
    if (m_spineAmplitudeSlider)
        m_spineAmplitudeSlider->setValue(static_cast<int>(params.getValue("spineAmplitude", 0.15) * 667));
    if (m_waveLengthSlider)
        m_waveLengthSlider->setValue(static_cast<int>(params.getValue("waveLength", 1.0) * 100));
    if (m_tailAmplitudeRatioSlider)
        m_tailAmplitudeRatioSlider->setValue(static_cast<int>(params.getValue("tailAmplitudeRatio", 2.5) * 40));
    if (m_bodyRollSlider)
        m_bodyRollSlider->setValue(static_cast<int>(params.getValue("bodyRoll", 0.05) * 2000));
    if (m_forwardThrustSlider)
        m_forwardThrustSlider->setValue(static_cast<int>(params.getValue("forwardThrust", 0.08) * 1250));
    if (m_pectoralFlapPowerSlider)
        m_pectoralFlapPowerSlider->setValue(static_cast<int>(params.getValue("pectoralFlapPower", 0.4) * 250));
    if (m_pelvicFlapPowerSlider)
        m_pelvicFlapPowerSlider->setValue(static_cast<int>(params.getValue("pelvicFlapPower", 0.25) * 400));
    if (m_dorsalSwayPowerSlider)
        m_dorsalSwayPowerSlider->setValue(static_cast<int>(params.getValue("dorsalSwayPower", 0.2) * 500));
    if (m_ventralSwayPowerSlider)
        m_ventralSwayPowerSlider->setValue(static_cast<int>(params.getValue("ventralSwayPower", 0.2) * 500));
    if (m_pectoralPhaseOffsetSlider)
        m_pectoralPhaseOffsetSlider->setValue(static_cast<int>(params.getValue("pectoralPhaseOffset", 0.0) * 100));
    if (m_pelvicPhaseOffsetSlider)
        m_pelvicPhaseOffsetSlider->setValue(static_cast<int>(params.getValue("pelvicPhaseOffset", 0.5) * 100));

    m_stepLengthSlider->blockSignals(false);
    m_stepHeightSlider->blockSignals(false);
    m_bodyBobSlider->blockSignals(false);
    m_gaitSpeedSlider->blockSignals(false);
    m_useFabrikCheck->blockSignals(false);
    m_planeStabilizationCheck->blockSignals(false);
    m_rubForwardOffsetSlider->blockSignals(false);
    m_rubUpOffsetSlider->blockSignals(false);

    // Unblock signals for fish parameters
    if (m_swimSpeedSlider)
        m_swimSpeedSlider->blockSignals(false);
    if (m_swimFrequencySlider)
        m_swimFrequencySlider->blockSignals(false);
    if (m_spineAmplitudeSlider)
        m_spineAmplitudeSlider->blockSignals(false);
    if (m_waveLengthSlider)
        m_waveLengthSlider->blockSignals(false);
    if (m_tailAmplitudeRatioSlider)
        m_tailAmplitudeRatioSlider->blockSignals(false);
    if (m_bodyRollSlider)
        m_bodyRollSlider->blockSignals(false);
    if (m_forwardThrustSlider)
        m_forwardThrustSlider->blockSignals(false);
    if (m_pectoralFlapPowerSlider)
        m_pectoralFlapPowerSlider->blockSignals(false);
    if (m_pelvicFlapPowerSlider)
        m_pelvicFlapPowerSlider->blockSignals(false);
    if (m_dorsalSwayPowerSlider)
        m_dorsalSwayPowerSlider->blockSignals(false);
    if (m_ventralSwayPowerSlider)
        m_ventralSwayPowerSlider->blockSignals(false);
    if (m_pectoralPhaseOffsetSlider)
        m_pectoralPhaseOffsetSlider->blockSignals(false);
    if (m_pelvicPhaseOffsetSlider)
        m_pelvicPhaseOffsetSlider->blockSignals(false);

    // Update title and trigger preview
    m_parametersGroupBox->setTitle(tr("Parameters (") + anim->name + ")");

    // Store the loaded params for preview
    m_animationParams = params;
    triggerPreviewRegeneration();
}
