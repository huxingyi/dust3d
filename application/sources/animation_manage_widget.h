#ifndef DUST3D_APPLICATION_ANIMATION_MANAGE_WIDGET_H_
#define DUST3D_APPLICATION_ANIMATION_MANAGE_WIDGET_H_

#include "animation_preview_worker.h"
#include "model_mesh.h"
#include "world_widget.h"
#include <QCheckBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include <QSlider>
#include <QSpinBox>
#include <QThread>
#include <QTimer>
#include <QWidget>
#include <vector>

class Document;
class QComboBox;
class ToolbarButton;

class AnimationManageWidget : public QWidget {
    Q_OBJECT

public:
    explicit AnimationManageWidget(Document* document, QWidget* parent = nullptr);
    ~AnimationManageWidget();
    void setWireframeVisible(bool visible);

public slots:
    void onResultRigChanged();
    void onAnimationPreviewReady();
    void onAnimationFrameTimeout();
    void onSelectedBoneChanged(const QString& boneName);

private slots:
    void onDeleteAnimationClicked();
    void onDuplicateAnimationClicked();
    void onAnimationListSelectionChanged();
    void onParameterChanged();
    void onAnimationNameEdited(const QString& name);
    void onAddAnimationClicked();
    void onPlayPauseClicked();

private:
    void startAnimationLoop();
    void stopAnimationLoop();
    void displayCurrentFrame();
    void triggerPreviewRegeneration();
    void createParameterWidgets();
    void autoSaveCurrentAnimation();
    void updateVisibleParameters(const QString& animationType);
    void setParameterRowVisible(QWidget* rowWidget, QLabel* rowLabel, bool visible);
    void updatePlayPauseIcon();

    Document* m_document = nullptr;
    WorldWidget* m_modelWidget = nullptr;
    QTimer* m_frameTimer = nullptr;
    QSlider* m_sharedStepLengthSlider = nullptr;
    QSlider* m_sharedStepHeightSlider = nullptr;
    QSlider* m_sharedBodyBobSlider = nullptr;
    QSlider* m_sharedGaitSpeedSlider = nullptr;
    QSlider* m_insectRubHandsRubForwardOffsetSlider = nullptr;
    QSlider* m_insectRubHandsRubUpOffsetSlider = nullptr;

    // Insect die simulation parameter sliders
    QSlider* m_sharedDieLengthStiffnessSlider = nullptr;
    QSlider* m_sharedDieParentStiffnessSlider = nullptr;
    QSlider* m_sharedDieMaxJointAngleSlider = nullptr;
    QSlider* m_sharedDieDampingSlider = nullptr;
    QSlider* m_sharedDieGroundBounceSlider = nullptr;

    // Fish die parameter sliders
    QSlider* m_fishDieHitIntensitySlider = nullptr;
    QSlider* m_fishDieHitFrequencySlider = nullptr;
    QSlider* m_fishDieFlipSpeedSlider = nullptr;
    QSlider* m_fishDieFlipAngleSlider = nullptr;
    QSlider* m_fishDieTiltSlider = nullptr;
    QSlider* m_fishDieFinFlopSlider = nullptr;
    QSlider* m_fishDieSpinDecaySlider = nullptr;

    // Fish animation parameter sliders
    QSlider* m_fishForwardSwimSpeedSlider = nullptr;
    QSlider* m_fishForwardSwimFrequencySlider = nullptr;
    QSlider* m_fishForwardSpineAmplitudeSlider = nullptr;
    QSlider* m_fishForwardWaveLengthSlider = nullptr;
    QSlider* m_fishForwardTailAmplitudeRatioSlider = nullptr;
    QSlider* m_fishForwardBodyRollSlider = nullptr;
    QSlider* m_fishForwardForwardThrustSlider = nullptr;
    QSlider* m_fishForwardPectoralFlapPowerSlider = nullptr;
    QSlider* m_fishForwardPelvicFlapPowerSlider = nullptr;
    QSlider* m_fishForwardDorsalSwayPowerSlider = nullptr;
    QSlider* m_fishForwardVentralSwayPowerSlider = nullptr;
    QSlider* m_fishForwardPectoralPhaseOffsetSlider = nullptr;
    QSlider* m_fishForwardPelvicPhaseOffsetSlider = nullptr;
    QSlider* m_snakeForwardWaveSpeedSlider = nullptr;
    QSlider* m_snakeForwardWaveFrequencySlider = nullptr;
    QSlider* m_snakeForwardWaveAmplitudeSlider = nullptr;
    QSlider* m_snakeForwardWaveLengthSlider = nullptr;
    QSlider* m_snakeForwardTailAmplitudeRatioSlider = nullptr;
    QSlider* m_snakeForwardHeadYawFactorSlider = nullptr;
    QSlider* m_snakeForwardHeadPullFactorSlider = nullptr;
    QSlider* m_snakeForwardJawAmplitudeSlider = nullptr;
    QSlider* m_snakeForwardJawFrequencySlider = nullptr;

    QFormLayout* m_parameterLayout = nullptr;
    QWidget* m_sharedStepLengthRow = nullptr;
    QLabel* m_sharedStepLengthLabel = nullptr;
    QWidget* m_sharedStepHeightRow = nullptr;
    QLabel* m_sharedStepHeightLabel = nullptr;
    QWidget* m_sharedBodyBobRow = nullptr;
    QLabel* m_sharedBodyBobLabel = nullptr;
    QWidget* m_sharedGaitSpeedRow = nullptr;
    QLabel* m_sharedGaitSpeedLabel = nullptr;
    QWidget* m_insectRubHandsRubForwardOffsetRow = nullptr;
    QLabel* m_insectRubHandsRubForwardOffsetLabel = nullptr;
    QWidget* m_insectRubHandsRubUpOffsetRow = nullptr;
    QLabel* m_insectRubHandsRubUpOffsetLabel = nullptr;

    // Animation preview frame scrubber
    QSlider* m_animationFrameSlider = nullptr;
    QPushButton* m_playPauseButton = nullptr;
    bool m_isScrubbing = false;

    // Insect die parameter rows and labels
    QWidget* m_sharedDieLengthStiffnessRow = nullptr;
    QLabel* m_sharedDieLengthStiffnessLabel = nullptr;
    QWidget* m_sharedDieParentStiffnessRow = nullptr;
    QLabel* m_sharedDieParentStiffnessLabel = nullptr;
    QWidget* m_sharedDieMaxJointAngleRow = nullptr;
    QLabel* m_sharedDieMaxJointAngleLabel = nullptr;
    QWidget* m_sharedDieDampingRow = nullptr;
    QLabel* m_sharedDieDampingLabel = nullptr;
    QWidget* m_sharedDieGroundBounceRow = nullptr;
    QLabel* m_sharedDieGroundBounceLabel = nullptr;

    // Fish die parameter rows and labels
    QWidget* m_fishDieHitIntensityRow = nullptr;
    QLabel* m_fishDieHitIntensityLabel = nullptr;
    QWidget* m_fishDieHitFrequencyRow = nullptr;
    QLabel* m_fishDieHitFrequencyLabel = nullptr;
    QWidget* m_fishDieFlipSpeedRow = nullptr;
    QLabel* m_fishDieFlipSpeedLabel = nullptr;
    QWidget* m_fishDieFlipAngleRow = nullptr;
    QLabel* m_fishDieFlipAngleLabel = nullptr;
    QWidget* m_fishDieTiltRow = nullptr;
    QLabel* m_fishDieTiltLabel = nullptr;
    QWidget* m_fishDieFinFlopRow = nullptr;
    QLabel* m_fishDieFinFlopLabel = nullptr;
    QWidget* m_fishDieSpinDecayRow = nullptr;
    QLabel* m_fishDieSpinDecayLabel = nullptr;

    // Fish animation parameter rows and labels
    QWidget* m_fishForwardSwimSpeedRow = nullptr;
    QLabel* m_fishForwardSwimSpeedLabel = nullptr;
    QWidget* m_fishForwardSwimFrequencyRow = nullptr;
    QLabel* m_fishForwardSwimFrequencyLabel = nullptr;
    QWidget* m_fishForwardSpineAmplitudeRow = nullptr;
    QLabel* m_fishForwardSpineAmplitudeLabel = nullptr;
    QWidget* m_fishForwardWaveLengthRow = nullptr;
    QLabel* m_fishForwardWaveLengthLabel = nullptr;
    QWidget* m_fishForwardTailAmplitudeRatioRow = nullptr;
    QLabel* m_fishForwardTailAmplitudeRatioLabel = nullptr;
    QWidget* m_fishForwardBodyRollRow = nullptr;
    QLabel* m_fishForwardBodyRollLabel = nullptr;
    QWidget* m_fishForwardForwardThrustRow = nullptr;
    QLabel* m_fishForwardForwardThrustLabel = nullptr;
    QWidget* m_fishForwardPectoralFlapPowerRow = nullptr;
    QLabel* m_fishForwardPectoralFlapPowerLabel = nullptr;
    QWidget* m_fishForwardPelvicFlapPowerRow = nullptr;
    QLabel* m_fishForwardPelvicFlapPowerLabel = nullptr;
    QWidget* m_fishForwardDorsalSwayPowerRow = nullptr;
    QLabel* m_fishForwardDorsalSwayPowerLabel = nullptr;
    QWidget* m_fishForwardVentralSwayPowerRow = nullptr;
    QLabel* m_fishForwardVentralSwayPowerLabel = nullptr;
    QWidget* m_fishForwardPectoralPhaseOffsetRow = nullptr;
    QLabel* m_fishForwardPectoralPhaseOffsetLabel = nullptr;
    QWidget* m_fishForwardPelvicPhaseOffsetRow = nullptr;
    QLabel* m_fishForwardPelvicPhaseOffsetLabel = nullptr;

    // Snake animation parameter rows and labels
    QWidget* m_snakeForwardWaveSpeedRow = nullptr;
    QLabel* m_snakeForwardWaveSpeedLabel = nullptr;
    QWidget* m_snakeForwardWaveFrequencyRow = nullptr;
    QLabel* m_snakeForwardWaveFrequencyLabel = nullptr;
    QWidget* m_snakeForwardWaveAmplitudeRow = nullptr;
    QLabel* m_snakeForwardWaveAmplitudeLabel = nullptr;
    QWidget* m_snakeForwardWaveLengthRow = nullptr;
    QLabel* m_snakeForwardWaveLengthLabel = nullptr;
    QWidget* m_snakeForwardTailAmplitudeRatioRow = nullptr;
    QLabel* m_snakeForwardTailAmplitudeRatioLabel = nullptr;
    QWidget* m_snakeForwardHeadYawFactorRow = nullptr;
    QLabel* m_snakeForwardHeadYawFactorLabel = nullptr;
    QWidget* m_snakeForwardHeadPullFactorRow = nullptr;
    QLabel* m_snakeForwardHeadPullFactorLabel = nullptr;
    QWidget* m_snakeForwardJawAmplitudeRow = nullptr;
    QLabel* m_snakeForwardJawAmplitudeLabel = nullptr;
    QWidget* m_snakeForwardJawFrequencyRow = nullptr;
    QLabel* m_snakeForwardJawFrequencyLabel = nullptr;

    // Biped walk parameter sliders
    QSlider* m_sharedArmSwingSlider = nullptr;
    QSlider* m_sharedHipSwaySlider = nullptr;
    QSlider* m_sharedHipRotateSlider = nullptr;
    QSlider* m_sharedBipedSpineFlexSlider = nullptr;
    QSlider* m_sharedHeadBobSlider = nullptr;
    QSlider* m_sharedKneeBendSlider = nullptr;
    QSlider* m_sharedLeanForwardSlider = nullptr;
    QSlider* m_sharedBouncinessSlider = nullptr;
    QSlider* m_sharedForearmPhaseOffsetSlider = nullptr;

    // Biped walk parameter rows and labels
    QWidget* m_sharedArmSwingRow = nullptr;
    QLabel* m_sharedArmSwingLabel = nullptr;
    QWidget* m_sharedHipSwayRow = nullptr;
    QLabel* m_sharedHipSwayLabel = nullptr;
    QWidget* m_sharedHipRotateRow = nullptr;
    QLabel* m_sharedHipRotateLabel = nullptr;
    QWidget* m_sharedBipedSpineFlexRow = nullptr;
    QLabel* m_sharedBipedSpineFlexLabel = nullptr;
    QWidget* m_sharedHeadBobRow = nullptr;
    QLabel* m_sharedHeadBobLabel = nullptr;
    QWidget* m_sharedKneeBendRow = nullptr;
    QLabel* m_sharedKneeBendLabel = nullptr;
    QWidget* m_sharedLeanForwardRow = nullptr;
    QLabel* m_sharedLeanForwardLabel = nullptr;
    QWidget* m_sharedBouncinessRow = nullptr;
    QLabel* m_sharedBouncinessLabel = nullptr;
    QWidget* m_sharedForearmPhaseOffsetRow = nullptr;
    QLabel* m_sharedForearmPhaseOffsetLabel = nullptr;

    // Quadruped walk parameter sliders
    QSlider* m_sharedSpineFlexSlider = nullptr;
    QSlider* m_sharedTailSwaySlider = nullptr;

    // Quadruped walk parameter rows and labels
    QWidget* m_sharedSpineFlexRow = nullptr;
    QLabel* m_sharedSpineFlexLabel = nullptr;
    QWidget* m_sharedTailSwayRow = nullptr;
    QLabel* m_sharedTailSwayLabel = nullptr;

    // Quadruped run parameter sliders
    QSlider* m_sharedSuspensionSlider = nullptr;
    QSlider* m_quadrupedRunForwardLeanSlider = nullptr;
    QSlider* m_sharedStrideFrequencySlider = nullptr;
    QSlider* m_quadrupedRunBoundSlider = nullptr;

    // Quadruped run parameter rows and labels
    QWidget* m_sharedSuspensionRow = nullptr;
    QLabel* m_sharedSuspensionLabel = nullptr;
    QWidget* m_quadrupedRunForwardLeanRow = nullptr;
    QLabel* m_quadrupedRunForwardLeanLabel = nullptr;
    QWidget* m_sharedStrideFrequencyRow = nullptr;
    QLabel* m_sharedStrideFrequencyLabel = nullptr;
    QWidget* m_quadrupedRunBoundRow = nullptr;
    QLabel* m_quadrupedRunBoundLabel = nullptr;

    // Quadruped die parameter sliders
    QSlider* m_quadDieCollapseSpeedSlider = nullptr;
    QSlider* m_quadDieLegSpreadSlider = nullptr;
    QSlider* m_quadDieRollIntensitySlider = nullptr;

    // Biped die parameter sliders
    QSlider* m_bipedDieCollapseSpeedSlider = nullptr;
    QSlider* m_bipedDieArmFlailSlider = nullptr;
    QSlider* m_bipedDieHeadDropSlider = nullptr;

    // Bird die parameter sliders
    QSlider* m_birdDieCollapseSpeedSlider = nullptr;
    QSlider* m_birdDieWingFlapSlider = nullptr;
    QSlider* m_birdDieRollIntensitySlider = nullptr;

    // Snake die parameter sliders
    QSlider* m_snakeDieCollapseSpeedSlider = nullptr;
    QSlider* m_snakeDieCoilFactorSlider = nullptr;
    QSlider* m_snakeDieJawOpenSlider = nullptr;

    // Spider die parameter sliders
    QSlider* m_spiderDieCollapseSpeedSlider = nullptr;
    QSlider* m_spiderDieLegSpreadSlider = nullptr;

    // Quadruped die parameter rows and labels
    QWidget* m_quadDieCollapseSpeedRow = nullptr;
    QLabel* m_quadDieCollapseSpeedLabel = nullptr;
    QWidget* m_quadDieLegSpreadRow = nullptr;
    QLabel* m_quadDieLegSpreadLabel = nullptr;
    QWidget* m_quadDieRollIntensityRow = nullptr;
    QLabel* m_quadDieRollIntensityLabel = nullptr;

    // Biped die parameter rows and labels
    QWidget* m_bipedDieCollapseSpeedRow = nullptr;
    QLabel* m_bipedDieCollapseSpeedLabel = nullptr;
    QWidget* m_bipedDieArmFlailRow = nullptr;
    QLabel* m_bipedDieArmFlailLabel = nullptr;
    QWidget* m_bipedDieHeadDropRow = nullptr;
    QLabel* m_bipedDieHeadDropLabel = nullptr;

    // Bird die parameter rows and labels
    QWidget* m_birdDieCollapseSpeedRow = nullptr;
    QLabel* m_birdDieCollapseSpeedLabel = nullptr;
    QWidget* m_birdDieWingFlapRow = nullptr;
    QLabel* m_birdDieWingFlapLabel = nullptr;
    QWidget* m_birdDieRollIntensityRow = nullptr;
    QLabel* m_birdDieRollIntensityLabel = nullptr;

    // Snake die parameter rows and labels
    QWidget* m_snakeDieCollapseSpeedRow = nullptr;
    QLabel* m_snakeDieCollapseSpeedLabel = nullptr;
    QWidget* m_snakeDieCoilFactorRow = nullptr;
    QLabel* m_snakeDieCoilFactorLabel = nullptr;
    QWidget* m_snakeDieJawOpenRow = nullptr;
    QLabel* m_snakeDieJawOpenLabel = nullptr;

    // Spider die parameter rows and labels
    QWidget* m_spiderDieCollapseSpeedRow = nullptr;
    QLabel* m_spiderDieCollapseSpeedLabel = nullptr;
    QWidget* m_spiderDieLegSpreadRow = nullptr;
    QLabel* m_spiderDieLegSpreadLabel = nullptr;

    // Spider walk parameter sliders
    QSlider* m_spiderLegSpreadSlider = nullptr;
    QSlider* m_spiderAbdomenSwaySlider = nullptr;
    QSlider* m_spiderPedipalpSwaySlider = nullptr;
    QSlider* m_spiderBodyYawSlider = nullptr;

    // Spider walk parameter rows and labels
    QWidget* m_spiderLegSpreadRow = nullptr;
    QLabel* m_spiderLegSpreadLabel = nullptr;
    QWidget* m_spiderAbdomenSwayRow = nullptr;
    QLabel* m_spiderAbdomenSwayLabel = nullptr;
    QWidget* m_spiderPedipalpSwayRow = nullptr;
    QLabel* m_spiderPedipalpSwayLabel = nullptr;
    QWidget* m_spiderBodyYawRow = nullptr;
    QLabel* m_spiderBodyYawLabel = nullptr;

    // Idle animation parameter sliders (shared across rig types)
    QSlider* m_idleBreathingAmplitudeSlider = nullptr;
    QSlider* m_idleBreathingSpeedSlider = nullptr;
    QSlider* m_idleWeightShiftSlider = nullptr;
    QSlider* m_idleHeadLookSlider = nullptr;
    QSlider* m_idleSpineSwaySlider = nullptr;
    QSlider* m_idleTailIdleSlider = nullptr;

    // Idle parameter rows and labels
    QWidget* m_idleBreathingAmplitudeRow = nullptr;
    QLabel* m_idleBreathingAmplitudeLabel = nullptr;
    QWidget* m_idleBreathingSpeedRow = nullptr;
    QLabel* m_idleBreathingSpeedLabel = nullptr;
    QWidget* m_idleWeightShiftRow = nullptr;
    QLabel* m_idleWeightShiftLabel = nullptr;
    QWidget* m_idleHeadLookRow = nullptr;
    QLabel* m_idleHeadLookLabel = nullptr;
    QWidget* m_idleSpineSwayRow = nullptr;
    QLabel* m_idleSpineSwayLabel = nullptr;
    QWidget* m_idleTailIdleRow = nullptr;
    QLabel* m_idleTailIdleLabel = nullptr;

    // Biped idle specific
    QSlider* m_bipedIdleArmRestSlider = nullptr;
    QWidget* m_bipedIdleArmRestRow = nullptr;
    QLabel* m_bipedIdleArmRestLabel = nullptr;

    // Quadruped idle specific
    QSlider* m_quadIdleJawSlider = nullptr;
    QWidget* m_quadIdleJawRow = nullptr;
    QLabel* m_quadIdleJawLabel = nullptr;

    // Insect idle specific
    QSlider* m_insectIdleAntennaeSwaySlider = nullptr;
    QSlider* m_insectIdleLegTwitchSlider = nullptr;
    QSlider* m_insectIdleWingFoldSlider = nullptr;
    QSlider* m_insectIdleAbdomenSwaySlider = nullptr;
    QWidget* m_insectIdleAntennaeSwayRow = nullptr;
    QLabel* m_insectIdleAntennaeSwayLabel = nullptr;
    QWidget* m_insectIdleLegTwitchRow = nullptr;
    QLabel* m_insectIdleLegTwitchLabel = nullptr;
    QWidget* m_insectIdleWingFoldRow = nullptr;
    QLabel* m_insectIdleWingFoldLabel = nullptr;
    QWidget* m_insectIdleAbdomenSwayRow = nullptr;
    QLabel* m_insectIdleAbdomenSwayLabel = nullptr;

    // Spider idle specific
    QSlider* m_spiderIdlePedipalpSwaySlider = nullptr;
    QSlider* m_spiderIdleLegTwitchSlider = nullptr;
    QSlider* m_spiderIdleAbdomenPulseSlider = nullptr;
    QSlider* m_spiderIdleBodySwaySlider = nullptr;
    QWidget* m_spiderIdlePedipalpSwayRow = nullptr;
    QLabel* m_spiderIdlePedipalpSwayLabel = nullptr;
    QWidget* m_spiderIdleLegTwitchRow = nullptr;
    QLabel* m_spiderIdleLegTwitchLabel = nullptr;
    QWidget* m_spiderIdleAbdomenPulseRow = nullptr;
    QLabel* m_spiderIdleAbdomenPulseLabel = nullptr;
    QWidget* m_spiderIdleBodySwayRow = nullptr;
    QLabel* m_spiderIdleBodySwayLabel = nullptr;

    // Bird idle specific
    QSlider* m_birdIdleHeadPeckSlider = nullptr;
    QSlider* m_birdIdleWingFoldSlider = nullptr;
    QSlider* m_birdIdleTailFeatherSlider = nullptr;
    QWidget* m_birdIdleHeadPeckRow = nullptr;
    QLabel* m_birdIdleHeadPeckLabel = nullptr;
    QWidget* m_birdIdleWingFoldRow = nullptr;
    QLabel* m_birdIdleWingFoldLabel = nullptr;
    QWidget* m_birdIdleTailFeatherRow = nullptr;
    QLabel* m_birdIdleTailFeatherLabel = nullptr;

    // Fish idle specific
    QSlider* m_fishIdleFinScullSlider = nullptr;
    QSlider* m_fishIdleTailSwaySlider = nullptr;
    QSlider* m_fishIdleBodyUndulationSlider = nullptr;
    QSlider* m_fishIdleDorsalSwaySlider = nullptr;
    QSlider* m_fishIdleDriftSlider = nullptr;
    QWidget* m_fishIdleFinScullRow = nullptr;
    QLabel* m_fishIdleFinScullLabel = nullptr;
    QWidget* m_fishIdleTailSwayRow = nullptr;
    QLabel* m_fishIdleTailSwayLabel = nullptr;
    QWidget* m_fishIdleBodyUndulationRow = nullptr;
    QLabel* m_fishIdleBodyUndulationLabel = nullptr;
    QWidget* m_fishIdleDorsalSwayRow = nullptr;
    QLabel* m_fishIdleDorsalSwayLabel = nullptr;
    QWidget* m_fishIdleDriftRow = nullptr;
    QLabel* m_fishIdleDriftLabel = nullptr;

    // Snake idle specific
    QSlider* m_snakeIdleHeadSwaySlider = nullptr;
    QSlider* m_snakeIdleHeadRaiseSlider = nullptr;
    QSlider* m_snakeIdleTongueFlickSlider = nullptr;
    QSlider* m_snakeIdleBodyUndulationSlider = nullptr;
    QSlider* m_snakeIdleTailTwitchSlider = nullptr;
    QWidget* m_snakeIdleHeadSwayRow = nullptr;
    QLabel* m_snakeIdleHeadSwayLabel = nullptr;
    QWidget* m_snakeIdleHeadRaiseRow = nullptr;
    QLabel* m_snakeIdleHeadRaiseLabel = nullptr;
    QWidget* m_snakeIdleTongueFlickRow = nullptr;
    QLabel* m_snakeIdleTongueFlickLabel = nullptr;
    QWidget* m_snakeIdleBodyUndulationRow = nullptr;
    QLabel* m_snakeIdleBodyUndulationLabel = nullptr;
    QWidget* m_snakeIdleTailTwitchRow = nullptr;
    QLabel* m_snakeIdleTailTwitchLabel = nullptr;

    QCheckBox* m_sharedUseFabrikCheck = nullptr;
    QCheckBox* m_sharedPlaneStabilizationCheck = nullptr;
    QCheckBox* m_hideBonesCheck = nullptr;
    QCheckBox* m_hidePartsCheck = nullptr;
    QCheckBox* m_hideWeightsCheck = nullptr;

    QDoubleSpinBox* m_sharedDurationSpinBox = nullptr;
    QSpinBox* m_sharedFrameCountSpinBox = nullptr;

    QComboBox* m_animationNameCombo = nullptr;
    ToolbarButton* m_addAnimationButton = nullptr;

    QLineEdit* m_animationNameInput = nullptr;
    QLineEdit* m_animationTypeInput = nullptr;
    QListWidget* m_animationListWidget = nullptr;
    QPushButton* m_deleteAnimationButton = nullptr;
    QPushButton* m_duplicateAnimationButton = nullptr;
    QGroupBox* m_parametersGroupBox = nullptr;

    void refreshAnimationList();
    void loadAnimationIntoForm(const dust3d::Uuid& animationId);

    void updateAnimationParamsFromWidgets();
    void updateAnimationNameForRigType(const QString& rigType);
    void onRigTypeChanged(const QString& rigType);

    std::unique_ptr<AnimationPreviewWorker> m_animationWorker;
    bool m_animationWorkerBusy = false;
    bool m_animationRegenerationPending = false;
    std::vector<ModelMesh> m_animationFrames;
    int m_currentFrame = 0;
    dust3d::AnimationParams m_animationParams;
    dust3d::Uuid m_currentAnimationId;
    bool m_isUpdatingForm = false;
    QString m_selectedBoneName;
};

#endif
