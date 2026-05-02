#ifndef DUST3D_APPLICATION_PREVIEW_OVERLAY_CONTROLLER_H_
#define DUST3D_APPLICATION_PREVIEW_OVERLAY_CONTROLLER_H_

#include "scene_widget.h"
#include <QObject>
#include <QString>
#include <array>
#include <dust3d/animation/animation_generator.h>
#include <dust3d/base/snapshot.h>
#include <memory>
#include <vector>

class Document;
class SceneWidget;
class ModelMesh;
class AnimationPreviewWorker;
class QTimer;

class PreviewOverlayController : public QObject {
    Q_OBJECT
public:
    explicit PreviewOverlayController(SceneWidget* modelWidget, const QString& previewResourcePath, int previewIndex, QObject* parent = nullptr);
    ~PreviewOverlayController() override;

    void start();
    void stop();

public slots:
    void onPreviewDocumentLoaded(dust3d::Snapshot snapshot);

private slots:
    void onPreviewDocumentRigReady();
    void onPreviewAnimationReady(int setIndex);
    void onPreviewFrameTimeout();

private:
    void displayPreviewFrame();
    void generateAnimationSet(const QString& animationType, int setIndex, const dust3d::AnimationParams& params = dust3d::AnimationParams());
    void startNextAnimationSet();
    dust3d::AnimationParams findAnimationParametersByState(const QString& stateName) const;
    void initializeWanderState();
    void scheduleNextTarget();
    void updateWanderMovement(float dt);
    void translateMesh(ModelMesh& mesh, float offsetX, float offsetZ);
    const std::vector<ModelMesh>* currentAnimationFrames() const;
    int currentFrameIntervalMs() const;

    enum class State {
        Idle = 0,
        Walk,
        Run,
        Eat,
        Hover,
        Glide,
        Count
    };

    QString animationStateName(State state) const;
    State movingState() const;
    State slowMovingState() const;
    State hoverState() const;

    struct AnimationSet {
        std::vector<ModelMesh> frames;
        double durationSeconds = 0.0;
        int frameIntervalMs = 100;
        float movementSpeed = 0.0f;
    };

    SceneWidget* m_modelWidget = nullptr;
    QString m_previewResourcePath;
    int m_previewIndex = 0;
    float m_baseOffsetX = 0.0f;
    float m_baseOffsetZ = 0.0f;
    Document* m_previewDocument = nullptr;
    std::unique_ptr<AnimationPreviewWorker> m_previewAnimationWorker;
    std::unique_ptr<ModelMesh> m_previewFallbackMesh;
    QTimer* m_previewFrameTimer = nullptr;
    std::array<AnimationSet, static_cast<int>(State::Count)> m_animationSets;
    std::vector<State> m_stateOrder;
    bool m_hasRunAnimation = false;
    bool m_hasEatAnimation = false;
    bool m_hasFlyAnimation = false;
    bool m_hasGlideAnimation = false;
    bool m_isFlightModel = false;
    State m_state = State::Idle;
    int m_previewCurrentFrame = 0;
    float m_posX = 0.0f;
    float m_posY = 0.0f;
    float m_posZ = 0.0f;
    float m_targetX = 0.0f;
    float m_targetZ = 0.0f;
    float m_escapeDirectionX = 0.0f;
    float m_escapeDirectionZ = 0.0f;
    float m_yaw = 0.0f;
    float m_idleTimer = 0.0f;
    bool m_initialEscapeDone = false;
    float m_cycleAngle = 0.0f;
    std::vector<SceneWidget::NameCube> m_cuboidColliders;
    int m_animationGenerationIndex = 0;
    bool m_previewWorkerBusy = false;
    bool m_previewDocumentLoading = false;
    bool m_previewActive = false;
};

#endif
