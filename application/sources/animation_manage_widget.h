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
    QSlider* m_stepLengthSlider = nullptr;
    QSlider* m_stepHeightSlider = nullptr;
    QSlider* m_bodyBobSlider = nullptr;
    QSlider* m_gaitSpeedSlider = nullptr;
    QSlider* m_rubForwardOffsetSlider = nullptr;
    QSlider* m_rubUpOffsetSlider = nullptr;

    // Insect die simulation parameter sliders
    QSlider* m_dieLengthStiffnessSlider = nullptr;
    QSlider* m_dieParentStiffnessSlider = nullptr;
    QSlider* m_dieMaxJointAngleSlider = nullptr;
    QSlider* m_dieDampingSlider = nullptr;
    QSlider* m_dieGroundBounceSlider = nullptr;

    // Fish animation parameter sliders
    QSlider* m_swimSpeedSlider = nullptr;
    QSlider* m_swimFrequencySlider = nullptr;
    QSlider* m_spineAmplitudeSlider = nullptr;
    QSlider* m_waveLengthSlider = nullptr;
    QSlider* m_tailAmplitudeRatioSlider = nullptr;
    QSlider* m_bodyRollSlider = nullptr;
    QSlider* m_forwardThrustSlider = nullptr;
    QSlider* m_pectoralFlapPowerSlider = nullptr;
    QSlider* m_pelvicFlapPowerSlider = nullptr;
    QSlider* m_dorsalSwayPowerSlider = nullptr;
    QSlider* m_ventralSwayPowerSlider = nullptr;
    QSlider* m_pectoralPhaseOffsetSlider = nullptr;
    QSlider* m_pelvicPhaseOffsetSlider = nullptr;

    QFormLayout* m_parameterLayout = nullptr;
    QWidget* m_stepLengthRow = nullptr;
    QLabel* m_stepLengthLabel = nullptr;
    QWidget* m_stepHeightRow = nullptr;
    QLabel* m_stepHeightLabel = nullptr;
    QWidget* m_bodyBobRow = nullptr;
    QLabel* m_bodyBobLabel = nullptr;
    QWidget* m_gaitSpeedRow = nullptr;
    QLabel* m_gaitSpeedLabel = nullptr;
    QWidget* m_rubForwardOffsetRow = nullptr;
    QLabel* m_rubForwardOffsetLabel = nullptr;
    QWidget* m_rubUpOffsetRow = nullptr;
    QLabel* m_rubUpOffsetLabel = nullptr;

    // Animation preview frame scrubber
    QSlider* m_animationFrameSlider = nullptr;
    QPushButton* m_playPauseButton = nullptr;
    bool m_isScrubbing = false;

    // Insect die parameter rows and labels
    QWidget* m_dieLengthStiffnessRow = nullptr;
    QLabel* m_dieLengthStiffnessLabel = nullptr;
    QWidget* m_dieParentStiffnessRow = nullptr;
    QLabel* m_dieParentStiffnessLabel = nullptr;
    QWidget* m_dieMaxJointAngleRow = nullptr;
    QLabel* m_dieMaxJointAngleLabel = nullptr;
    QWidget* m_dieDampingRow = nullptr;
    QLabel* m_dieDampingLabel = nullptr;
    QWidget* m_dieGroundBounceRow = nullptr;
    QLabel* m_dieGroundBounceLabel = nullptr;

    // Fish animation parameter rows and labels
    QWidget* m_swimSpeedRow = nullptr;
    QLabel* m_swimSpeedLabel = nullptr;
    QWidget* m_swimFrequencyRow = nullptr;
    QLabel* m_swimFrequencyLabel = nullptr;
    QWidget* m_spineAmplitudeRow = nullptr;
    QLabel* m_spineAmplitudeLabel = nullptr;
    QWidget* m_waveLengthRow = nullptr;
    QLabel* m_waveLengthLabel = nullptr;
    QWidget* m_tailAmplitudeRatioRow = nullptr;
    QLabel* m_tailAmplitudeRatioLabel = nullptr;
    QWidget* m_bodyRollRow = nullptr;
    QLabel* m_bodyRollLabel = nullptr;
    QWidget* m_forwardThrustRow = nullptr;
    QLabel* m_forwardThrustLabel = nullptr;
    QWidget* m_pectoralFlapPowerRow = nullptr;
    QLabel* m_pectoralFlapPowerLabel = nullptr;
    QWidget* m_pelvicFlapPowerRow = nullptr;
    QLabel* m_pelvicFlapPowerLabel = nullptr;
    QWidget* m_dorsalSwayPowerRow = nullptr;
    QLabel* m_dorsalSwayPowerLabel = nullptr;
    QWidget* m_ventralSwayPowerRow = nullptr;
    QLabel* m_ventralSwayPowerLabel = nullptr;
    QWidget* m_pectoralPhaseOffsetRow = nullptr;
    QLabel* m_pectoralPhaseOffsetLabel = nullptr;
    QWidget* m_pelvicPhaseOffsetRow = nullptr;
    QLabel* m_pelvicPhaseOffsetLabel = nullptr;
    QCheckBox* m_useFabrikCheck = nullptr;
    QCheckBox* m_planeStabilizationCheck = nullptr;
    QCheckBox* m_hideBonesCheck = nullptr;
    QCheckBox* m_hidePartsCheck = nullptr;
    QCheckBox* m_hideWeightsCheck = nullptr;

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
