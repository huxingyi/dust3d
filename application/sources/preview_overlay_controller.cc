#include "preview_overlay_controller.h"
#include "animation_preview_worker.h"
#include "document.h"
#include "model_mesh.h"
#include "model_widget.h"
#include "theme.h"
#include <algorithm>
#include <QFile>
#include <QMetaObject>
#include <QPointer>
#include <QThreadPool>
#include <QTimer>
#include <QRunnable>
#include <dust3d/base/snapshot.h>
#include <dust3d/base/snapshot_xml.h>
#include <dust3d/rig/rig_generator.h>

namespace {

class PreviewDocumentLoader : public QRunnable {
public:
    explicit PreviewDocumentLoader(PreviewOverlayController* controller)
        : m_controller(controller)
    {
        setAutoDelete(true);
    }

    void run() override
    {
        QPointer<PreviewOverlayController> controller(m_controller);
        QFile file(":/resources/preview_quadruped_walk.xml");
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            qWarning() << "Unable to open preview model resource";
            if (controller) {
                QMetaObject::invokeMethod(controller, [controller]() {
                    controller->onPreviewDocumentLoaded(dust3d::Snapshot());
                }, Qt::QueuedConnection);
            }
            return;
        }

        QByteArray data = file.readAll();
        data.append('\0');
        dust3d::Snapshot snapshot;
        loadSnapshotFromXmlString(&snapshot, data.data());
        if (controller) {
            QMetaObject::invokeMethod(controller, [controller, snapshot = std::move(snapshot)]() mutable {
                if (controller)
                    controller->onPreviewDocumentLoaded(std::move(snapshot));
            }, Qt::QueuedConnection);
        }
    }

private:
    PreviewOverlayController* m_controller;
};

}

PreviewOverlayController::PreviewOverlayController(ModelWidget* modelWidget, QObject* parent)
    : QObject(parent)
    , m_modelWidget(modelWidget)
    , m_previewDocument(new Document)
{
    connect(m_previewDocument, &Document::resultRigChanged,
        this, &PreviewOverlayController::onPreviewDocumentRigReady);
}

PreviewOverlayController::~PreviewOverlayController()
{
    if (m_previewFrameTimer) {
        m_previewFrameTimer->stop();
        delete m_previewFrameTimer;
        m_previewFrameTimer = nullptr;
    }

    delete m_previewDocument;
    m_previewDocument = nullptr;
}

void PreviewOverlayController::start()
{
    if (!m_modelWidget)
        return;

    m_previewActive = true;

    if (m_previewWorkerBusy || m_previewDocumentLoading)
        return;

    if (!m_previewAnimationFrames.empty() || m_previewFallbackMesh) {
        if (!m_previewFrameTimer) {
            m_previewFrameTimer = new QTimer(this);
            connect(m_previewFrameTimer, &QTimer::timeout, this, &PreviewOverlayController::onPreviewFrameTimeout);
        }
        m_previewFrameTimer->start();
        return;
    }

    m_previewDocumentLoading = true;
    QThreadPool::globalInstance()->start(new PreviewDocumentLoader(this));
}

void PreviewOverlayController::stop()
{
    m_previewActive = false;

    if (m_previewFrameTimer)
        m_previewFrameTimer->stop();

    m_previewAnimationFrames.clear();
    m_previewFallbackMesh.reset();
    m_previewCurrentFrame = 0;
    m_previewAnimationDurationSeconds = 0.0;
    m_previewMovementSpeed = 0.0f;
    m_previewMovementDirectionX = 0.0f;
    m_previewMovementDirectionZ = 0.0f;
    m_previewGroundOffsetX = 0.0f;
    m_previewGroundOffsetZ = 0.0f;
}

void PreviewOverlayController::onPreviewDocumentLoaded(dust3d::Snapshot snapshot)
{
    m_previewDocumentLoading = false;
    if (!m_previewActive)
        return;

    m_previewDocument->fromSnapshot(snapshot);
    m_previewDocument->generateMesh();
}

void PreviewOverlayController::onPreviewDocumentRigReady()
{
    if (m_previewWorkerBusy)
        return;
    if (!m_previewDocument)
        return;
    if (!m_previewActive)
        return;

    const RigStructure& rigStructure = m_previewDocument->getActualRigStructure();
    if (rigStructure.bones.empty()) {
        qWarning() << "Preview document rig has no bones";
        return;
    }
    if (!m_previewDocument->currentRigObject()) {
        qWarning() << "Preview document rig object missing";
        return;
    }

    m_previewAnimationWorker = std::make_unique<AnimationPreviewWorker>();
    m_previewAnimationWorker->setParameters(rigStructure, "QuadrupedWalk");
    m_previewAnimationWorker->setRigObject(std::unique_ptr<dust3d::Object>(m_previewDocument->takeRigObject()));
    m_previewAnimationWorker->setHideBones(true);
    m_previewAnimationWorker->setHideParts(false);
    m_previewAnimationWorker->setSoundEnabled(false);

    QThread* thread = new QThread;
    m_previewAnimationWorker->moveToThread(thread);
    connect(thread, &QThread::started, m_previewAnimationWorker.get(), &AnimationPreviewWorker::process);
    connect(m_previewAnimationWorker.get(), &AnimationPreviewWorker::finished,
        this, &PreviewOverlayController::onPreviewAnimationReady);
    connect(m_previewAnimationWorker.get(), &AnimationPreviewWorker::finished,
        thread, &QThread::quit);
    connect(thread, &QThread::finished, thread, &QThread::deleteLater);
    thread->start();
    m_previewWorkerBusy = true;
}

void PreviewOverlayController::onPreviewAnimationReady()
{
    m_previewWorkerBusy = false;
    if (!m_previewAnimationWorker) {
        qWarning() << "Preview animation worker finished but is null";
        return;
    }

    if (!m_previewActive) {
        m_previewAnimationWorker.reset();
        return;
    }

    m_previewAnimationFrames = m_previewAnimationWorker->takePreviewMeshes();
    m_previewAnimationDurationSeconds = m_previewAnimationWorker->durationSeconds();
    m_previewMovementSpeed = m_previewAnimationWorker->movementSpeed();
    m_previewMovementDirectionX = m_previewAnimationWorker->movementDirectionX();
    m_previewMovementDirectionZ = m_previewAnimationWorker->movementDirectionZ();
    m_previewGroundOffsetX = 0.0f;
    m_previewGroundOffsetZ = 0.0f;
    m_previewAnimationWorker.reset();
    if (m_previewAnimationFrames.empty()) {
        qWarning() << "Preview animation produced no frames";
        return;
    }

    // If the generated preview frames contain only skeleton geometry, keep a fallback
    // static mesh from the current preview document so the overlay still shows body geometry.
    bool allSkeletonOnly = true;
    for (const auto& frame : m_previewAnimationFrames) {
        if (frame.skeletonVertexCount() < frame.triangleVertexCount()) {
            allSkeletonOnly = false;
            break;
        }
    }
    if (allSkeletonOnly && m_previewDocument) {
        ModelMesh* fallbackMesh = m_previewDocument->takeResultMesh();
        if (fallbackMesh && fallbackMesh->triangleVertexCount() > 0) {
            m_previewFallbackMesh.reset(fallbackMesh);
        } else {
            delete fallbackMesh;
        }
    }

    m_previewCurrentFrame = 0;
    displayPreviewFrame();

    if (!m_previewFrameTimer) {
        m_previewFrameTimer = new QTimer(this);
        connect(m_previewFrameTimer, &QTimer::timeout, this, &PreviewOverlayController::onPreviewFrameTimeout);
    }
    int frameCount = (int)m_previewAnimationFrames.size();
    int intervalMs = 100;
    if (m_previewAnimationDurationSeconds > 0.0 && frameCount > 0) {
        intervalMs = std::max(1, static_cast<int>((m_previewAnimationDurationSeconds * 1000.0) / frameCount));
    }
    m_previewFrameTimer->setInterval(intervalMs);
    m_previewFrameTimer->start();
}

void PreviewOverlayController::displayPreviewFrame()
{
    if (!m_modelWidget)
        return;

    if (m_previewFallbackMesh) {
        m_modelWidget->updateMesh(new ModelMesh(*m_previewFallbackMesh));
        m_modelWidget->updateWireframeMesh(nullptr);
        return;
    }

    if (m_previewAnimationFrames.empty())
        return;

    const ModelMesh& frameSource = m_previewAnimationFrames[m_previewCurrentFrame];
    m_modelWidget->updateMesh(new ModelMesh(frameSource));
    m_modelWidget->updateWireframeMesh(nullptr);
}

void PreviewOverlayController::onPreviewFrameTimeout()
{
    if (m_previewAnimationFrames.empty())
        return;

    if (m_previewMovementSpeed > 0.0f && m_modelWidget && m_previewAnimationDurationSeconds > 0.0 && !m_previewAnimationFrames.empty()) {
        float dt = static_cast<float>(m_previewAnimationDurationSeconds / m_previewAnimationFrames.size());
        m_previewGroundOffsetX += m_previewMovementDirectionX * m_previewMovementSpeed * dt;
        m_previewGroundOffsetZ += m_previewMovementDirectionZ * m_previewMovementSpeed * dt;
    }

    m_previewCurrentFrame = (m_previewCurrentFrame + 1) % (int)m_previewAnimationFrames.size();
    displayPreviewFrame();
}
