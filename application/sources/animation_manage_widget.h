#ifndef DUST3D_APPLICATION_ANIMATION_MANAGE_WIDGET_H_
#define DUST3D_APPLICATION_ANIMATION_MANAGE_WIDGET_H_

#include "animation_preview_worker.h"
#include "model_mesh.h"
#include "world_widget.h"
#include <QBuffer>
#include <QCheckBox>
#include <QComboBox>
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
#include <dust3d/animation/sound_generator.h>
#include <vector>

class Document;
class QComboBox;
class ToolbarButton;

struct DynamicParameterControl {
    std::string paramName;
    QSlider* slider = nullptr;
    QWidget* rowWidget = nullptr;
    QLabel* rowLabel = nullptr;
    QLabel* valueLabel = nullptr;
    std::function<double(int)> toParam;
    std::function<int(double)> fromParam;
    double defaultParamValue = 1.0;
};

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
    void rebuildDynamicControls(const QString& animationType);
    void updatePlayPauseIcon();
    void updateParametersGroupBoxTitle();

    Document* m_document = nullptr;
    WorldWidget* m_modelWidget = nullptr;
    QTimer* m_frameTimer = nullptr;

    QFormLayout* m_parameterLayout = nullptr;

    // Dynamic parameter controls created from animation_parameter_table.h
    std::vector<DynamicParameterControl> m_dynamicControls;

    // Animation preview frame scrubber
    QSlider* m_animationFrameSlider = nullptr;
    QPushButton* m_playPauseButton = nullptr;
    bool m_isScrubbing = false;

    QCheckBox* m_hideBonesCheck = nullptr;
    QCheckBox* m_hidePartsCheck = nullptr;
    QCheckBox* m_hideWeightsCheck = nullptr;

    // Sound controls
    QCheckBox* m_playSoundCheck = nullptr;
    QComboBox* m_surfaceMaterialCombo = nullptr;
    QPushButton* m_exportAudioButton = nullptr;

    QDoubleSpinBox* m_sharedDurationSpinBox = nullptr;
    QSpinBox* m_sharedFrameCountSpinBox = nullptr;

    QComboBox* m_animationNameCombo = nullptr;
    ToolbarButton* m_addAnimationButton = nullptr;

    QLineEdit* m_animationNameInput = nullptr;
    QString m_currentAnimationType;
    QListWidget* m_animationListWidget = nullptr;
    QPushButton* m_deleteAnimationButton = nullptr;
    QPushButton* m_duplicateAnimationButton = nullptr;
    QGroupBox* m_parametersGroupBox = nullptr;
    QWidget* m_bottomStretch = nullptr;

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

    // Sound playback
    dust3d::AnimationSoundData m_soundData;
    float m_movementSpeed = 0.0f;
    float m_movementDirectionX = 0.0f;
    float m_movementDirectionZ = 0.0f;
    float m_groundOffsetX = 0.0f;
    float m_groundOffsetZ = 0.0f;
    QByteArray m_soundWavData;
    QBuffer* m_soundBuffer = nullptr;
    QObject* m_audioOutput = nullptr;
    void startSoundPlayback();
    void stopSoundPlayback();
    void onExportAudioClicked();
};

#endif
