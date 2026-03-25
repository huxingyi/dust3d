#ifndef DUST3D_APPLICATION_ANIMATION_MANAGE_WIDGET_H_
#define DUST3D_APPLICATION_ANIMATION_MANAGE_WIDGET_H_

#include "animation_preview_worker.h"
#include "model_mesh.h"
#include "model_widget.h"
#include <QWidget>
#include <QTimer>
#include <QThread>
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

    Document* m_document = nullptr;
    ModelWidget* m_modelWidget = nullptr;
    QTimer* m_frameTimer = nullptr;
    std::unique_ptr<AnimationPreviewWorker> m_animationWorker;
    std::vector<ModelMesh> m_animationFrames;
    int m_currentFrame = 0;
};

#endif
