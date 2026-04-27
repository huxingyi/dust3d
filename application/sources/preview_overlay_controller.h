#ifndef DUST3D_APPLICATION_PREVIEW_OVERLAY_CONTROLLER_H_
#define DUST3D_APPLICATION_PREVIEW_OVERLAY_CONTROLLER_H_

#include <QObject>
#include <dust3d/base/snapshot.h>
#include <memory>
#include <vector>

class Document;
class ModelWidget;
class ModelMesh;
class AnimationPreviewWorker;
class QTimer;

class PreviewOverlayController : public QObject {
    Q_OBJECT
public:
    explicit PreviewOverlayController(ModelWidget* modelWidget, QObject* parent = nullptr);
    ~PreviewOverlayController() override;

    void start();
    void stop();

public slots:
    void onPreviewDocumentLoaded(dust3d::Snapshot snapshot);

private slots:
    void onPreviewDocumentRigReady();
    void onPreviewAnimationReady();
    void onPreviewFrameTimeout();

private:
    void displayPreviewFrame();

    ModelWidget* m_modelWidget = nullptr;
    Document* m_previewDocument = nullptr;
    std::unique_ptr<AnimationPreviewWorker> m_previewAnimationWorker;
    std::unique_ptr<ModelMesh> m_previewFallbackMesh;
    QTimer* m_previewFrameTimer = nullptr;
    std::vector<ModelMesh> m_previewAnimationFrames;
    double m_previewAnimationDurationSeconds = 0.0;
    int m_previewCurrentFrame = 0;
    float m_previewMovementSpeed = 0.0f;
    float m_previewMovementDirectionX = 0.0f;
    float m_previewMovementDirectionZ = 0.0f;
    float m_previewGroundOffsetX = 0.0f;
    float m_previewGroundOffsetZ = 0.0f;
    bool m_previewWorkerBusy = false;
    bool m_previewDocumentLoading = false;
    bool m_previewActive = false;
};

#endif
