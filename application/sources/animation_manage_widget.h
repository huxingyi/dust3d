#ifndef DUST3D_APPLICATION_ANIMATION_MANAGE_WIDGET_H_
#define DUST3D_APPLICATION_ANIMATION_MANAGE_WIDGET_H_

#include "animation_preview_worker.h"
#include "model_mesh.h"
#include "world_widget.h"
#include <QWidget>
#include <QTimer>
#include <QThread>
#include <QSlider>
#include <QLabel>
#include <QCheckBox>
#include <QDoubleSpinBox>
#include <QPushButton>
#include <QLineEdit>
#include <QListWidget>
#include <QGroupBox>
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

    Document* m_document = nullptr;
    WorldWidget* m_modelWidget = nullptr;
    QTimer* m_frameTimer = nullptr;
    QSlider* m_stepLengthSlider = nullptr;
    QSlider* m_stepHeightSlider = nullptr;
    QSlider* m_bodyBobSlider = nullptr;
    QSlider* m_gaitSpeedSlider = nullptr;
    QSlider* m_rubCenterOffsetSlider = nullptr;
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
