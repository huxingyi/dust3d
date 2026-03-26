#ifndef DUST3D_APPLICATION_ANIMATION_MANAGE_WIDGET_H_
#define DUST3D_APPLICATION_ANIMATION_MANAGE_WIDGET_H_

#include "animation_preview_worker.h"
#include "model_mesh.h"
#include "model_widget.h"
#include <QWidget>
#include <QTimer>
#include <QThread>
#include <QSlider>
#include <QLabel>
#include <QCheckBox>
#include <QDoubleSpinBox>
#include <vector>

class Document;

class AnimationManageWidget : public QWidget {
    Q_OBJECT

public:
    explicit AnimationManageWidget(Document* document, QWidget* parent = nullptr);
    ~AnimationManageWidget();

public slots:
    void onResultRigChanged();
    void onAnimationPreviewReady();
    void onAnimationFrameTimeout();

private:
    void startAnimationLoop();
    void stopAnimationLoop();
    void triggerPreviewRegeneration();
    void createParameterWidgets();

    Document* m_document = nullptr;
    ModelWidget* m_modelWidget = nullptr;
    QTimer* m_frameTimer = nullptr;
    QSlider* m_stepLengthSlider = nullptr;
    QSlider* m_stepHeightSlider = nullptr;
    QSlider* m_bodyBobSlider = nullptr;
    QSlider* m_gaitSpeedSlider = nullptr;
    QCheckBox* m_useFabrikCheck = nullptr;
    QCheckBox* m_planeStabilizationCheck = nullptr;

    std::unique_ptr<AnimationPreviewWorker> m_animationWorker;
    bool m_animationWorkerBusy = false;
    bool m_animationRegenerationPending = false;
    std::vector<ModelMesh> m_animationFrames;
    int m_currentFrame = 0;
    dust3d::AnimationParams m_animationParams;
};

#endif
