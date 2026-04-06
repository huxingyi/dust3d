#include "animation_manage_widget.h"
#include "document.h"
#include "monochrome_mesh.h"
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
    m_hideBonesCheck->setChecked(false);
    m_hidePartsCheck = new QCheckBox("Hide Parts");
    m_hidePartsCheck->setChecked(false);
    m_hideWeightsCheck = new QCheckBox("Hide Weights");
    m_hideWeightsCheck->setChecked(true);

    QWidget* previewOptionWidget = new QWidget;
    QHBoxLayout* previewOptionLayout = new QHBoxLayout(previewOptionWidget);
    previewOptionLayout->setContentsMargins(0, 0, 0, 0);
    previewOptionLayout->addWidget(m_hideBonesCheck);
    previewOptionLayout->addWidget(m_hidePartsCheck);
    previewOptionLayout->addWidget(m_hideWeightsCheck);
    previewOptionLayout->addStretch();
    groupBoxLayout->addWidget(previewOptionWidget);

    // Preview timeline slider
    m_animationFrameSlider = new QSlider(Qt::Horizontal);
    m_animationFrameSlider->setMinimum(0);
    m_animationFrameSlider->setMaximum(0);
    m_animationFrameSlider->setEnabled(false);
    m_animationFrameSlider->setToolTip(tr("Drag to scrub animation frames"));

    m_playPauseButton = new QPushButton;
    m_playPauseButton->setIcon(Theme::awesome()->icon(fa::play));
    m_playPauseButton->setToolTip(tr("Play/Pause animation"));
    m_playPauseButton->setFixedSize(26, 26);
    m_playPauseButton->setEnabled(false);

    QWidget* frameSliderWidget = new QWidget;
    QHBoxLayout* frameSliderLayout = new QHBoxLayout(frameSliderWidget);
    frameSliderLayout->setContentsMargins(0, 0, 0, 0);
    frameSliderLayout->addWidget(m_playPauseButton);
    frameSliderLayout->addWidget(m_animationFrameSlider);

    groupBoxLayout->addWidget(frameSliderWidget);

    // Parameters area in form layout (label right aligned, control left aligned)
    QFormLayout* parameterLayout = new QFormLayout;
    m_parameterLayout = parameterLayout;
    parameterLayout->setLabelAlignment(Qt::AlignRight | Qt::AlignVCenter);
    parameterLayout->setFormAlignment(Qt::AlignLeft | Qt::AlignTop);
    parameterLayout->setContentsMargins(0, 0, 0, 0);
    parameterLayout->setSpacing(6);

    m_animationTypeInput = new QLineEdit;
    m_animationTypeInput->setReadOnly(true);
    parameterLayout->addRow(new QLabel(tr("Type:")), m_animationTypeInput);

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

    m_sharedStepLengthSlider = new QSlider;
    m_sharedStepHeightSlider = new QSlider;
    m_sharedBodyBobSlider = new QSlider;
    m_sharedGaitSpeedSlider = new QSlider;
    m_insectRubHandsRubForwardOffsetSlider = new QSlider;
    m_insectRubHandsRubUpOffsetSlider = new QSlider;

    // Fish die parameter sliders
    m_fishDieHitIntensitySlider = new QSlider;
    m_fishDieHitFrequencySlider = new QSlider;
    m_fishDieFlipSpeedSlider = new QSlider;
    m_fishDieFlipAngleSlider = new QSlider;
    m_fishDieTiltSlider = new QSlider;
    m_fishDieFinFlopSlider = new QSlider;
    m_fishDieSpinDecaySlider = new QSlider;

    // Fish animation parameter sliders
    m_fishForwardSwimSpeedSlider = new QSlider;
    m_fishForwardSwimFrequencySlider = new QSlider;
    m_fishForwardSpineAmplitudeSlider = new QSlider;
    m_fishForwardWaveLengthSlider = new QSlider;
    m_fishForwardTailAmplitudeRatioSlider = new QSlider;
    m_fishForwardBodyRollSlider = new QSlider;
    m_fishForwardForwardThrustSlider = new QSlider;
    m_fishForwardPectoralFlapPowerSlider = new QSlider;
    m_fishForwardPelvicFlapPowerSlider = new QSlider;
    m_fishForwardDorsalSwayPowerSlider = new QSlider;
    m_fishForwardVentralSwayPowerSlider = new QSlider;
    m_fishForwardPectoralPhaseOffsetSlider = new QSlider;
    m_fishForwardPelvicPhaseOffsetSlider = new QSlider;

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

    auto stepLengthPair = makeSliderRow(tr("Step Length"), m_sharedStepLengthSlider, 100);
    m_sharedStepLengthRow = stepLengthPair.first;
    m_sharedStepLengthLabel = stepLengthPair.second;
    auto stepHeightPair = makeSliderRow(tr("Step Height"), m_sharedStepHeightSlider, 100);
    m_sharedStepHeightRow = stepHeightPair.first;
    m_sharedStepHeightLabel = stepHeightPair.second;
    auto bodyBobPair = makeSliderRow(tr("Body Bob"), m_sharedBodyBobSlider, 100);
    m_sharedBodyBobRow = bodyBobPair.first;
    m_sharedBodyBobLabel = bodyBobPair.second;
    auto gaitSpeedPair = makeSliderRow(tr("Gait Speed"), m_sharedGaitSpeedSlider, 100);
    m_sharedGaitSpeedRow = gaitSpeedPair.first;
    m_sharedGaitSpeedLabel = gaitSpeedPair.second;
    auto rubForwardOffsetPair = makeSliderRow(tr("Rub Forward Offset"), m_insectRubHandsRubForwardOffsetSlider, 100);
    m_insectRubHandsRubForwardOffsetRow = rubForwardOffsetPair.first;
    m_insectRubHandsRubForwardOffsetLabel = rubForwardOffsetPair.second;
    auto rubUpOffsetPair = makeSliderRow(tr("Rub Up Offset"), m_insectRubHandsRubUpOffsetSlider, 100);
    m_insectRubHandsRubUpOffsetRow = rubUpOffsetPair.first;
    m_insectRubHandsRubUpOffsetLabel = rubUpOffsetPair.second;

    // Biped walk parameters
    m_sharedArmSwingSlider = new QSlider;
    m_sharedHipSwaySlider = new QSlider;
    m_sharedHipRotateSlider = new QSlider;
    m_sharedBipedSpineFlexSlider = new QSlider;
    m_sharedHeadBobSlider = new QSlider;
    m_sharedKneeBendSlider = new QSlider;
    m_sharedLeanForwardSlider = new QSlider;
    m_sharedBouncinessSlider = new QSlider;

    auto armSwingPair = makeSliderRow(tr("Arm Swing"), m_sharedArmSwingSlider, 100);
    m_sharedArmSwingRow = armSwingPair.first;
    m_sharedArmSwingLabel = armSwingPair.second;
    auto hipSwayPair = makeSliderRow(tr("Hip Sway"), m_sharedHipSwaySlider, 100);
    m_sharedHipSwayRow = hipSwayPair.first;
    m_sharedHipSwayLabel = hipSwayPair.second;
    auto hipRotatePair = makeSliderRow(tr("Hip Rotate"), m_sharedHipRotateSlider, 100);
    m_sharedHipRotateRow = hipRotatePair.first;
    m_sharedHipRotateLabel = hipRotatePair.second;
    auto bipedSpineFlexPair = makeSliderRow(tr("Spine Counter"), m_sharedBipedSpineFlexSlider, 100);
    m_sharedBipedSpineFlexRow = bipedSpineFlexPair.first;
    m_sharedBipedSpineFlexLabel = bipedSpineFlexPair.second;
    auto headBobPair = makeSliderRow(tr("Head Stabilize"), m_sharedHeadBobSlider, 100);
    m_sharedHeadBobRow = headBobPair.first;
    m_sharedHeadBobLabel = headBobPair.second;
    auto kneeBendPair = makeSliderRow(tr("Knee Bend"), m_sharedKneeBendSlider, 100);
    m_sharedKneeBendRow = kneeBendPair.first;
    m_sharedKneeBendLabel = kneeBendPair.second;
    auto leanForwardPair = makeSliderRow(tr("Lean Forward"), m_sharedLeanForwardSlider, 100, 0, 200);
    m_sharedLeanForwardRow = leanForwardPair.first;
    m_sharedLeanForwardLabel = leanForwardPair.second;
    auto bouncinessPair = makeSliderRow(tr("Bounciness"), m_sharedBouncinessSlider, 100, 0, 300);
    m_sharedBouncinessRow = bouncinessPair.first;
    m_sharedBouncinessLabel = bouncinessPair.second;

    // Quadruped walk parameters
    m_sharedSpineFlexSlider = new QSlider;
    m_sharedTailSwaySlider = new QSlider;

    auto spineFlexPair = makeSliderRow(tr("Spine Flex"), m_sharedSpineFlexSlider, 100);
    m_sharedSpineFlexRow = spineFlexPair.first;
    m_sharedSpineFlexLabel = spineFlexPair.second;
    auto tailSwayPair = makeSliderRow(tr("Tail Sway"), m_sharedTailSwaySlider, 100);
    m_sharedTailSwayRow = tailSwayPair.first;
    m_sharedTailSwayLabel = tailSwayPair.second;

    // Quadruped run parameters
    m_sharedSuspensionSlider = new QSlider;
    m_quadrupedRunForwardLeanSlider = new QSlider;
    m_sharedStrideFrequencySlider = new QSlider;

    auto suspensionPair = makeSliderRow(tr("Suspension"), m_sharedSuspensionSlider, 100);
    m_sharedSuspensionRow = suspensionPair.first;
    m_sharedSuspensionLabel = suspensionPair.second;
    auto forwardLeanPair = makeSliderRow(tr("Forward Lean"), m_quadrupedRunForwardLeanSlider, 100);
    m_quadrupedRunForwardLeanRow = forwardLeanPair.first;
    m_quadrupedRunForwardLeanLabel = forwardLeanPair.second;
    auto strideFrequencyPair = makeSliderRow(tr("Stride Frequency"), m_sharedStrideFrequencySlider, 100);
    m_sharedStrideFrequencyRow = strideFrequencyPair.first;
    m_sharedStrideFrequencyLabel = strideFrequencyPair.second;
    m_quadrupedRunBoundSlider = new QSlider;
    auto boundPair = makeSliderRow(tr("Bound"), m_quadrupedRunBoundSlider, 0, 0, 100);
    m_quadrupedRunBoundRow = boundPair.first;
    m_quadrupedRunBoundLabel = boundPair.second;

    // Quadruped die parameters
    m_quadDieCollapseSpeedSlider = new QSlider;
    m_quadDieLegSpreadSlider = new QSlider;
    m_quadDieRollIntensitySlider = new QSlider;

    auto quadDieCollapseSpeedPair = makeSliderRow(tr("Collapse Speed"), m_quadDieCollapseSpeedSlider, 100, 10, 300);
    m_quadDieCollapseSpeedRow = quadDieCollapseSpeedPair.first;
    m_quadDieCollapseSpeedLabel = quadDieCollapseSpeedPair.second;

    auto quadDieLegSpreadPair = makeSliderRow(tr("Leg Spread"), m_quadDieLegSpreadSlider, 100, 10, 300);
    m_quadDieLegSpreadRow = quadDieLegSpreadPair.first;
    m_quadDieLegSpreadLabel = quadDieLegSpreadPair.second;

    auto quadDieRollIntensityPair = makeSliderRow(tr("Roll Intensity"), m_quadDieRollIntensitySlider, 100, 0, 300);
    m_quadDieRollIntensityRow = quadDieRollIntensityPair.first;
    m_quadDieRollIntensityLabel = quadDieRollIntensityPair.second;

    // Biped die parameters
    m_bipedDieCollapseSpeedSlider = new QSlider;
    m_bipedDieArmFlailSlider = new QSlider;
    m_bipedDieHeadDropSlider = new QSlider;

    auto bipedDieCollapseSpeedPair = makeSliderRow(tr("Collapse Speed"), m_bipedDieCollapseSpeedSlider, 100, 10, 300);
    m_bipedDieCollapseSpeedRow = bipedDieCollapseSpeedPair.first;
    m_bipedDieCollapseSpeedLabel = bipedDieCollapseSpeedPair.second;
    auto bipedDieArmFlailPair = makeSliderRow(tr("Arm Flail"), m_bipedDieArmFlailSlider, 100, 0, 300);
    m_bipedDieArmFlailRow = bipedDieArmFlailPair.first;
    m_bipedDieArmFlailLabel = bipedDieArmFlailPair.second;
    auto bipedDieHeadDropPair = makeSliderRow(tr("Head Drop"), m_bipedDieHeadDropSlider, 100, 0, 300);
    m_bipedDieHeadDropRow = bipedDieHeadDropPair.first;
    m_bipedDieHeadDropLabel = bipedDieHeadDropPair.second;

    // Bird die parameters
    m_birdDieCollapseSpeedSlider = new QSlider;
    m_birdDieWingFlapSlider = new QSlider;
    m_birdDieRollIntensitySlider = new QSlider;

    auto birdDieCollapseSpeedPair = makeSliderRow(tr("Collapse Speed"), m_birdDieCollapseSpeedSlider, 100, 10, 300);
    m_birdDieCollapseSpeedRow = birdDieCollapseSpeedPair.first;
    m_birdDieCollapseSpeedLabel = birdDieCollapseSpeedPair.second;
    auto birdDieWingFlapPair = makeSliderRow(tr("Wing Flap"), m_birdDieWingFlapSlider, 100, 0, 300);
    m_birdDieWingFlapRow = birdDieWingFlapPair.first;
    m_birdDieWingFlapLabel = birdDieWingFlapPair.second;
    auto birdDieRollIntensityPair = makeSliderRow(tr("Roll Intensity"), m_birdDieRollIntensitySlider, 100, 0, 300);
    m_birdDieRollIntensityRow = birdDieRollIntensityPair.first;
    m_birdDieRollIntensityLabel = birdDieRollIntensityPair.second;

    // Snake die parameters
    m_snakeDieCollapseSpeedSlider = new QSlider;
    m_snakeDieCoilFactorSlider = new QSlider;
    m_snakeDieJawOpenSlider = new QSlider;

    auto snakeDieCollapseSpeedPair = makeSliderRow(tr("Flip Speed"), m_snakeDieCollapseSpeedSlider, 100, 25, 400);
    m_snakeDieCollapseSpeedRow = snakeDieCollapseSpeedPair.first;
    m_snakeDieCollapseSpeedLabel = snakeDieCollapseSpeedPair.second;
    // Flip Angle: 0–180 degrees, default 180
    auto snakeDieCoilFactorPair = makeSliderRow(tr("Flip Angle"), m_snakeDieCoilFactorSlider, 180, 0, 180);
    m_snakeDieCoilFactorRow = snakeDieCoilFactorPair.first;
    m_snakeDieCoilFactorLabel = snakeDieCoilFactorPair.second;
    auto snakeDieJawOpenPair = makeSliderRow(tr("Jaw Open"), m_snakeDieJawOpenSlider, 63, 0, 63);
    m_snakeDieJawOpenRow = snakeDieJawOpenPair.first;
    m_snakeDieJawOpenLabel = snakeDieJawOpenPair.second;

    // Spider die parameters
    m_spiderDieCollapseSpeedSlider = new QSlider;
    m_spiderDieLegSpreadSlider = new QSlider;

    auto spiderDieCollapseSpeedPair = makeSliderRow(tr("Collapse Speed"), m_spiderDieCollapseSpeedSlider, 100, 10, 300);
    m_spiderDieCollapseSpeedRow = spiderDieCollapseSpeedPair.first;
    m_spiderDieCollapseSpeedLabel = spiderDieCollapseSpeedPair.second;
    auto spiderDieLegSpreadPair = makeSliderRow(tr("Leg Spread"), m_spiderDieLegSpreadSlider, 100, 0, 300);
    m_spiderDieLegSpreadRow = spiderDieLegSpreadPair.first;
    m_spiderDieLegSpreadLabel = spiderDieLegSpreadPair.second;

    // Spider walk parameters
    m_spiderLegSpreadSlider = new QSlider;
    m_spiderAbdomenSwaySlider = new QSlider;
    m_spiderPedipalpSwaySlider = new QSlider;
    m_spiderBodyYawSlider = new QSlider;

    auto spiderLegSpreadPair = makeSliderRow(tr("Leg Spread"), m_spiderLegSpreadSlider, 100);
    m_spiderLegSpreadRow = spiderLegSpreadPair.first;
    m_spiderLegSpreadLabel = spiderLegSpreadPair.second;
    auto spiderAbdomenSwayPair = makeSliderRow(tr("Abdomen Sway"), m_spiderAbdomenSwaySlider, 100, 0, 300);
    m_spiderAbdomenSwayRow = spiderAbdomenSwayPair.first;
    m_spiderAbdomenSwayLabel = spiderAbdomenSwayPair.second;
    auto spiderPedipalpSwayPair = makeSliderRow(tr("Pedipalp Sway"), m_spiderPedipalpSwaySlider, 100, 0, 300);
    m_spiderPedipalpSwayRow = spiderPedipalpSwayPair.first;
    m_spiderPedipalpSwayLabel = spiderPedipalpSwayPair.second;
    auto spiderBodyYawPair = makeSliderRow(tr("Body Yaw"), m_spiderBodyYawSlider, 100, 0, 300);
    m_spiderBodyYawRow = spiderBodyYawPair.first;
    m_spiderBodyYawLabel = spiderBodyYawPair.second;

    // Insect die parameters
    m_sharedDieLengthStiffnessSlider = new QSlider;
    m_sharedDieParentStiffnessSlider = new QSlider;
    m_sharedDieMaxJointAngleSlider = new QSlider;
    m_sharedDieDampingSlider = new QSlider;
    m_sharedDieGroundBounceSlider = new QSlider;

    auto fishDieHitIntensityPair = makeSliderRow(tr("Hit Intensity"), m_fishDieHitIntensitySlider, 100, 10, 300);
    m_fishDieHitIntensityRow = fishDieHitIntensityPair.first;
    m_fishDieHitIntensityLabel = fishDieHitIntensityPair.second;

    auto fishDieHitFrequencyPair = makeSliderRow(tr("Hit Frequency"), m_fishDieHitFrequencySlider, 100, 20, 300);
    m_fishDieHitFrequencyRow = fishDieHitFrequencyPair.first;
    m_fishDieHitFrequencyLabel = fishDieHitFrequencyPair.second;

    auto fishDieFlipSpeedPair = makeSliderRow(tr("Flip Speed"), m_fishDieFlipSpeedSlider, 100, 25, 400);
    m_fishDieFlipSpeedRow = fishDieFlipSpeedPair.first;
    m_fishDieFlipSpeedLabel = fishDieFlipSpeedPair.second;

    // Flip Angle: 0–180 degree, default 180 (slider 0–180, value=angle in degrees)
    auto fishDieFlipAnglePair = makeSliderRow(tr("Flip Angle"), m_fishDieFlipAngleSlider, 180, 0, 180);
    m_fishDieFlipAngleRow = fishDieFlipAnglePair.first;
    m_fishDieFlipAngleLabel = fishDieFlipAnglePair.second;

    // Tilt: -100 to +100, 0 = no tilt; maps to -1.0..+1.0 * bodyLength fraction
    auto fishDieTiltPair = makeSliderRow(tr("Tilt"), m_fishDieTiltSlider, 0, -100, 100);
    m_fishDieTiltRow = fishDieTiltPair.first;
    m_fishDieTiltLabel = fishDieTiltPair.second;

    auto fishDieFinFlopPair = makeSliderRow(tr("Fin Flop"), m_fishDieFinFlopSlider, 100, 0, 300);
    m_fishDieFinFlopRow = fishDieFinFlopPair.first;
    m_fishDieFinFlopLabel = fishDieFinFlopPair.second;

    auto fishDieSpinDecayPair = makeSliderRow(tr("Spin Decay"), m_fishDieSpinDecaySlider, 100, 10, 300);
    m_fishDieSpinDecayRow = fishDieSpinDecayPair.first;
    m_fishDieSpinDecayLabel = fishDieSpinDecayPair.second;

    auto dieLengthStiffnessPair = makeSliderRow(tr("Length Stiffness"), m_sharedDieLengthStiffnessSlider, 90, 10, 200);
    m_sharedDieLengthStiffnessRow = dieLengthStiffnessPair.first;
    m_sharedDieLengthStiffnessLabel = dieLengthStiffnessPair.second;

    auto dieParentStiffnessPair = makeSliderRow(tr("Parent Stiffness"), m_sharedDieParentStiffnessSlider, 80, 10, 200);
    m_sharedDieParentStiffnessRow = dieParentStiffnessPair.first;
    m_sharedDieParentStiffnessLabel = dieParentStiffnessPair.second;

    auto dieMaxJointAnglePair = makeSliderRow(tr("Max Joint Angle"), m_sharedDieMaxJointAngleSlider, 120, 60, 180);
    m_sharedDieMaxJointAngleRow = dieMaxJointAnglePair.first;
    m_sharedDieMaxJointAngleLabel = dieMaxJointAnglePair.second;

    auto dieDampingPair = makeSliderRow(tr("Damping"), m_sharedDieDampingSlider, 95, 50, 99);
    m_sharedDieDampingRow = dieDampingPair.first;
    m_sharedDieDampingLabel = dieDampingPair.second;

    auto dieGroundBouncePair = makeSliderRow(tr("Ground Bounce"), m_sharedDieGroundBounceSlider, 22, 0, 100);
    m_sharedDieGroundBounceRow = dieGroundBouncePair.first;
    m_sharedDieGroundBounceLabel = dieGroundBouncePair.second;

    // Fish animation parameters
    auto swimSpeedPair = makeSliderRow(tr("Swim Speed"), m_fishForwardSwimSpeedSlider, 100, 25, 300);
    m_fishForwardSwimSpeedRow = swimSpeedPair.first;
    m_fishForwardSwimSpeedLabel = swimSpeedPair.second;

    auto swimFrequencyPair = makeSliderRow(tr("Swim Frequency"), m_fishForwardSwimFrequencySlider, 100, 25, 400);
    m_fishForwardSwimFrequencyRow = swimFrequencyPair.first;
    m_fishForwardSwimFrequencyLabel = swimFrequencyPair.second;

    auto spineAmplitudePair = makeSliderRow(tr("Spine Amplitude"), m_fishForwardSpineAmplitudeSlider, 100, 10, 300);
    m_fishForwardSpineAmplitudeRow = spineAmplitudePair.first;
    m_fishForwardSpineAmplitudeLabel = spineAmplitudePair.second;

    auto waveLengthPair = makeSliderRow(tr("Wave Length"), m_fishForwardWaveLengthSlider, 100, 50, 200);
    m_fishForwardWaveLengthRow = waveLengthPair.first;
    m_fishForwardWaveLengthLabel = waveLengthPair.second;

    auto tailAmplitudeRatioPair = makeSliderRow(tr("Tail Amplitude Ratio"), m_fishForwardTailAmplitudeRatioSlider, 100, 50, 500);
    m_fishForwardTailAmplitudeRatioRow = tailAmplitudeRatioPair.first;
    m_fishForwardTailAmplitudeRatioLabel = tailAmplitudeRatioPair.second;

    auto bodyRollPair = makeSliderRow(tr("Body Roll"), m_fishForwardBodyRollSlider, 100, 0, 300);
    m_fishForwardBodyRollRow = bodyRollPair.first;
    m_fishForwardBodyRollLabel = bodyRollPair.second;

    auto forwardThrustPair = makeSliderRow(tr("Forward Thrust"), m_fishForwardForwardThrustSlider, 100, 0, 300);
    m_fishForwardForwardThrustRow = forwardThrustPair.first;
    m_fishForwardForwardThrustLabel = forwardThrustPair.second;

    auto pectoralFlapPowerPair = makeSliderRow(tr("Pectoral Flap Power"), m_fishForwardPectoralFlapPowerSlider, 100, 0, 300);
    m_fishForwardPectoralFlapPowerRow = pectoralFlapPowerPair.first;
    m_fishForwardPectoralFlapPowerLabel = pectoralFlapPowerPair.second;

    auto pelvicFlapPowerPair = makeSliderRow(tr("Pelvic Flap Power"), m_fishForwardPelvicFlapPowerSlider, 100, 0, 300);
    m_fishForwardPelvicFlapPowerRow = pelvicFlapPowerPair.first;
    m_fishForwardPelvicFlapPowerLabel = pelvicFlapPowerPair.second;

    auto dorsalSwayPowerPair = makeSliderRow(tr("Dorsal Sway Power"), m_fishForwardDorsalSwayPowerSlider, 100, 0, 300);
    m_fishForwardDorsalSwayPowerRow = dorsalSwayPowerPair.first;
    m_fishForwardDorsalSwayPowerLabel = dorsalSwayPowerPair.second;

    auto ventralSwayPowerPair = makeSliderRow(tr("Ventral Sway Power"), m_fishForwardVentralSwayPowerSlider, 100, 0, 300);
    m_fishForwardVentralSwayPowerRow = ventralSwayPowerPair.first;
    m_fishForwardVentralSwayPowerLabel = ventralSwayPowerPair.second;

    auto pectoralPhaseOffsetPair = makeSliderRow(tr("Pectoral Phase Offset"), m_fishForwardPectoralPhaseOffsetSlider, 0, -200, 200);
    m_fishForwardPectoralPhaseOffsetRow = pectoralPhaseOffsetPair.first;
    m_fishForwardPectoralPhaseOffsetLabel = pectoralPhaseOffsetPair.second;

    auto pelvicPhaseOffsetPair = makeSliderRow(tr("Pelvic Phase Offset"), m_fishForwardPelvicPhaseOffsetSlider, 50, -200, 200);
    m_fishForwardPelvicPhaseOffsetRow = pelvicPhaseOffsetPair.first;
    m_fishForwardPelvicPhaseOffsetLabel = pelvicPhaseOffsetPair.second;

    // Snake animation parameters
    m_snakeForwardWaveSpeedSlider = new QSlider;
    m_snakeForwardWaveFrequencySlider = new QSlider;
    m_snakeForwardWaveAmplitudeSlider = new QSlider;
    m_snakeForwardWaveLengthSlider = new QSlider;
    m_snakeForwardTailAmplitudeRatioSlider = new QSlider;
    m_snakeForwardHeadYawFactorSlider = new QSlider;
    m_snakeForwardHeadPullFactorSlider = new QSlider;
    m_snakeForwardJawAmplitudeSlider = new QSlider;
    m_snakeForwardJawFrequencySlider = new QSlider;

    auto snakeWaveSpeedPair = makeSliderRow(tr("Wave Speed"), m_snakeForwardWaveSpeedSlider, 100, 25, 300);
    m_snakeForwardWaveSpeedRow = snakeWaveSpeedPair.first;
    m_snakeForwardWaveSpeedLabel = snakeWaveSpeedPair.second;

    auto snakeWaveFrequencyPair = makeSliderRow(tr("Wave Frequency"), m_snakeForwardWaveFrequencySlider, 100, 25, 400);
    m_snakeForwardWaveFrequencyRow = snakeWaveFrequencyPair.first;
    m_snakeForwardWaveFrequencyLabel = snakeWaveFrequencyPair.second;

    auto snakeWaveAmplitudePair = makeSliderRow(tr("Wave Amplitude"), m_snakeForwardWaveAmplitudeSlider, 100, 10, 300);
    m_snakeForwardWaveAmplitudeRow = snakeWaveAmplitudePair.first;
    m_snakeForwardWaveAmplitudeLabel = snakeWaveAmplitudePair.second;

    auto snakeWaveLengthPair = makeSliderRow(tr("Wave Length"), m_snakeForwardWaveLengthSlider, 100, 50, 200);
    m_snakeForwardWaveLengthRow = snakeWaveLengthPair.first;
    m_snakeForwardWaveLengthLabel = snakeWaveLengthPair.second;

    auto snakeTailAmplitudeRatioPair = makeSliderRow(tr("Tail Amplitude Ratio"), m_snakeForwardTailAmplitudeRatioSlider, 100, 50, 500);
    m_snakeForwardTailAmplitudeRatioRow = snakeTailAmplitudeRatioPair.first;
    m_snakeForwardTailAmplitudeRatioLabel = snakeTailAmplitudeRatioPair.second;

    auto snakeHeadYawPair = makeSliderRow(tr("Head Yaw"), m_snakeForwardHeadYawFactorSlider, 100, 0, 200);
    m_snakeForwardHeadYawFactorRow = snakeHeadYawPair.first;
    m_snakeForwardHeadYawFactorLabel = snakeHeadYawPair.second;

    auto snakeHeadPullPair = makeSliderRow(tr("Head Pull"), m_snakeForwardHeadPullFactorSlider, 100, 0, 300);
    m_snakeForwardHeadPullFactorRow = snakeHeadPullPair.first;
    m_snakeForwardHeadPullFactorLabel = snakeHeadPullPair.second;

    auto snakeJawAmplitudePair = makeSliderRow(tr("Jaw Amplitude"), m_snakeForwardJawAmplitudeSlider, 200, 0, 600);
    m_snakeForwardJawAmplitudeRow = snakeJawAmplitudePair.first;
    m_snakeForwardJawAmplitudeLabel = snakeJawAmplitudePair.second;

    auto snakeJawFrequencyPair = makeSliderRow(tr("Jaw Frequency"), m_snakeForwardJawFrequencySlider, 100, 0, 600);
    m_snakeForwardJawFrequencyRow = snakeJawFrequencyPair.first;
    m_snakeForwardJawFrequencyLabel = snakeJawFrequencyPair.second;

    m_sharedUseFabrikCheck = new QCheckBox("Use FABRIK IK");
    m_sharedUseFabrikCheck->setChecked(true);
    parameterLayout->addRow(m_sharedUseFabrikCheck);

    m_sharedPlaneStabilizationCheck = new QCheckBox("Plane Stabilization");
    m_sharedPlaneStabilizationCheck->setChecked(true);
    parameterLayout->addRow(m_sharedPlaneStabilizationCheck);

    // Add parameter layout to a scrollable area to prevent uncontrolled growth
    QWidget* scrollAreaWidget = new QWidget;
    scrollAreaWidget->setLayout(parameterLayout);
    QScrollArea* parameterScrollArea = new QScrollArea;
    parameterScrollArea->setWidget(scrollAreaWidget);
    parameterScrollArea->setWidgetResizable(true);
    groupBoxLayout->addWidget(parameterScrollArea, 1);
    groupBoxLayout->addStretch();

    topLayout->addWidget(m_parametersGroupBox);
    m_parametersGroupBox->hide();

    topLayout->addStretch();

    auto connectAll = [&]() {
        connect(m_sharedStepLengthSlider, &QSlider::valueChanged, this, &AnimationManageWidget::onParameterChanged);
        connect(m_sharedStepHeightSlider, &QSlider::valueChanged, this, &AnimationManageWidget::onParameterChanged);
        connect(m_sharedBodyBobSlider, &QSlider::valueChanged, this, &AnimationManageWidget::onParameterChanged);
        connect(m_sharedGaitSpeedSlider, &QSlider::valueChanged, this, &AnimationManageWidget::onParameterChanged);
        connect(m_insectRubHandsRubForwardOffsetSlider, &QSlider::valueChanged, this, &AnimationManageWidget::onParameterChanged);
        connect(m_insectRubHandsRubUpOffsetSlider, &QSlider::valueChanged, this, &AnimationManageWidget::onParameterChanged);

        // Quadruped walk parameter connections
        connect(m_sharedSpineFlexSlider, &QSlider::valueChanged, this, &AnimationManageWidget::onParameterChanged);
        connect(m_sharedTailSwaySlider, &QSlider::valueChanged, this, &AnimationManageWidget::onParameterChanged);

        // Biped walk/run parameter connections
        connect(m_sharedArmSwingSlider, &QSlider::valueChanged, this, &AnimationManageWidget::onParameterChanged);
        connect(m_sharedHipSwaySlider, &QSlider::valueChanged, this, &AnimationManageWidget::onParameterChanged);
        connect(m_sharedHipRotateSlider, &QSlider::valueChanged, this, &AnimationManageWidget::onParameterChanged);
        connect(m_sharedBipedSpineFlexSlider, &QSlider::valueChanged, this, &AnimationManageWidget::onParameterChanged);
        connect(m_sharedHeadBobSlider, &QSlider::valueChanged, this, &AnimationManageWidget::onParameterChanged);
        connect(m_sharedKneeBendSlider, &QSlider::valueChanged, this, &AnimationManageWidget::onParameterChanged);
        connect(m_sharedLeanForwardSlider, &QSlider::valueChanged, this, &AnimationManageWidget::onParameterChanged);
        connect(m_sharedBouncinessSlider, &QSlider::valueChanged, this, &AnimationManageWidget::onParameterChanged);

        // Quadruped run parameter connections
        connect(m_sharedSuspensionSlider, &QSlider::valueChanged, this, &AnimationManageWidget::onParameterChanged);
        connect(m_quadrupedRunForwardLeanSlider, &QSlider::valueChanged, this, &AnimationManageWidget::onParameterChanged);
        connect(m_sharedStrideFrequencySlider, &QSlider::valueChanged, this, &AnimationManageWidget::onParameterChanged);
        connect(m_quadrupedRunBoundSlider, &QSlider::valueChanged, this, &AnimationManageWidget::onParameterChanged);

        // Insect die parameter connections
        connect(m_sharedDieLengthStiffnessSlider, &QSlider::valueChanged, this, &AnimationManageWidget::onParameterChanged);
        connect(m_sharedDieParentStiffnessSlider, &QSlider::valueChanged, this, &AnimationManageWidget::onParameterChanged);
        connect(m_sharedDieMaxJointAngleSlider, &QSlider::valueChanged, this, &AnimationManageWidget::onParameterChanged);
        connect(m_sharedDieDampingSlider, &QSlider::valueChanged, this, &AnimationManageWidget::onParameterChanged);
        connect(m_sharedDieGroundBounceSlider, &QSlider::valueChanged, this, &AnimationManageWidget::onParameterChanged);

        // Quadruped die parameter connections
        connect(m_quadDieCollapseSpeedSlider, &QSlider::valueChanged, this, &AnimationManageWidget::onParameterChanged);
        connect(m_quadDieLegSpreadSlider, &QSlider::valueChanged, this, &AnimationManageWidget::onParameterChanged);
        connect(m_quadDieRollIntensitySlider, &QSlider::valueChanged, this, &AnimationManageWidget::onParameterChanged);

        // Biped die parameter connections
        connect(m_bipedDieCollapseSpeedSlider, &QSlider::valueChanged, this, &AnimationManageWidget::onParameterChanged);
        connect(m_bipedDieArmFlailSlider, &QSlider::valueChanged, this, &AnimationManageWidget::onParameterChanged);
        connect(m_bipedDieHeadDropSlider, &QSlider::valueChanged, this, &AnimationManageWidget::onParameterChanged);

        // Bird die parameter connections
        connect(m_birdDieCollapseSpeedSlider, &QSlider::valueChanged, this, &AnimationManageWidget::onParameterChanged);
        connect(m_birdDieWingFlapSlider, &QSlider::valueChanged, this, &AnimationManageWidget::onParameterChanged);
        connect(m_birdDieRollIntensitySlider, &QSlider::valueChanged, this, &AnimationManageWidget::onParameterChanged);

        // Snake die parameter connections
        connect(m_snakeDieCollapseSpeedSlider, &QSlider::valueChanged, this, &AnimationManageWidget::onParameterChanged);
        connect(m_snakeDieCoilFactorSlider, &QSlider::valueChanged, this, &AnimationManageWidget::onParameterChanged);
        connect(m_snakeDieJawOpenSlider, &QSlider::valueChanged, this, &AnimationManageWidget::onParameterChanged);

        // Spider die parameter connections
        connect(m_spiderDieCollapseSpeedSlider, &QSlider::valueChanged, this, &AnimationManageWidget::onParameterChanged);
        connect(m_spiderDieLegSpreadSlider, &QSlider::valueChanged, this, &AnimationManageWidget::onParameterChanged);

        // Spider walk parameter connections
        connect(m_spiderLegSpreadSlider, &QSlider::valueChanged, this, &AnimationManageWidget::onParameterChanged);
        connect(m_spiderAbdomenSwaySlider, &QSlider::valueChanged, this, &AnimationManageWidget::onParameterChanged);
        connect(m_spiderPedipalpSwaySlider, &QSlider::valueChanged, this, &AnimationManageWidget::onParameterChanged);
        connect(m_spiderBodyYawSlider, &QSlider::valueChanged, this, &AnimationManageWidget::onParameterChanged);

        // Fish die parameter connections
        connect(m_fishDieHitIntensitySlider, &QSlider::valueChanged, this, &AnimationManageWidget::onParameterChanged);
        connect(m_fishDieHitFrequencySlider, &QSlider::valueChanged, this, &AnimationManageWidget::onParameterChanged);
        connect(m_fishDieFlipSpeedSlider, &QSlider::valueChanged, this, &AnimationManageWidget::onParameterChanged);
        connect(m_fishDieFlipAngleSlider, &QSlider::valueChanged, this, &AnimationManageWidget::onParameterChanged);
        connect(m_fishDieTiltSlider, &QSlider::valueChanged, this, &AnimationManageWidget::onParameterChanged);
        connect(m_fishDieFinFlopSlider, &QSlider::valueChanged, this, &AnimationManageWidget::onParameterChanged);
        connect(m_fishDieSpinDecaySlider, &QSlider::valueChanged, this, &AnimationManageWidget::onParameterChanged);

        // Fish animation parameter connections
        connect(m_fishForwardSwimSpeedSlider, &QSlider::valueChanged, this, &AnimationManageWidget::onParameterChanged);
        connect(m_fishForwardSwimFrequencySlider, &QSlider::valueChanged, this, &AnimationManageWidget::onParameterChanged);
        connect(m_fishForwardSpineAmplitudeSlider, &QSlider::valueChanged, this, &AnimationManageWidget::onParameterChanged);
        connect(m_fishForwardWaveLengthSlider, &QSlider::valueChanged, this, &AnimationManageWidget::onParameterChanged);
        connect(m_fishForwardTailAmplitudeRatioSlider, &QSlider::valueChanged, this, &AnimationManageWidget::onParameterChanged);
        connect(m_fishForwardBodyRollSlider, &QSlider::valueChanged, this, &AnimationManageWidget::onParameterChanged);
        connect(m_fishForwardForwardThrustSlider, &QSlider::valueChanged, this, &AnimationManageWidget::onParameterChanged);
        connect(m_fishForwardPectoralFlapPowerSlider, &QSlider::valueChanged, this, &AnimationManageWidget::onParameterChanged);
        connect(m_fishForwardPelvicFlapPowerSlider, &QSlider::valueChanged, this, &AnimationManageWidget::onParameterChanged);
        connect(m_fishForwardDorsalSwayPowerSlider, &QSlider::valueChanged, this, &AnimationManageWidget::onParameterChanged);
        connect(m_fishForwardVentralSwayPowerSlider, &QSlider::valueChanged, this, &AnimationManageWidget::onParameterChanged);
        connect(m_fishForwardPectoralPhaseOffsetSlider, &QSlider::valueChanged, this, &AnimationManageWidget::onParameterChanged);
        connect(m_fishForwardPelvicPhaseOffsetSlider, &QSlider::valueChanged, this, &AnimationManageWidget::onParameterChanged);
        connect(m_snakeForwardWaveSpeedSlider, &QSlider::valueChanged, this, &AnimationManageWidget::onParameterChanged);
        connect(m_snakeForwardWaveFrequencySlider, &QSlider::valueChanged, this, &AnimationManageWidget::onParameterChanged);
        connect(m_snakeForwardWaveAmplitudeSlider, &QSlider::valueChanged, this, &AnimationManageWidget::onParameterChanged);
        connect(m_snakeForwardWaveLengthSlider, &QSlider::valueChanged, this, &AnimationManageWidget::onParameterChanged);
        connect(m_snakeForwardTailAmplitudeRatioSlider, &QSlider::valueChanged, this, &AnimationManageWidget::onParameterChanged);
        connect(m_snakeForwardHeadYawFactorSlider, &QSlider::valueChanged, this, &AnimationManageWidget::onParameterChanged);
        connect(m_snakeForwardHeadPullFactorSlider, &QSlider::valueChanged, this, &AnimationManageWidget::onParameterChanged);
        connect(m_snakeForwardJawAmplitudeSlider, &QSlider::valueChanged, this, &AnimationManageWidget::onParameterChanged);
        connect(m_snakeForwardJawFrequencySlider, &QSlider::valueChanged, this, &AnimationManageWidget::onParameterChanged);

        connect(m_sharedUseFabrikCheck, &QCheckBox::toggled, this, &AnimationManageWidget::onParameterChanged);
        connect(m_sharedPlaneStabilizationCheck, &QCheckBox::toggled, this, &AnimationManageWidget::onParameterChanged);
        connect(m_hideBonesCheck, &QCheckBox::toggled, this, &AnimationManageWidget::onParameterChanged);
        connect(m_hidePartsCheck, &QCheckBox::toggled, this, &AnimationManageWidget::onParameterChanged);
        connect(m_hideWeightsCheck, &QCheckBox::toggled, this, &AnimationManageWidget::onParameterChanged);

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
            // If no skinned vertices or skeleton-only frame, hide rig skeleton wireframe
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

void AnimationManageWidget::updateVisibleParameters(const QString& animationType)
{
    // Hide all parameter rows first
    setParameterRowVisible(m_sharedStepLengthRow, m_sharedStepLengthLabel, false);
    setParameterRowVisible(m_sharedStepHeightRow, m_sharedStepHeightLabel, false);
    setParameterRowVisible(m_sharedBodyBobRow, m_sharedBodyBobLabel, false);
    setParameterRowVisible(m_sharedGaitSpeedRow, m_sharedGaitSpeedLabel, false);
    setParameterRowVisible(m_insectRubHandsRubForwardOffsetRow, m_insectRubHandsRubForwardOffsetLabel, false);
    setParameterRowVisible(m_insectRubHandsRubUpOffsetRow, m_insectRubHandsRubUpOffsetLabel, false);

    // Hide insect die parameter rows
    setParameterRowVisible(m_sharedDieLengthStiffnessRow, m_sharedDieLengthStiffnessLabel, false);
    setParameterRowVisible(m_sharedDieParentStiffnessRow, m_sharedDieParentStiffnessLabel, false);
    setParameterRowVisible(m_sharedDieMaxJointAngleRow, m_sharedDieMaxJointAngleLabel, false);
    setParameterRowVisible(m_sharedDieDampingRow, m_sharedDieDampingLabel, false);
    setParameterRowVisible(m_sharedDieGroundBounceRow, m_sharedDieGroundBounceLabel, false);

    // Hide fish die parameter rows
    setParameterRowVisible(m_fishDieHitIntensityRow, m_fishDieHitIntensityLabel, false);
    setParameterRowVisible(m_fishDieHitFrequencyRow, m_fishDieHitFrequencyLabel, false);
    setParameterRowVisible(m_fishDieFlipSpeedRow, m_fishDieFlipSpeedLabel, false);
    setParameterRowVisible(m_fishDieFlipAngleRow, m_fishDieFlipAngleLabel, false);
    setParameterRowVisible(m_fishDieTiltRow, m_fishDieTiltLabel, false);
    setParameterRowVisible(m_fishDieFinFlopRow, m_fishDieFinFlopLabel, false);
    setParameterRowVisible(m_fishDieSpinDecayRow, m_fishDieSpinDecayLabel, false);

    // Hide fish parameter rows
    setParameterRowVisible(m_fishForwardSwimSpeedRow, m_fishForwardSwimSpeedLabel, false);
    setParameterRowVisible(m_fishForwardSwimFrequencyRow, m_fishForwardSwimFrequencyLabel, false);
    setParameterRowVisible(m_fishForwardSpineAmplitudeRow, m_fishForwardSpineAmplitudeLabel, false);
    setParameterRowVisible(m_fishForwardWaveLengthRow, m_fishForwardWaveLengthLabel, false);
    setParameterRowVisible(m_fishForwardTailAmplitudeRatioRow, m_fishForwardTailAmplitudeRatioLabel, false);
    setParameterRowVisible(m_fishForwardBodyRollRow, m_fishForwardBodyRollLabel, false);
    setParameterRowVisible(m_fishForwardForwardThrustRow, m_fishForwardForwardThrustLabel, false);
    setParameterRowVisible(m_fishForwardPectoralFlapPowerRow, m_fishForwardPectoralFlapPowerLabel, false);
    setParameterRowVisible(m_fishForwardPelvicFlapPowerRow, m_fishForwardPelvicFlapPowerLabel, false);
    setParameterRowVisible(m_fishForwardDorsalSwayPowerRow, m_fishForwardDorsalSwayPowerLabel, false);
    setParameterRowVisible(m_fishForwardVentralSwayPowerRow, m_fishForwardVentralSwayPowerLabel, false);
    setParameterRowVisible(m_fishForwardPectoralPhaseOffsetRow, m_fishForwardPectoralPhaseOffsetLabel, false);
    setParameterRowVisible(m_fishForwardPelvicPhaseOffsetRow, m_fishForwardPelvicPhaseOffsetLabel, false);

    // Hide snake forward parameter rows
    setParameterRowVisible(m_snakeForwardWaveSpeedRow, m_snakeForwardWaveSpeedLabel, false);
    setParameterRowVisible(m_snakeForwardWaveFrequencyRow, m_snakeForwardWaveFrequencyLabel, false);
    setParameterRowVisible(m_snakeForwardWaveAmplitudeRow, m_snakeForwardWaveAmplitudeLabel, false);
    setParameterRowVisible(m_snakeForwardWaveLengthRow, m_snakeForwardWaveLengthLabel, false);
    setParameterRowVisible(m_snakeForwardTailAmplitudeRatioRow, m_snakeForwardTailAmplitudeRatioLabel, false);
    setParameterRowVisible(m_snakeForwardHeadYawFactorRow, m_snakeForwardHeadYawFactorLabel, false);
    setParameterRowVisible(m_snakeForwardHeadPullFactorRow, m_snakeForwardHeadPullFactorLabel, false);
    setParameterRowVisible(m_snakeForwardJawAmplitudeRow, m_snakeForwardJawAmplitudeLabel, false);
    setParameterRowVisible(m_snakeForwardJawFrequencyRow, m_snakeForwardJawFrequencyLabel, false);

    // Hide quadruped parameter rows
    setParameterRowVisible(m_sharedSpineFlexRow, m_sharedSpineFlexLabel, false);
    setParameterRowVisible(m_sharedTailSwayRow, m_sharedTailSwayLabel, false);
    setParameterRowVisible(m_sharedSuspensionRow, m_sharedSuspensionLabel, false);
    setParameterRowVisible(m_quadrupedRunForwardLeanRow, m_quadrupedRunForwardLeanLabel, false);
    setParameterRowVisible(m_sharedStrideFrequencyRow, m_sharedStrideFrequencyLabel, false);
    setParameterRowVisible(m_quadrupedRunBoundRow, m_quadrupedRunBoundLabel, false);

    // Hide biped walk parameter rows
    setParameterRowVisible(m_sharedArmSwingRow, m_sharedArmSwingLabel, false);
    setParameterRowVisible(m_sharedHipSwayRow, m_sharedHipSwayLabel, false);
    setParameterRowVisible(m_sharedHipRotateRow, m_sharedHipRotateLabel, false);
    setParameterRowVisible(m_sharedBipedSpineFlexRow, m_sharedBipedSpineFlexLabel, false);
    setParameterRowVisible(m_sharedHeadBobRow, m_sharedHeadBobLabel, false);
    setParameterRowVisible(m_sharedKneeBendRow, m_sharedKneeBendLabel, false);
    setParameterRowVisible(m_sharedLeanForwardRow, m_sharedLeanForwardLabel, false);
    setParameterRowVisible(m_sharedBouncinessRow, m_sharedBouncinessLabel, false);

    // Hide quadruped die parameter rows
    setParameterRowVisible(m_quadDieCollapseSpeedRow, m_quadDieCollapseSpeedLabel, false);
    setParameterRowVisible(m_quadDieLegSpreadRow, m_quadDieLegSpreadLabel, false);
    setParameterRowVisible(m_quadDieRollIntensityRow, m_quadDieRollIntensityLabel, false);

    // Hide biped die parameter rows
    setParameterRowVisible(m_bipedDieCollapseSpeedRow, m_bipedDieCollapseSpeedLabel, false);
    setParameterRowVisible(m_bipedDieArmFlailRow, m_bipedDieArmFlailLabel, false);
    setParameterRowVisible(m_bipedDieHeadDropRow, m_bipedDieHeadDropLabel, false);

    // Hide bird die parameter rows
    setParameterRowVisible(m_birdDieCollapseSpeedRow, m_birdDieCollapseSpeedLabel, false);
    setParameterRowVisible(m_birdDieWingFlapRow, m_birdDieWingFlapLabel, false);
    setParameterRowVisible(m_birdDieRollIntensityRow, m_birdDieRollIntensityLabel, false);

    // Hide snake die parameter rows
    setParameterRowVisible(m_snakeDieCollapseSpeedRow, m_snakeDieCollapseSpeedLabel, false);
    setParameterRowVisible(m_snakeDieCoilFactorRow, m_snakeDieCoilFactorLabel, false);
    setParameterRowVisible(m_snakeDieJawOpenRow, m_snakeDieJawOpenLabel, false);

    // Hide spider die parameter rows
    setParameterRowVisible(m_spiderDieCollapseSpeedRow, m_spiderDieCollapseSpeedLabel, false);
    setParameterRowVisible(m_spiderDieLegSpreadRow, m_spiderDieLegSpreadLabel, false);

    // Hide spider walk parameter rows
    setParameterRowVisible(m_spiderLegSpreadRow, m_spiderLegSpreadLabel, false);
    setParameterRowVisible(m_spiderAbdomenSwayRow, m_spiderAbdomenSwayLabel, false);
    setParameterRowVisible(m_spiderPedipalpSwayRow, m_spiderPedipalpSwayLabel, false);
    setParameterRowVisible(m_spiderBodyYawRow, m_spiderBodyYawLabel, false);

    // useFabrikIk and planeStabilization are only relevant for animation types that use IK
    bool showIkParams = (animationType == "InsectWalk" || animationType == "InsectForward"
        || animationType == "InsectAttack" || animationType == "InsectRubHands"
        || animationType == "QuadrupedWalk" || animationType == "QuadrupedRun"
        || animationType == "BipedWalk" || animationType == "BipedRun"
        || animationType == "SpiderWalk");
    if (m_sharedUseFabrikCheck)
        m_sharedUseFabrikCheck->setVisible(showIkParams);
    if (m_sharedPlaneStabilizationCheck)
        m_sharedPlaneStabilizationCheck->setVisible(showIkParams);

    if (animationType == "InsectWalk" || animationType == "InsectForward" || animationType == "InsectAttack") {
        setParameterRowVisible(m_sharedStepLengthRow, m_sharedStepLengthLabel, true);
        setParameterRowVisible(m_sharedStepHeightRow, m_sharedStepHeightLabel, true);
        setParameterRowVisible(m_sharedBodyBobRow, m_sharedBodyBobLabel, true);
        setParameterRowVisible(m_sharedGaitSpeedRow, m_sharedGaitSpeedLabel, true);
    } else if (animationType == "InsectDie") {
        setParameterRowVisible(m_sharedDieLengthStiffnessRow, m_sharedDieLengthStiffnessLabel, true);
        setParameterRowVisible(m_sharedDieParentStiffnessRow, m_sharedDieParentStiffnessLabel, true);
        setParameterRowVisible(m_sharedDieMaxJointAngleRow, m_sharedDieMaxJointAngleLabel, true);
        setParameterRowVisible(m_sharedDieDampingRow, m_sharedDieDampingLabel, true);
        setParameterRowVisible(m_sharedDieGroundBounceRow, m_sharedDieGroundBounceLabel, true);
    } else if (animationType == "BirdForward") {
        setParameterRowVisible(m_sharedStepLengthRow, m_sharedStepLengthLabel, true);
        setParameterRowVisible(m_sharedStepHeightRow, m_sharedStepHeightLabel, true);
        setParameterRowVisible(m_sharedBodyBobRow, m_sharedBodyBobLabel, true);
        setParameterRowVisible(m_sharedGaitSpeedRow, m_sharedGaitSpeedLabel, true);
    } else if (animationType == "FishDie") {
        setParameterRowVisible(m_fishDieHitIntensityRow, m_fishDieHitIntensityLabel, true);
        setParameterRowVisible(m_fishDieHitFrequencyRow, m_fishDieHitFrequencyLabel, true);
        setParameterRowVisible(m_fishDieFlipSpeedRow, m_fishDieFlipSpeedLabel, true);
        setParameterRowVisible(m_fishDieFlipAngleRow, m_fishDieFlipAngleLabel, true);
        setParameterRowVisible(m_fishDieTiltRow, m_fishDieTiltLabel, true);
        setParameterRowVisible(m_fishDieFinFlopRow, m_fishDieFinFlopLabel, true);
        setParameterRowVisible(m_fishDieSpinDecayRow, m_fishDieSpinDecayLabel, true);
    } else if (animationType == "FishForward") {
        // Show fish-specific parameters
        setParameterRowVisible(m_fishForwardSwimSpeedRow, m_fishForwardSwimSpeedLabel, true);
        setParameterRowVisible(m_fishForwardSwimFrequencyRow, m_fishForwardSwimFrequencyLabel, true);
        setParameterRowVisible(m_fishForwardSpineAmplitudeRow, m_fishForwardSpineAmplitudeLabel, true);
        setParameterRowVisible(m_fishForwardWaveLengthRow, m_fishForwardWaveLengthLabel, true);
        setParameterRowVisible(m_fishForwardTailAmplitudeRatioRow, m_fishForwardTailAmplitudeRatioLabel, true);
        setParameterRowVisible(m_fishForwardBodyRollRow, m_fishForwardBodyRollLabel, true);
        setParameterRowVisible(m_fishForwardForwardThrustRow, m_fishForwardForwardThrustLabel, true);
        setParameterRowVisible(m_fishForwardPectoralFlapPowerRow, m_fishForwardPectoralFlapPowerLabel, true);
        setParameterRowVisible(m_fishForwardPelvicFlapPowerRow, m_fishForwardPelvicFlapPowerLabel, true);
        setParameterRowVisible(m_fishForwardDorsalSwayPowerRow, m_fishForwardDorsalSwayPowerLabel, true);
        setParameterRowVisible(m_fishForwardVentralSwayPowerRow, m_fishForwardVentralSwayPowerLabel, true);
        setParameterRowVisible(m_fishForwardPectoralPhaseOffsetRow, m_fishForwardPectoralPhaseOffsetLabel, true);
        setParameterRowVisible(m_fishForwardPelvicPhaseOffsetRow, m_fishForwardPelvicPhaseOffsetLabel, true);
    } else if (animationType == "SnakeForward") {
        setParameterRowVisible(m_snakeForwardWaveSpeedRow, m_snakeForwardWaveSpeedLabel, true);
        setParameterRowVisible(m_snakeForwardWaveFrequencyRow, m_snakeForwardWaveFrequencyLabel, true);
        setParameterRowVisible(m_snakeForwardWaveAmplitudeRow, m_snakeForwardWaveAmplitudeLabel, true);
        setParameterRowVisible(m_snakeForwardWaveLengthRow, m_snakeForwardWaveLengthLabel, true);
        setParameterRowVisible(m_snakeForwardTailAmplitudeRatioRow, m_snakeForwardTailAmplitudeRatioLabel, true);
        setParameterRowVisible(m_snakeForwardHeadYawFactorRow, m_snakeForwardHeadYawFactorLabel, true);
        setParameterRowVisible(m_snakeForwardHeadPullFactorRow, m_snakeForwardHeadPullFactorLabel, true);
        setParameterRowVisible(m_snakeForwardJawAmplitudeRow, m_snakeForwardJawAmplitudeLabel, true);
        setParameterRowVisible(m_snakeForwardJawFrequencyRow, m_snakeForwardJawFrequencyLabel, true);
    } else if (animationType == "InsectRubHands") {
        setParameterRowVisible(m_sharedStepLengthRow, m_sharedStepLengthLabel, true);
        setParameterRowVisible(m_sharedStepHeightRow, m_sharedStepHeightLabel, true);
        setParameterRowVisible(m_sharedBodyBobRow, m_sharedBodyBobLabel, true);
        setParameterRowVisible(m_sharedGaitSpeedRow, m_sharedGaitSpeedLabel, true);
        setParameterRowVisible(m_insectRubHandsRubForwardOffsetRow, m_insectRubHandsRubForwardOffsetLabel, true);
        setParameterRowVisible(m_insectRubHandsRubUpOffsetRow, m_insectRubHandsRubUpOffsetLabel, true);
    } else if (animationType == "QuadrupedWalk") {
        setParameterRowVisible(m_sharedStepLengthRow, m_sharedStepLengthLabel, true);
        setParameterRowVisible(m_sharedStepHeightRow, m_sharedStepHeightLabel, true);
        setParameterRowVisible(m_sharedBodyBobRow, m_sharedBodyBobLabel, true);
        setParameterRowVisible(m_sharedGaitSpeedRow, m_sharedGaitSpeedLabel, true);
        setParameterRowVisible(m_sharedSpineFlexRow, m_sharedSpineFlexLabel, true);
        setParameterRowVisible(m_sharedTailSwayRow, m_sharedTailSwayLabel, true);
    } else if (animationType == "QuadrupedRun") {
        setParameterRowVisible(m_sharedStepLengthRow, m_sharedStepLengthLabel, true);
        setParameterRowVisible(m_sharedStepHeightRow, m_sharedStepHeightLabel, true);
        setParameterRowVisible(m_sharedBodyBobRow, m_sharedBodyBobLabel, true);
        setParameterRowVisible(m_sharedGaitSpeedRow, m_sharedGaitSpeedLabel, true);
        setParameterRowVisible(m_sharedSpineFlexRow, m_sharedSpineFlexLabel, true);
        setParameterRowVisible(m_sharedTailSwayRow, m_sharedTailSwayLabel, true);
        setParameterRowVisible(m_sharedSuspensionRow, m_sharedSuspensionLabel, true);
        setParameterRowVisible(m_quadrupedRunForwardLeanRow, m_quadrupedRunForwardLeanLabel, true);
        setParameterRowVisible(m_sharedStrideFrequencyRow, m_sharedStrideFrequencyLabel, true);
        setParameterRowVisible(m_quadrupedRunBoundRow, m_quadrupedRunBoundLabel, true);
    } else if (animationType == "BipedWalk") {
        setParameterRowVisible(m_sharedStepLengthRow, m_sharedStepLengthLabel, true);
        setParameterRowVisible(m_sharedStepHeightRow, m_sharedStepHeightLabel, true);
        setParameterRowVisible(m_sharedBodyBobRow, m_sharedBodyBobLabel, true);
        setParameterRowVisible(m_sharedGaitSpeedRow, m_sharedGaitSpeedLabel, true);
        setParameterRowVisible(m_sharedArmSwingRow, m_sharedArmSwingLabel, true);
        setParameterRowVisible(m_sharedHipSwayRow, m_sharedHipSwayLabel, true);
        setParameterRowVisible(m_sharedHipRotateRow, m_sharedHipRotateLabel, true);
        setParameterRowVisible(m_sharedBipedSpineFlexRow, m_sharedBipedSpineFlexLabel, true);
        setParameterRowVisible(m_sharedHeadBobRow, m_sharedHeadBobLabel, true);
        setParameterRowVisible(m_sharedKneeBendRow, m_sharedKneeBendLabel, true);
        setParameterRowVisible(m_sharedLeanForwardRow, m_sharedLeanForwardLabel, true);
        setParameterRowVisible(m_sharedBouncinessRow, m_sharedBouncinessLabel, true);
        setParameterRowVisible(m_sharedTailSwayRow, m_sharedTailSwayLabel, true);
    } else if (animationType == "BipedRun") {
        setParameterRowVisible(m_sharedStepLengthRow, m_sharedStepLengthLabel, true);
        setParameterRowVisible(m_sharedStepHeightRow, m_sharedStepHeightLabel, true);
        setParameterRowVisible(m_sharedBodyBobRow, m_sharedBodyBobLabel, true);
        setParameterRowVisible(m_sharedGaitSpeedRow, m_sharedGaitSpeedLabel, true);
        setParameterRowVisible(m_sharedArmSwingRow, m_sharedArmSwingLabel, true);
        setParameterRowVisible(m_sharedHipSwayRow, m_sharedHipSwayLabel, true);
        setParameterRowVisible(m_sharedHipRotateRow, m_sharedHipRotateLabel, true);
        setParameterRowVisible(m_sharedBipedSpineFlexRow, m_sharedBipedSpineFlexLabel, true);
        setParameterRowVisible(m_sharedHeadBobRow, m_sharedHeadBobLabel, true);
        setParameterRowVisible(m_sharedKneeBendRow, m_sharedKneeBendLabel, true);
        setParameterRowVisible(m_sharedLeanForwardRow, m_sharedLeanForwardLabel, true);
        setParameterRowVisible(m_sharedBouncinessRow, m_sharedBouncinessLabel, true);
        setParameterRowVisible(m_sharedTailSwayRow, m_sharedTailSwayLabel, true);
        setParameterRowVisible(m_sharedSuspensionRow, m_sharedSuspensionLabel, true);
        setParameterRowVisible(m_sharedStrideFrequencyRow, m_sharedStrideFrequencyLabel, true);
    } else if (animationType == "QuadrupedDie") {
        setParameterRowVisible(m_quadDieCollapseSpeedRow, m_quadDieCollapseSpeedLabel, true);
        setParameterRowVisible(m_quadDieLegSpreadRow, m_quadDieLegSpreadLabel, true);
        setParameterRowVisible(m_quadDieRollIntensityRow, m_quadDieRollIntensityLabel, true);
        setParameterRowVisible(m_sharedDieLengthStiffnessRow, m_sharedDieLengthStiffnessLabel, true);
        setParameterRowVisible(m_sharedDieParentStiffnessRow, m_sharedDieParentStiffnessLabel, true);
        setParameterRowVisible(m_sharedDieMaxJointAngleRow, m_sharedDieMaxJointAngleLabel, true);
        setParameterRowVisible(m_sharedDieDampingRow, m_sharedDieDampingLabel, true);
        setParameterRowVisible(m_sharedDieGroundBounceRow, m_sharedDieGroundBounceLabel, true);
    } else if (animationType == "BipedDie") {
        setParameterRowVisible(m_bipedDieCollapseSpeedRow, m_bipedDieCollapseSpeedLabel, true);
        setParameterRowVisible(m_bipedDieArmFlailRow, m_bipedDieArmFlailLabel, true);
        setParameterRowVisible(m_bipedDieHeadDropRow, m_bipedDieHeadDropLabel, true);
        setParameterRowVisible(m_sharedDieLengthStiffnessRow, m_sharedDieLengthStiffnessLabel, true);
        setParameterRowVisible(m_sharedDieParentStiffnessRow, m_sharedDieParentStiffnessLabel, true);
        setParameterRowVisible(m_sharedDieMaxJointAngleRow, m_sharedDieMaxJointAngleLabel, true);
        setParameterRowVisible(m_sharedDieDampingRow, m_sharedDieDampingLabel, true);
        setParameterRowVisible(m_sharedDieGroundBounceRow, m_sharedDieGroundBounceLabel, true);
    } else if (animationType == "BirdDie") {
        setParameterRowVisible(m_birdDieCollapseSpeedRow, m_birdDieCollapseSpeedLabel, true);
        setParameterRowVisible(m_birdDieWingFlapRow, m_birdDieWingFlapLabel, true);
        setParameterRowVisible(m_birdDieRollIntensityRow, m_birdDieRollIntensityLabel, true);
        setParameterRowVisible(m_sharedDieLengthStiffnessRow, m_sharedDieLengthStiffnessLabel, true);
        setParameterRowVisible(m_sharedDieParentStiffnessRow, m_sharedDieParentStiffnessLabel, true);
        setParameterRowVisible(m_sharedDieMaxJointAngleRow, m_sharedDieMaxJointAngleLabel, true);
        setParameterRowVisible(m_sharedDieDampingRow, m_sharedDieDampingLabel, true);
        setParameterRowVisible(m_sharedDieGroundBounceRow, m_sharedDieGroundBounceLabel, true);
    } else if (animationType == "SnakeDie") {
        setParameterRowVisible(m_snakeDieCollapseSpeedRow, m_snakeDieCollapseSpeedLabel, true);
        setParameterRowVisible(m_snakeDieCoilFactorRow, m_snakeDieCoilFactorLabel, true);
        setParameterRowVisible(m_snakeDieJawOpenRow, m_snakeDieJawOpenLabel, true);
    } else if (animationType == "SpiderDie") {
        setParameterRowVisible(m_spiderDieCollapseSpeedRow, m_spiderDieCollapseSpeedLabel, true);
        setParameterRowVisible(m_spiderDieLegSpreadRow, m_spiderDieLegSpreadLabel, true);
        setParameterRowVisible(m_sharedDieLengthStiffnessRow, m_sharedDieLengthStiffnessLabel, true);
        setParameterRowVisible(m_sharedDieParentStiffnessRow, m_sharedDieParentStiffnessLabel, true);
        setParameterRowVisible(m_sharedDieMaxJointAngleRow, m_sharedDieMaxJointAngleLabel, true);
        setParameterRowVisible(m_sharedDieDampingRow, m_sharedDieDampingLabel, true);
        setParameterRowVisible(m_sharedDieGroundBounceRow, m_sharedDieGroundBounceLabel, true);
    } else if (animationType == "SpiderWalk") {
        setParameterRowVisible(m_sharedStepLengthRow, m_sharedStepLengthLabel, true);
        setParameterRowVisible(m_sharedStepHeightRow, m_sharedStepHeightLabel, true);
        setParameterRowVisible(m_sharedBodyBobRow, m_sharedBodyBobLabel, true);
        setParameterRowVisible(m_sharedGaitSpeedRow, m_sharedGaitSpeedLabel, true);
        setParameterRowVisible(m_spiderLegSpreadRow, m_spiderLegSpreadLabel, true);
        setParameterRowVisible(m_spiderAbdomenSwayRow, m_spiderAbdomenSwayLabel, true);
        setParameterRowVisible(m_spiderPedipalpSwayRow, m_spiderPedipalpSwayLabel, true);
        setParameterRowVisible(m_spiderBodyYawRow, m_spiderBodyYawLabel, true);
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
        m_animationNameCombo->addItem("InsectDie");
        m_animationNameCombo->setEnabled(true);
        m_addAnimationButton->setEnabled(true);
    } else if (rigType.compare("Bird", Qt::CaseInsensitive) == 0) {
        m_animationNameCombo->addItem("BirdForward");
        m_animationNameCombo->addItem("BirdDie");
        m_animationNameCombo->setEnabled(true);
        m_addAnimationButton->setEnabled(true);
    } else if (rigType.compare("Fish", Qt::CaseInsensitive) == 0) {
        m_animationNameCombo->addItem("FishForward");
        m_animationNameCombo->addItem("FishDie");
        m_animationNameCombo->setEnabled(true);
        m_addAnimationButton->setEnabled(true);
    } else if (rigType.compare("Biped", Qt::CaseInsensitive) == 0) {
        m_animationNameCombo->addItem("BipedWalk");
        m_animationNameCombo->addItem("BipedRun");
        m_animationNameCombo->addItem("BipedDie");
        m_animationNameCombo->setEnabled(true);
        m_addAnimationButton->setEnabled(true);
    } else if (rigType.compare("Quadruped", Qt::CaseInsensitive) == 0) {
        m_animationNameCombo->addItem("QuadrupedWalk");
        m_animationNameCombo->addItem("QuadrupedRun");
        m_animationNameCombo->addItem("QuadrupedDie");
        m_animationNameCombo->setEnabled(true);
        m_addAnimationButton->setEnabled(true);
    } else if (rigType.compare("Spider", Qt::CaseInsensitive) == 0) {
        m_animationNameCombo->addItem("SpiderWalk");
        m_animationNameCombo->addItem("SpiderDie");
        m_animationNameCombo->setEnabled(true);
        m_addAnimationButton->setEnabled(true);
    } else if (rigType.compare("Snake", Qt::CaseInsensitive) == 0) {
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
    if (!m_sharedStepLengthSlider || !m_sharedStepHeightSlider || !m_sharedBodyBobSlider || !m_sharedGaitSpeedSlider || !m_insectRubHandsRubForwardOffsetSlider || !m_insectRubHandsRubUpOffsetSlider || !m_sharedUseFabrikCheck || !m_sharedPlaneStabilizationCheck)
        return;

    // Standard parameters
    m_animationParams.setValue("stepLengthFactor", m_sharedStepLengthSlider->value() / 100.0);
    m_animationParams.setValue("stepHeightFactor", m_sharedStepHeightSlider->value() / 100.0);
    m_animationParams.setValue("bodyBobFactor", m_sharedBodyBobSlider->value() / 100.0);
    m_animationParams.setValue("gaitSpeedFactor", m_sharedGaitSpeedSlider->value() / 10.0);
    m_animationParams.setValue("rubForwardOffsetFactor", m_insectRubHandsRubForwardOffsetSlider->value() / 100.0);
    m_animationParams.setValue("rubUpOffsetFactor", m_insectRubHandsRubUpOffsetSlider->value() / 100.0);
    m_animationParams.setBool("useFabrikIk", m_sharedUseFabrikCheck->isChecked());
    m_animationParams.setBool("planeStabilization", m_sharedPlaneStabilizationCheck->isChecked());

    // Quadruped walk parameters
    if (m_sharedSpineFlexSlider)
        m_animationParams.setValue("spineFlexFactor", m_sharedSpineFlexSlider->value() / 100.0);
    if (m_sharedTailSwaySlider)
        m_animationParams.setValue("tailSwayFactor", m_sharedTailSwaySlider->value() / 100.0);

    // Biped walk parameters
    if (m_sharedArmSwingSlider)
        m_animationParams.setValue("armSwingFactor", m_sharedArmSwingSlider->value() / 100.0);
    if (m_sharedHipSwaySlider)
        m_animationParams.setValue("hipSwayFactor", m_sharedHipSwaySlider->value() / 100.0);
    if (m_sharedHipRotateSlider)
        m_animationParams.setValue("hipRotateFactor", m_sharedHipRotateSlider->value() / 100.0);
    if (m_sharedBipedSpineFlexSlider)
        m_animationParams.setValue("spineFlexFactor", m_sharedBipedSpineFlexSlider->value() / 100.0);
    if (m_sharedHeadBobSlider)
        m_animationParams.setValue("headBobFactor", m_sharedHeadBobSlider->value() / 100.0);
    if (m_sharedKneeBendSlider)
        m_animationParams.setValue("kneeBendFactor", m_sharedKneeBendSlider->value() / 100.0);
    if (m_sharedLeanForwardSlider)
        m_animationParams.setValue("leanForwardFactor", m_sharedLeanForwardSlider->value() / 100.0);
    if (m_sharedBouncinessSlider)
        m_animationParams.setValue("bouncinessFactor", m_sharedBouncinessSlider->value() / 100.0);

    // Quadruped run parameters
    if (m_sharedSuspensionSlider)
        m_animationParams.setValue("suspensionFactor", m_sharedSuspensionSlider->value() / 100.0);
    if (m_quadrupedRunForwardLeanSlider)
        m_animationParams.setValue("forwardLeanFactor", m_quadrupedRunForwardLeanSlider->value() / 100.0);
    if (m_sharedStrideFrequencySlider)
        m_animationParams.setValue("strideFrequencyFactor", m_sharedStrideFrequencySlider->value() / 100.0);
    if (m_quadrupedRunBoundSlider)
        m_animationParams.setValue("boundFactor", m_quadrupedRunBoundSlider->value() / 100.0);

    // Common parameters
    if (m_sharedDurationSpinBox)
        m_animationParams.setValue("durationSeconds", m_sharedDurationSpinBox->value());
    if (m_sharedFrameCountSpinBox)
        m_animationParams.setValue("frameCount", m_sharedFrameCountSpinBox->value());

    const QString animationType = m_animationTypeInput ? m_animationTypeInput->text().trimmed() : QString();
    const bool isInsectWalk = animationType == "InsectWalk" || animationType == "InsectForward" || animationType == "InsectAttack";
    const bool isInsectRubHands = animationType == "InsectRubHands";
    const bool isBirdForward = animationType == "BirdForward";
    const bool isBirdDie = animationType == "BirdDie";
    const bool isFishForward = animationType == "FishForward";
    const bool isFishDie = animationType == "FishDie";
    const bool isSnakeForward = animationType == "SnakeForward";
    const bool isSnakeDie = animationType == "SnakeDie";
    const bool isBipedWalkRun = animationType == "BipedWalk" || animationType == "BipedRun";
    const bool isBipedDie = animationType == "BipedDie";
    const bool isQuadrupedWalk = animationType == "QuadrupedWalk";
    const bool isQuadrupedRun = animationType == "QuadrupedRun";
    const bool isQuadrupedDie = animationType == "QuadrupedDie";
    const bool isSpiderWalk = animationType == "SpiderWalk";
    const bool isSpiderDie = animationType == "SpiderDie";
    const bool isDieAnimation = (animationType.endsWith("Die", Qt::CaseInsensitive) && animationType != "FishDie");
    const bool isIkAnimation = isInsectWalk || isInsectRubHands || isBirdForward || isBipedWalkRun || isQuadrupedWalk || isQuadrupedRun || isSpiderWalk;

    if (isInsectWalk || isInsectRubHands || isBirdForward || isBipedWalkRun || isQuadrupedWalk || isQuadrupedRun || isSpiderWalk) {
        m_animationParams.setValue("stepLengthFactor", m_sharedStepLengthSlider->value() / 100.0);
        m_animationParams.setValue("stepHeightFactor", m_sharedStepHeightSlider->value() / 100.0);
        m_animationParams.setValue("bodyBobFactor", m_sharedBodyBobSlider->value() / 100.0);
        m_animationParams.setValue("gaitSpeedFactor", m_sharedGaitSpeedSlider->value() / 10.0);
    }
    if (isInsectRubHands)
        m_animationParams.setValue("rubForwardOffsetFactor", m_insectRubHandsRubForwardOffsetSlider->value() / 100.0);
    if (isInsectRubHands)
        m_animationParams.setValue("rubUpOffsetFactor", m_insectRubHandsRubUpOffsetSlider->value() / 100.0);
    if (isIkAnimation) {
        m_animationParams.setBool("useFabrikIk", m_sharedUseFabrikCheck->isChecked());
        m_animationParams.setBool("planeStabilization", m_sharedPlaneStabilizationCheck->isChecked());
    }

    if (isBipedWalkRun) {
        if (m_sharedArmSwingSlider)
            m_animationParams.setValue("armSwingFactor", m_sharedArmSwingSlider->value() / 100.0);
        if (m_sharedHipSwaySlider)
            m_animationParams.setValue("hipSwayFactor", m_sharedHipSwaySlider->value() / 100.0);
        if (m_sharedHipRotateSlider)
            m_animationParams.setValue("hipRotateFactor", m_sharedHipRotateSlider->value() / 100.0);
        if (m_sharedBipedSpineFlexSlider)
            m_animationParams.setValue("spineFlexFactor", m_sharedBipedSpineFlexSlider->value() / 100.0);
        if (m_sharedHeadBobSlider)
            m_animationParams.setValue("headBobFactor", m_sharedHeadBobSlider->value() / 100.0);
        if (m_sharedKneeBendSlider)
            m_animationParams.setValue("kneeBendFactor", m_sharedKneeBendSlider->value() / 100.0);
        if (m_sharedLeanForwardSlider)
            m_animationParams.setValue("leanForwardFactor", m_sharedLeanForwardSlider->value() / 100.0);
        if (m_sharedBouncinessSlider)
            m_animationParams.setValue("bouncinessFactor", m_sharedBouncinessSlider->value() / 100.0);
    }

    if (isQuadrupedWalk || isQuadrupedRun) {
        if (m_sharedSpineFlexSlider)
            m_animationParams.setValue("spineFlexFactor", m_sharedSpineFlexSlider->value() / 100.0);
        if (m_sharedTailSwaySlider)
            m_animationParams.setValue("tailSwayFactor", m_sharedTailSwaySlider->value() / 100.0);
    }
    if (isQuadrupedRun) {
        if (m_sharedSuspensionSlider)
            m_animationParams.setValue("suspensionFactor", m_sharedSuspensionSlider->value() / 100.0);
        if (m_quadrupedRunForwardLeanSlider)
            m_animationParams.setValue("forwardLeanFactor", m_quadrupedRunForwardLeanSlider->value() / 100.0);
        if (m_sharedStrideFrequencySlider)
            m_animationParams.setValue("strideFrequencyFactor", m_sharedStrideFrequencySlider->value() / 100.0);
        if (m_quadrupedRunBoundSlider)
            m_animationParams.setValue("boundFactor", m_quadrupedRunBoundSlider->value() / 100.0);
    }

    if (isSpiderWalk) {
        if (m_spiderLegSpreadSlider)
            m_animationParams.setValue("legSpreadFactor", m_spiderLegSpreadSlider->value() / 100.0);
        if (m_spiderAbdomenSwaySlider)
            m_animationParams.setValue("abdomenSwayFactor", m_spiderAbdomenSwaySlider->value() / 100.0);
        if (m_spiderPedipalpSwaySlider)
            m_animationParams.setValue("pedipalpSwayFactor", m_spiderPedipalpSwaySlider->value() / 100.0);
        if (m_spiderBodyYawSlider)
            m_animationParams.setValue("bodyYawFactor", m_spiderBodyYawSlider->value() / 100.0);
    }

    if (isFishForward) {
        if (m_fishForwardSwimSpeedSlider)
            m_animationParams.setValue("swimSpeed", m_fishForwardSwimSpeedSlider->value() / 100.0);
        if (m_fishForwardSwimFrequencySlider)
            m_animationParams.setValue("swimFrequency", m_fishForwardSwimFrequencySlider->value() / 50.0);
        if (m_fishForwardSpineAmplitudeSlider)
            m_animationParams.setValue("spineAmplitude", m_fishForwardSpineAmplitudeSlider->value() / 667.0); // 0.15 at value 100
        if (m_fishForwardWaveLengthSlider)
            m_animationParams.setValue("waveLength", m_fishForwardWaveLengthSlider->value() / 100.0);
        if (m_fishForwardTailAmplitudeRatioSlider)
            m_animationParams.setValue("tailAmplitudeRatio", m_fishForwardTailAmplitudeRatioSlider->value() / 40.0); // 2.5 at value 100
        if (m_fishForwardBodyRollSlider)
            m_animationParams.setValue("bodyRoll", m_fishForwardBodyRollSlider->value() / 2000.0); // 0.05 at value 100
        if (m_fishForwardForwardThrustSlider)
            m_animationParams.setValue("forwardThrust", m_fishForwardForwardThrustSlider->value() / 1250.0); // 0.08 at value 100
        if (m_fishForwardPectoralFlapPowerSlider)
            m_animationParams.setValue("pectoralFlapPower", m_fishForwardPectoralFlapPowerSlider->value() / 250.0); // 0.4 at value 100
        if (m_fishForwardPelvicFlapPowerSlider)
            m_animationParams.setValue("pelvicFlapPower", m_fishForwardPelvicFlapPowerSlider->value() / 400.0); // 0.25 at value 100
        if (m_fishForwardDorsalSwayPowerSlider)
            m_animationParams.setValue("dorsalSwayPower", m_fishForwardDorsalSwayPowerSlider->value() / 500.0); // 0.2 at value 100
        if (m_fishForwardVentralSwayPowerSlider)
            m_animationParams.setValue("ventralSwayPower", m_fishForwardVentralSwayPowerSlider->value() / 500.0); // 0.2 at value 100
        if (m_fishForwardPectoralPhaseOffsetSlider)
            m_animationParams.setValue("pectoralPhaseOffset", m_fishForwardPectoralPhaseOffsetSlider->value() / 100.0);
        if (m_fishForwardPelvicPhaseOffsetSlider)
            m_animationParams.setValue("pelvicPhaseOffset", m_fishForwardPelvicPhaseOffsetSlider->value() / 100.0);
    }

    if (isSnakeForward) {
        if (m_snakeForwardWaveSpeedSlider)
            m_animationParams.setValue("waveSpeed", m_snakeForwardWaveSpeedSlider->value() / 100.0);
        if (m_snakeForwardWaveFrequencySlider)
            m_animationParams.setValue("waveFrequency", m_snakeForwardWaveFrequencySlider->value() / 50.0);
        if (m_snakeForwardWaveAmplitudeSlider)
            m_animationParams.setValue("waveAmplitude", m_snakeForwardWaveAmplitudeSlider->value() / 667.0);
        if (m_snakeForwardWaveLengthSlider)
            m_animationParams.setValue("waveLength", m_snakeForwardWaveLengthSlider->value() / 100.0);
        if (m_snakeForwardTailAmplitudeRatioSlider)
            m_animationParams.setValue("tailAmplitudeRatio", m_snakeForwardTailAmplitudeRatioSlider->value() / 40.0);
        if (m_snakeForwardHeadYawFactorSlider)
            m_animationParams.setValue("headYawFactor", m_snakeForwardHeadYawFactorSlider->value() / 2000.0);
        if (m_snakeForwardHeadPullFactorSlider)
            m_animationParams.setValue("headPullFactor", m_snakeForwardHeadPullFactorSlider->value() / 200.0);
        if (m_snakeForwardJawAmplitudeSlider)
            m_animationParams.setValue("jawAmplitude", m_snakeForwardJawAmplitudeSlider->value() / 500.0);
        if (m_snakeForwardJawFrequencySlider)
            m_animationParams.setValue("jawFrequency", m_snakeForwardJawFrequencySlider->value() / 100.0);
    }

    if (isDieAnimation) {
        if (m_sharedDieLengthStiffnessSlider)
            m_animationParams.setValue("lengthStiffness", m_sharedDieLengthStiffnessSlider->value() / 100.0);
        if (m_sharedDieParentStiffnessSlider)
            m_animationParams.setValue("parentStiffness", m_sharedDieParentStiffnessSlider->value() / 100.0);
        if (m_sharedDieMaxJointAngleSlider)
            m_animationParams.setValue("maxJointAngleDeg", m_sharedDieMaxJointAngleSlider->value());
        if (m_sharedDieDampingSlider)
            m_animationParams.setValue("damping", m_sharedDieDampingSlider->value() / 100.0);
        if (m_sharedDieGroundBounceSlider)
            m_animationParams.setValue("groundBounce", m_sharedDieGroundBounceSlider->value() / 100.0);
    }

    if (isQuadrupedDie) {
        if (m_quadDieCollapseSpeedSlider)
            m_animationParams.setValue("collapseSpeed", m_quadDieCollapseSpeedSlider->value() / 100.0);
        if (m_quadDieLegSpreadSlider)
            m_animationParams.setValue("legSpreadFactor", m_quadDieLegSpreadSlider->value() / 100.0);
        if (m_quadDieRollIntensitySlider)
            m_animationParams.setValue("rollIntensity", m_quadDieRollIntensitySlider->value() / 100.0);
    } else if (isBipedDie) {
        if (m_bipedDieCollapseSpeedSlider)
            m_animationParams.setValue("collapseSpeed", m_bipedDieCollapseSpeedSlider->value() / 100.0);
        if (m_bipedDieArmFlailSlider)
            m_animationParams.setValue("armFlail", m_bipedDieArmFlailSlider->value() / 100.0);
        if (m_bipedDieHeadDropSlider)
            m_animationParams.setValue("headDrop", m_bipedDieHeadDropSlider->value() / 100.0);
    } else if (isBirdDie) {
        if (m_birdDieCollapseSpeedSlider)
            m_animationParams.setValue("collapseSpeed", m_birdDieCollapseSpeedSlider->value() / 100.0);
        if (m_birdDieWingFlapSlider)
            m_animationParams.setValue("wingFlap", m_birdDieWingFlapSlider->value() / 100.0);
        if (m_birdDieRollIntensitySlider)
            m_animationParams.setValue("rollIntensity", m_birdDieRollIntensitySlider->value() / 100.0);
    } else if (isSnakeDie) {
        if (m_snakeDieCollapseSpeedSlider)
            m_animationParams.setValue("flipSpeed", m_snakeDieCollapseSpeedSlider->value() / 100.0);
        if (m_snakeDieCoilFactorSlider)
            m_animationParams.setValue("flipAngle", static_cast<double>(m_snakeDieCoilFactorSlider->value()));
        if (m_snakeDieJawOpenSlider)
            m_animationParams.setValue("jawOpen", static_cast<double>(m_snakeDieJawOpenSlider->value()));
    } else if (isSpiderDie) {
        if (m_spiderDieCollapseSpeedSlider)
            m_animationParams.setValue("collapseSpeed", m_spiderDieCollapseSpeedSlider->value() / 100.0);
        if (m_spiderDieLegSpreadSlider)
            m_animationParams.setValue("legSpreadFactor", m_spiderDieLegSpreadSlider->value() / 100.0);
    }

    if (isFishDie) {
        if (m_fishDieHitIntensitySlider)
            m_animationParams.setValue("hitIntensity", m_fishDieHitIntensitySlider->value() / 100.0);
        if (m_fishDieHitFrequencySlider)
            m_animationParams.setValue("hitFrequency", m_fishDieHitFrequencySlider->value() / 100.0 * 8.0); // 8.0 at value 100
        if (m_fishDieFlipSpeedSlider)
            m_animationParams.setValue("flipSpeed", m_fishDieFlipSpeedSlider->value() / 100.0);
        if (m_fishDieFlipAngleSlider)
            m_animationParams.setValue("flipAngle", static_cast<double>(m_fishDieFlipAngleSlider->value())); // degrees 0–180
        if (m_fishDieTiltSlider)
            m_animationParams.setValue("tilt", m_fishDieTiltSlider->value() / 100.0); // -1.0..+1.0
        if (m_fishDieFinFlopSlider)
            m_animationParams.setValue("finFlop", m_fishDieFinFlopSlider->value() / 100.0);
        if (m_fishDieSpinDecaySlider)
            m_animationParams.setValue("spinDecay", m_fishDieSpinDecaySlider->value() / 100.0 * 4.0); // 4.0 at value 100
    }
}

void AnimationManageWidget::triggerPreviewRegeneration()
{
    if (!m_sharedStepLengthSlider || !m_sharedStepHeightSlider || !m_sharedBodyBobSlider || !m_sharedGaitSpeedSlider || !m_insectRubHandsRubForwardOffsetSlider || !m_insectRubHandsRubUpOffsetSlider || !m_sharedUseFabrikCheck || !m_sharedPlaneStabilizationCheck || !m_hideBonesCheck || !m_hidePartsCheck || !m_hideWeightsCheck)
        return;

    updateAnimationParamsFromWidgets();

    onResultRigChanged();
}

AnimationManageWidget::~AnimationManageWidget()
{
    stopAnimationLoop();
}

void AnimationManageWidget::setWireframeVisible(bool visible)
{
    if (m_modelWidget)
        m_modelWidget->setWireframeVisible(visible);
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

    // Configure frame slider for scrubber
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

    updatePlayPauseIcon();
}

void AnimationManageWidget::stopAnimationLoop()
{
    if (m_frameTimer && m_frameTimer->isActive()) {
        m_frameTimer->stop();
    }
    m_currentFrame = 0;
    updatePlayPauseIcon();
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
    m_animationTypeInput->setFocus();

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
    m_sharedStepLengthSlider->blockSignals(true);
    m_sharedStepHeightSlider->blockSignals(true);
    m_sharedBodyBobSlider->blockSignals(true);
    m_sharedGaitSpeedSlider->blockSignals(true);
    m_sharedUseFabrikCheck->blockSignals(true);
    m_sharedPlaneStabilizationCheck->blockSignals(true);
    m_insectRubHandsRubForwardOffsetSlider->blockSignals(true);
    m_insectRubHandsRubUpOffsetSlider->blockSignals(true);

    // Block signals for quadruped parameters
    if (m_sharedSpineFlexSlider)
        m_sharedSpineFlexSlider->blockSignals(true);
    if (m_sharedTailSwaySlider)
        m_sharedTailSwaySlider->blockSignals(true);

    if (m_sharedDurationSpinBox)
        m_sharedDurationSpinBox->blockSignals(true);
    if (m_sharedFrameCountSpinBox)
        m_sharedFrameCountSpinBox->blockSignals(true);

    // Block signals for insect die parameters
    if (m_sharedDieLengthStiffnessSlider)
        m_sharedDieLengthStiffnessSlider->blockSignals(true);
    if (m_sharedDieParentStiffnessSlider)
        m_sharedDieParentStiffnessSlider->blockSignals(true);
    if (m_sharedDieMaxJointAngleSlider)
        m_sharedDieMaxJointAngleSlider->blockSignals(true);
    if (m_sharedDieDampingSlider)
        m_sharedDieDampingSlider->blockSignals(true);
    if (m_sharedDieGroundBounceSlider)
        m_sharedDieGroundBounceSlider->blockSignals(true);

    // Block signals for quadruped die parameters
    if (m_quadDieCollapseSpeedSlider)
        m_quadDieCollapseSpeedSlider->blockSignals(true);
    if (m_quadDieLegSpreadSlider)
        m_quadDieLegSpreadSlider->blockSignals(true);
    if (m_quadDieRollIntensitySlider)
        m_quadDieRollIntensitySlider->blockSignals(true);

    // Block signals for biped die parameters
    if (m_bipedDieCollapseSpeedSlider)
        m_bipedDieCollapseSpeedSlider->blockSignals(true);
    if (m_bipedDieArmFlailSlider)
        m_bipedDieArmFlailSlider->blockSignals(true);
    if (m_bipedDieHeadDropSlider)
        m_bipedDieHeadDropSlider->blockSignals(true);

    // Block signals for bird die parameters
    if (m_birdDieCollapseSpeedSlider)
        m_birdDieCollapseSpeedSlider->blockSignals(true);
    if (m_birdDieWingFlapSlider)
        m_birdDieWingFlapSlider->blockSignals(true);
    if (m_birdDieRollIntensitySlider)
        m_birdDieRollIntensitySlider->blockSignals(true);

    // Block signals for snake die parameters
    if (m_snakeDieCollapseSpeedSlider)
        m_snakeDieCollapseSpeedSlider->blockSignals(true);
    if (m_snakeDieCoilFactorSlider)
        m_snakeDieCoilFactorSlider->blockSignals(true);
    if (m_snakeDieJawOpenSlider)
        m_snakeDieJawOpenSlider->blockSignals(true);

    // Block signals for spider die parameters
    if (m_spiderDieCollapseSpeedSlider)
        m_spiderDieCollapseSpeedSlider->blockSignals(true);
    if (m_spiderDieLegSpreadSlider)
        m_spiderDieLegSpreadSlider->blockSignals(true);

    // Block signals for fish parameters
    if (m_fishForwardSwimSpeedSlider)
        m_fishForwardSwimSpeedSlider->blockSignals(true);
    if (m_fishForwardSwimFrequencySlider)
        m_fishForwardSwimFrequencySlider->blockSignals(true);
    if (m_fishForwardSpineAmplitudeSlider)
        m_fishForwardSpineAmplitudeSlider->blockSignals(true);
    if (m_fishForwardWaveLengthSlider)
        m_fishForwardWaveLengthSlider->blockSignals(true);
    if (m_fishForwardTailAmplitudeRatioSlider)
        m_fishForwardTailAmplitudeRatioSlider->blockSignals(true);
    if (m_fishForwardBodyRollSlider)
        m_fishForwardBodyRollSlider->blockSignals(true);
    if (m_fishForwardForwardThrustSlider)
        m_fishForwardForwardThrustSlider->blockSignals(true);
    if (m_fishForwardPectoralFlapPowerSlider)
        m_fishForwardPectoralFlapPowerSlider->blockSignals(true);
    if (m_fishForwardPelvicFlapPowerSlider)
        m_fishForwardPelvicFlapPowerSlider->blockSignals(true);
    if (m_fishForwardDorsalSwayPowerSlider)
        m_fishForwardDorsalSwayPowerSlider->blockSignals(true);
    if (m_fishForwardVentralSwayPowerSlider)
        m_fishForwardVentralSwayPowerSlider->blockSignals(true);
    if (m_fishForwardPectoralPhaseOffsetSlider)
        m_fishForwardPectoralPhaseOffsetSlider->blockSignals(true);
    if (m_fishForwardPelvicPhaseOffsetSlider)
        m_fishForwardPelvicPhaseOffsetSlider->blockSignals(true);
    if (m_snakeForwardWaveSpeedSlider)
        m_snakeForwardWaveSpeedSlider->blockSignals(true);
    if (m_snakeForwardWaveFrequencySlider)
        m_snakeForwardWaveFrequencySlider->blockSignals(true);
    if (m_snakeForwardWaveAmplitudeSlider)
        m_snakeForwardWaveAmplitudeSlider->blockSignals(true);
    if (m_snakeForwardWaveLengthSlider)
        m_snakeForwardWaveLengthSlider->blockSignals(true);
    if (m_snakeForwardTailAmplitudeRatioSlider)
        m_snakeForwardTailAmplitudeRatioSlider->blockSignals(true);
    if (m_snakeForwardHeadYawFactorSlider)
        m_snakeForwardHeadYawFactorSlider->blockSignals(true);

    m_sharedStepLengthSlider->setValue(static_cast<int>(params.getValue("stepLengthFactor", 1.0) * 100));
    m_sharedStepHeightSlider->setValue(static_cast<int>(params.getValue("stepHeightFactor", 1.0) * 100));
    m_sharedBodyBobSlider->setValue(static_cast<int>(params.getValue("bodyBobFactor", 1.0) * 100));
    m_sharedGaitSpeedSlider->setValue(static_cast<int>(params.getValue("gaitSpeedFactor", 1.0) * 10));
    m_insectRubHandsRubForwardOffsetSlider->setValue(static_cast<int>(params.getValue("rubForwardOffsetFactor", 1.0) * 100));
    m_insectRubHandsRubUpOffsetSlider->setValue(static_cast<int>(params.getValue("rubUpOffsetFactor", 1.0) * 100));
    m_sharedUseFabrikCheck->setChecked(params.getBool("useFabrikIk", true));
    m_sharedPlaneStabilizationCheck->setChecked(params.getBool("planeStabilization", true));

    // Load quadruped walk parameters
    if (m_sharedSpineFlexSlider)
        m_sharedSpineFlexSlider->setValue(static_cast<int>(params.getValue("spineFlexFactor", 1.0) * 100));
    if (m_sharedTailSwaySlider)
        m_sharedTailSwaySlider->setValue(static_cast<int>(params.getValue("tailSwayFactor", 1.0) * 100));

    // Load quadruped run parameters
    if (m_sharedSuspensionSlider)
        m_sharedSuspensionSlider->setValue(static_cast<int>(params.getValue("suspensionFactor", 1.0) * 100));
    if (m_quadrupedRunForwardLeanSlider)
        m_quadrupedRunForwardLeanSlider->setValue(static_cast<int>(params.getValue("forwardLeanFactor", 1.0) * 100));
    if (m_sharedStrideFrequencySlider)
        m_sharedStrideFrequencySlider->setValue(static_cast<int>(params.getValue("strideFrequencyFactor", 1.0) * 100));
    if (m_quadrupedRunBoundSlider)
        m_quadrupedRunBoundSlider->setValue(static_cast<int>(params.getValue("boundFactor", 0.0) * 100));

    if (m_sharedDurationSpinBox)
        m_sharedDurationSpinBox->setValue(params.getValue("durationSeconds", 3.0));
    if (m_sharedFrameCountSpinBox)
        m_sharedFrameCountSpinBox->setValue(static_cast<int>(params.getValue("frameCount", 90.0)));

    // Load insect die parameters
    if (m_sharedDieLengthStiffnessSlider)
        m_sharedDieLengthStiffnessSlider->setValue(static_cast<int>(params.getValue("lengthStiffness", 0.9) * 100));
    if (m_sharedDieParentStiffnessSlider)
        m_sharedDieParentStiffnessSlider->setValue(static_cast<int>(params.getValue("parentStiffness", 0.8) * 100));
    if (m_sharedDieMaxJointAngleSlider)
        m_sharedDieMaxJointAngleSlider->setValue(static_cast<int>(params.getValue("maxJointAngleDeg", 120.0)));
    if (m_sharedDieDampingSlider)
        m_sharedDieDampingSlider->setValue(static_cast<int>(params.getValue("damping", 0.95) * 100));
    if (m_sharedDieGroundBounceSlider)
        m_sharedDieGroundBounceSlider->setValue(static_cast<int>(params.getValue("groundBounce", 0.22) * 100));

    // Load quadruped die parameter values
    if (m_quadDieCollapseSpeedSlider)
        m_quadDieCollapseSpeedSlider->setValue(static_cast<int>(params.getValue("collapseSpeed", 1.0) * 100));
    if (m_quadDieLegSpreadSlider)
        m_quadDieLegSpreadSlider->setValue(static_cast<int>(params.getValue("legSpreadFactor", 1.0) * 100));
    if (m_quadDieRollIntensitySlider)
        m_quadDieRollIntensitySlider->setValue(static_cast<int>(params.getValue("rollIntensity", 1.0) * 100));

    // Load biped die parameter values
    if (m_bipedDieCollapseSpeedSlider)
        m_bipedDieCollapseSpeedSlider->setValue(static_cast<int>(params.getValue("collapseSpeed", 1.0) * 100));
    if (m_bipedDieArmFlailSlider)
        m_bipedDieArmFlailSlider->setValue(static_cast<int>(params.getValue("armFlail", 1.0) * 100));
    if (m_bipedDieHeadDropSlider)
        m_bipedDieHeadDropSlider->setValue(static_cast<int>(params.getValue("headDrop", 1.0) * 100));

    // Load bird die parameter values
    if (m_birdDieCollapseSpeedSlider)
        m_birdDieCollapseSpeedSlider->setValue(static_cast<int>(params.getValue("collapseSpeed", 1.0) * 100));
    if (m_birdDieWingFlapSlider)
        m_birdDieWingFlapSlider->setValue(static_cast<int>(params.getValue("wingFlap", 1.0) * 100));
    if (m_birdDieRollIntensitySlider)
        m_birdDieRollIntensitySlider->setValue(static_cast<int>(params.getValue("rollIntensity", 1.0) * 100));

    // Load snake die parameter values
    if (m_snakeDieCollapseSpeedSlider)
        m_snakeDieCollapseSpeedSlider->setValue(static_cast<int>(params.getValue("flipSpeed", 1.0) * 100));
    if (m_snakeDieCoilFactorSlider)
        m_snakeDieCoilFactorSlider->setValue(static_cast<int>(params.getValue("flipAngle", 180.0)));
    if (m_snakeDieJawOpenSlider)
        m_snakeDieJawOpenSlider->setValue(static_cast<int>(params.getValue("jawOpen", 63.0)));

    // Load spider die parameter values
    if (m_spiderDieCollapseSpeedSlider)
        m_spiderDieCollapseSpeedSlider->setValue(static_cast<int>(params.getValue("collapseSpeed", 1.0) * 100));
    if (m_spiderDieLegSpreadSlider)
        m_spiderDieLegSpreadSlider->setValue(static_cast<int>(params.getValue("legSpreadFactor", 1.0) * 100));

    // Load fish parameter values (using default values matching the animation implementation)
    if (m_fishForwardSwimSpeedSlider)
        m_fishForwardSwimSpeedSlider->setValue(static_cast<int>(params.getValue("swimSpeed", 1.0) * 100));
    if (m_fishForwardSwimFrequencySlider)
        m_fishForwardSwimFrequencySlider->setValue(static_cast<int>(params.getValue("swimFrequency", 2.0) * 50));
    if (m_fishForwardSpineAmplitudeSlider)
        m_fishForwardSpineAmplitudeSlider->setValue(static_cast<int>(params.getValue("spineAmplitude", 0.15) * 667));
    if (m_fishForwardWaveLengthSlider)
        m_fishForwardWaveLengthSlider->setValue(static_cast<int>(params.getValue("waveLength", 1.0) * 100));
    if (m_fishForwardTailAmplitudeRatioSlider)
        m_fishForwardTailAmplitudeRatioSlider->setValue(static_cast<int>(params.getValue("tailAmplitudeRatio", 2.5) * 40));
    if (m_fishForwardBodyRollSlider)
        m_fishForwardBodyRollSlider->setValue(static_cast<int>(params.getValue("bodyRoll", 0.05) * 2000));
    if (m_fishForwardForwardThrustSlider)
        m_fishForwardForwardThrustSlider->setValue(static_cast<int>(params.getValue("forwardThrust", 0.08) * 1250));
    if (m_fishForwardPectoralFlapPowerSlider)
        m_fishForwardPectoralFlapPowerSlider->setValue(static_cast<int>(params.getValue("pectoralFlapPower", 0.4) * 250));
    if (m_fishForwardPelvicFlapPowerSlider)
        m_fishForwardPelvicFlapPowerSlider->setValue(static_cast<int>(params.getValue("pelvicFlapPower", 0.25) * 400));
    if (m_fishForwardDorsalSwayPowerSlider)
        m_fishForwardDorsalSwayPowerSlider->setValue(static_cast<int>(params.getValue("dorsalSwayPower", 0.2) * 500));
    if (m_fishForwardVentralSwayPowerSlider)
        m_fishForwardVentralSwayPowerSlider->setValue(static_cast<int>(params.getValue("ventralSwayPower", 0.2) * 500));
    if (m_fishForwardPectoralPhaseOffsetSlider)
        m_fishForwardPectoralPhaseOffsetSlider->setValue(static_cast<int>(params.getValue("pectoralPhaseOffset", 0.0) * 100));
    if (m_fishForwardPelvicPhaseOffsetSlider)
        m_fishForwardPelvicPhaseOffsetSlider->setValue(static_cast<int>(params.getValue("pelvicPhaseOffset", 0.5) * 100));

    if (m_snakeForwardWaveSpeedSlider)
        m_snakeForwardWaveSpeedSlider->setValue(static_cast<int>(params.getValue("waveSpeed", 1.0) * 100));
    if (m_snakeForwardWaveFrequencySlider)
        m_snakeForwardWaveFrequencySlider->setValue(static_cast<int>(params.getValue("waveFrequency", 2.0) * 50));
    if (m_snakeForwardWaveAmplitudeSlider)
        m_snakeForwardWaveAmplitudeSlider->setValue(static_cast<int>(params.getValue("waveAmplitude", 0.15) * 667));
    if (m_snakeForwardWaveLengthSlider)
        m_snakeForwardWaveLengthSlider->setValue(static_cast<int>(params.getValue("waveLength", 1.0) * 100));
    if (m_snakeForwardTailAmplitudeRatioSlider)
        m_snakeForwardTailAmplitudeRatioSlider->setValue(static_cast<int>(params.getValue("tailAmplitudeRatio", 2.5) * 40));
    if (m_snakeForwardHeadYawFactorSlider)
        m_snakeForwardHeadYawFactorSlider->setValue(static_cast<int>(params.getValue("headYawFactor", 0.05) * 2000));
    if (m_snakeForwardHeadPullFactorSlider)
        m_snakeForwardHeadPullFactorSlider->setValue(static_cast<int>(params.getValue("headPullFactor", 0.2) * 200));
    if (m_snakeForwardJawAmplitudeSlider)
        m_snakeForwardJawAmplitudeSlider->setValue(static_cast<int>(params.getValue("jawAmplitude", 0.25) * 500));
    if (m_snakeForwardJawFrequencySlider)
        m_snakeForwardJawFrequencySlider->setValue(static_cast<int>(params.getValue("jawFrequency", 1.0) * 100));

    m_sharedStepLengthSlider->blockSignals(false);
    m_sharedStepHeightSlider->blockSignals(false);
    m_sharedBodyBobSlider->blockSignals(false);
    m_sharedGaitSpeedSlider->blockSignals(false);
    m_sharedUseFabrikCheck->blockSignals(false);
    m_sharedPlaneStabilizationCheck->blockSignals(false);
    m_insectRubHandsRubForwardOffsetSlider->blockSignals(false);
    m_insectRubHandsRubUpOffsetSlider->blockSignals(false);

    // Unblock signals for quadruped parameters
    if (m_sharedSpineFlexSlider)
        m_sharedSpineFlexSlider->blockSignals(false);
    if (m_sharedTailSwaySlider)
        m_sharedTailSwaySlider->blockSignals(false);

    if (m_sharedDurationSpinBox)
        m_sharedDurationSpinBox->blockSignals(false);
    if (m_sharedFrameCountSpinBox)
        m_sharedFrameCountSpinBox->blockSignals(false);

    // Unblock signals for insect die parameters
    if (m_sharedDieLengthStiffnessSlider)
        m_sharedDieLengthStiffnessSlider->blockSignals(false);
    if (m_sharedDieParentStiffnessSlider)
        m_sharedDieParentStiffnessSlider->blockSignals(false);
    if (m_sharedDieMaxJointAngleSlider)
        m_sharedDieMaxJointAngleSlider->blockSignals(false);
    if (m_sharedDieDampingSlider)
        m_sharedDieDampingSlider->blockSignals(false);
    if (m_sharedDieGroundBounceSlider)
        m_sharedDieGroundBounceSlider->blockSignals(false);

    // Unblock signals for quadruped die parameters
    if (m_quadDieCollapseSpeedSlider)
        m_quadDieCollapseSpeedSlider->blockSignals(false);
    if (m_quadDieLegSpreadSlider)
        m_quadDieLegSpreadSlider->blockSignals(false);
    if (m_quadDieRollIntensitySlider)
        m_quadDieRollIntensitySlider->blockSignals(false);

    if (m_bipedDieCollapseSpeedSlider)
        m_bipedDieCollapseSpeedSlider->blockSignals(false);
    if (m_bipedDieArmFlailSlider)
        m_bipedDieArmFlailSlider->blockSignals(false);
    if (m_bipedDieHeadDropSlider)
        m_bipedDieHeadDropSlider->blockSignals(false);

    if (m_birdDieCollapseSpeedSlider)
        m_birdDieCollapseSpeedSlider->blockSignals(false);
    if (m_birdDieWingFlapSlider)
        m_birdDieWingFlapSlider->blockSignals(false);
    if (m_birdDieRollIntensitySlider)
        m_birdDieRollIntensitySlider->blockSignals(false);

    if (m_snakeDieCollapseSpeedSlider)
        m_snakeDieCollapseSpeedSlider->blockSignals(false);
    if (m_snakeDieCoilFactorSlider)
        m_snakeDieCoilFactorSlider->blockSignals(false);
    if (m_snakeDieJawOpenSlider)
        m_snakeDieJawOpenSlider->blockSignals(false);

    if (m_spiderDieCollapseSpeedSlider)
        m_spiderDieCollapseSpeedSlider->blockSignals(false);
    if (m_spiderDieLegSpreadSlider)
        m_spiderDieLegSpreadSlider->blockSignals(false);

    // Unblock signals for fish parameters
    if (m_fishForwardSwimSpeedSlider)
        m_fishForwardSwimSpeedSlider->blockSignals(false);
    if (m_fishForwardSwimFrequencySlider)
        m_fishForwardSwimFrequencySlider->blockSignals(false);
    if (m_fishForwardSpineAmplitudeSlider)
        m_fishForwardSpineAmplitudeSlider->blockSignals(false);
    if (m_fishForwardWaveLengthSlider)
        m_fishForwardWaveLengthSlider->blockSignals(false);
    if (m_fishForwardTailAmplitudeRatioSlider)
        m_fishForwardTailAmplitudeRatioSlider->blockSignals(false);
    if (m_fishForwardBodyRollSlider)
        m_fishForwardBodyRollSlider->blockSignals(false);
    if (m_fishForwardForwardThrustSlider)
        m_fishForwardForwardThrustSlider->blockSignals(false);
    if (m_fishForwardPectoralFlapPowerSlider)
        m_fishForwardPectoralFlapPowerSlider->blockSignals(false);
    if (m_fishForwardPelvicFlapPowerSlider)
        m_fishForwardPelvicFlapPowerSlider->blockSignals(false);
    if (m_fishForwardDorsalSwayPowerSlider)
        m_fishForwardDorsalSwayPowerSlider->blockSignals(false);
    if (m_fishForwardVentralSwayPowerSlider)
        m_fishForwardVentralSwayPowerSlider->blockSignals(false);
    if (m_fishForwardPectoralPhaseOffsetSlider)
        m_fishForwardPectoralPhaseOffsetSlider->blockSignals(false);
    if (m_fishForwardPelvicPhaseOffsetSlider)
        m_fishForwardPelvicPhaseOffsetSlider->blockSignals(false);
    if (m_snakeForwardWaveSpeedSlider)
        m_snakeForwardWaveSpeedSlider->blockSignals(false);
    if (m_snakeForwardWaveFrequencySlider)
        m_snakeForwardWaveFrequencySlider->blockSignals(false);
    if (m_snakeForwardWaveAmplitudeSlider)
        m_snakeForwardWaveAmplitudeSlider->blockSignals(false);
    if (m_snakeForwardWaveLengthSlider)
        m_snakeForwardWaveLengthSlider->blockSignals(false);
    if (m_snakeForwardTailAmplitudeRatioSlider)
        m_snakeForwardTailAmplitudeRatioSlider->blockSignals(false);
    if (m_snakeForwardHeadYawFactorSlider)
        m_snakeForwardHeadYawFactorSlider->blockSignals(false);

    // Update title and trigger preview
    m_parametersGroupBox->setTitle(tr("Parameters (") + anim->name + ")");

    // Store the loaded params for preview
    m_animationParams = params;
    triggerPreviewRegeneration();
}

void AnimationManageWidget::onSelectedBoneChanged(const QString& boneName)
{
    m_selectedBoneName = boneName;
    // Trigger preview regeneration with new bone selection
    triggerPreviewRegeneration();
}
