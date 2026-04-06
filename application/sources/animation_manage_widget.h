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

    // Quadruped die parameter rows and labels
    QWidget* m_quadDieCollapseSpeedRow = nullptr;
    QLabel* m_quadDieCollapseSpeedLabel = nullptr;
    QWidget* m_quadDieLegSpreadRow = nullptr;
    QLabel* m_quadDieLegSpreadLabel = nullptr;
    QWidget* m_quadDieRollIntensityRow = nullptr;
    QLabel* m_quadDieRollIntensityLabel = nullptr;

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
