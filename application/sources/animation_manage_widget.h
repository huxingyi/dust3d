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

public slots:
    void onResultRigChanged();
    void onAnimationPreviewReady();
    void onAnimationFrameTimeout();

private slots:
    void onDeleteAnimationClicked();
    void onDuplicateAnimationClicked();
    void onAnimationListSelectionChanged();
    void onParameterChanged();
    void onAnimationNameEdited(const QString& name);
    void onAddAnimationClicked();

private:
    void startAnimationLoop();
    void stopAnimationLoop();
    void triggerPreviewRegeneration();
    void createParameterWidgets();
    void autoSaveCurrentAnimation();
    void updateVisibleParameters(const QString& animationType);
    void setParameterRowVisible(QWidget* rowWidget, QLabel* rowLabel, bool visible);

    Document* m_document = nullptr;
    WorldWidget* m_modelWidget = nullptr;
    QTimer* m_frameTimer = nullptr;
    QSlider* m_stepLengthSlider = nullptr;
    QSlider* m_stepHeightSlider = nullptr;
    QSlider* m_bodyBobSlider = nullptr;
    QSlider* m_gaitSpeedSlider = nullptr;
    QSlider* m_rubForwardOffsetSlider = nullptr;
    QSlider* m_rubUpOffsetSlider = nullptr;
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
    QCheckBox* m_useFabrikCheck = nullptr;
    QCheckBox* m_planeStabilizationCheck = nullptr;
    QCheckBox* m_hideBonesCheck = nullptr;
    QCheckBox* m_hidePartsCheck = nullptr;

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
};

#endif
