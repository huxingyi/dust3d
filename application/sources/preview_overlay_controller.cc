#include "preview_overlay_controller.h"
#include "animation_preview_worker.h"
#include "document.h"
#include "model_mesh.h"
#include "scene_widget.h"
#include "theme.h"
#include <QFile>
#include <QMetaObject>
#include <QPointer>
#include <QRandomGenerator>
#include <QRunnable>
#include <QThreadPool>
#include <QTimer>
#include <algorithm>
#include <cmath>
#include <dust3d/base/snapshot.h>
#include <dust3d/base/snapshot_xml.h>
#include <dust3d/rig/rig_generator.h>

namespace {

class PreviewDocumentLoader : public QRunnable {
public:
    explicit PreviewDocumentLoader(PreviewOverlayController* controller, const QString& resourcePath)
        : m_controller(controller)
        , m_resourcePath(resourcePath)
    {
        setAutoDelete(true);
    }

    void run() override
    {
        QPointer<PreviewOverlayController> controller(m_controller);
        QFile file(m_resourcePath);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            qWarning() << "Unable to open preview model resource" << m_resourcePath;
            if (controller)
                QMetaObject::invokeMethod(controller.data(), [controller]() { if (controller) controller->onPreviewDocumentLoaded(dust3d::Snapshot()); }, Qt::QueuedConnection);
            return;
        }

        QByteArray data = file.readAll();
        data.append('\0');
        dust3d::Snapshot snapshot;
        loadSnapshotFromXmlString(&snapshot, data.data());
        if (controller) {
            QMetaObject::invokeMethod(controller.data(), [controller, snapshot = std::move(snapshot)]() mutable {
                if (controller)
                    controller->onPreviewDocumentLoaded(std::move(snapshot)); }, Qt::QueuedConnection);
        }
    }

private:
    PreviewOverlayController* m_controller;
    QString m_resourcePath;
};

}

PreviewOverlayController::PreviewOverlayController(SceneWidget* modelWidget, const QString& previewResourcePath, int previewIndex, QObject* parent)
    : QObject(parent)
    , m_modelWidget(modelWidget)
    , m_previewResourcePath(previewResourcePath)
    , m_previewIndex(previewIndex)
    , m_previewDocument(new Document)
{
    const float offsetDistance = 1.1f;
    const int axisCount = 3;
    float angle = static_cast<float>(m_previewIndex % axisCount) / static_cast<float>(axisCount) * 2.0f * M_PI;
    m_baseOffsetX = std::cos(angle) * offsetDistance;
    m_baseOffsetZ = std::sin(angle) * offsetDistance;
    connect(m_previewDocument, &Document::resultRigChanged,
        this, &PreviewOverlayController::onPreviewDocumentRigReady);
}

void PreviewOverlayController::generateAnimationSet(const QString& animationType, int setIndex, const dust3d::AnimationParams& params)
{
    if (!m_previewDocument || m_previewWorkerBusy)
        return;
    const auto& rigStructure = m_previewDocument->getActualRigStructure();
    if (rigStructure.bones.empty())
        return;
    if (!m_previewDocument->currentRigObject())
        return;

    m_previewAnimationWorker = std::make_unique<AnimationPreviewWorker>();
    m_previewAnimationWorker->setParameters(rigStructure, animationType.toStdString(), params);
    m_previewAnimationWorker->setRigObject(std::unique_ptr<dust3d::Object>(m_previewDocument->takeRigObject()));
    m_previewAnimationWorker->setHideBones(true);
    m_previewAnimationWorker->setHideParts(false);
    m_previewAnimationWorker->setSoundEnabled(false);

    QThread* thread = new QThread;
    m_previewAnimationWorker->moveToThread(thread);
    connect(thread, &QThread::started, m_previewAnimationWorker.get(), &AnimationPreviewWorker::process);
    connect(m_previewAnimationWorker.get(), &AnimationPreviewWorker::finished,
        this, [this, setIndex, thread]() {
            this->onPreviewAnimationReady(setIndex);
            thread->quit();
        });
    connect(thread, &QThread::finished, thread, &QThread::deleteLater);
    thread->start();
    m_previewWorkerBusy = true;
}

QString PreviewOverlayController::animationStateName(State state) const
{
    if (m_isFlightModel) {
        switch (state) {
        case State::Hover:
            return "Fly";
        case State::Glide:
            return "Glide";
        case State::Eat:
            return "Eat";
        default:
            return "Fly";
        }
    }

    switch (state) {
    case State::Idle:
        return "Idle";
    case State::Walk:
        return "Walk";
    case State::Run:
        return "Run";
    case State::Eat:
        return "Eat";
    default:
        return "Idle";
    }
}

PreviewOverlayController::State PreviewOverlayController::movingState() const
{
    return m_isFlightModel ? State::Glide : State::Run;
}

PreviewOverlayController::State PreviewOverlayController::slowMovingState() const
{
    return m_isFlightModel ? State::Glide : State::Walk;
}

PreviewOverlayController::State PreviewOverlayController::hoverState() const
{
    return m_isFlightModel ? State::Hover : State::Idle;
}

void PreviewOverlayController::displayPreviewFrame()
{
    if (!m_modelWidget)
        return;

    if (m_previewFallbackMesh) {
        ModelMesh* mesh = new ModelMesh(*m_previewFallbackMesh);
        translateMesh(*mesh, m_posX, m_posZ);
        m_modelWidget->updatePreviewMesh(m_previewIndex, mesh);
        m_modelWidget->updateWireframeMesh(nullptr);
        return;
    }

    const std::vector<ModelMesh>* frames = currentAnimationFrames();
    if (!frames || frames->empty())
        return;

    const ModelMesh& frameSource = (*frames)[m_previewCurrentFrame % frames->size()];
    ModelMesh* mesh = new ModelMesh(frameSource);
    translateMesh(*mesh, m_posX, m_posZ);
    m_modelWidget->updatePreviewMesh(m_previewIndex, mesh);
    m_modelWidget->updateWireframeMesh(nullptr);
}

void PreviewOverlayController::startNextAnimationSet()
{
    if (!m_previewDocument)
        return;
    if (m_animationGenerationIndex >= static_cast<int>(m_stateOrder.size()))
        return;

    QString animationType = m_previewDocument->getRigType();
    if (animationType.isEmpty())
        animationType = "Quadruped";
    const State state = m_stateOrder[m_animationGenerationIndex];
    QString animationState = animationStateName(state);
    animationType += animationState;
    dust3d::AnimationParams params = findAnimationParametersByState(animationState);
    generateAnimationSet(animationType, static_cast<int>(state), params);
}

dust3d::AnimationParams PreviewOverlayController::findAnimationParametersByState(const QString& stateName) const
{
    dust3d::AnimationParams params;
    if (!m_previewDocument)
        return params;

    QString target = stateName.trimmed().toLower();
    if (target.isEmpty())
        return params;

    std::vector<dust3d::Uuid> animationIds;
    m_previewDocument->getAllAnimationIds(animationIds);
    const Document::Animation* bestMatch = nullptr;
    for (const auto& animationId : animationIds) {
        const auto* anim = m_previewDocument->findAnimation(animationId);
        if (!anim)
            continue;

        QString lowerName = anim->name.trimmed().toLower();
        QString lowerType = anim->type.trimmed().toLower();
        if (lowerName == target) {
            bestMatch = anim;
            break;
        }
        if (lowerName.contains(target) || lowerType.endsWith(target)) {
            if (!bestMatch)
                bestMatch = anim;
        }
    }

    if (bestMatch)
        params.values = bestMatch->params;
    return params;
}

void PreviewOverlayController::initializeWanderState()
{
    m_cuboidColliders.clear();
    if (m_modelWidget) {
        m_cuboidColliders = m_modelWidget->nameCubes();
    }
    m_posX = m_baseOffsetX;
    m_posY = m_isFlightModel ? 1.0f : 0.0f;
    m_posZ = m_baseOffsetZ;
    m_idleTimer = 0.0f;
    m_state = hoverState();
    m_previewCurrentFrame = 0;
    m_initialEscapeDone = m_isFlightModel;
    m_escapeDirectionX = 0.0f;
    m_escapeDirectionZ = 0.0f;
    m_cycleAngle = static_cast<float>(QRandomGenerator::global()->bounded(3600)) / 3600.0f * 2.0f * M_PI;
    scheduleNextTarget();
}

void PreviewOverlayController::scheduleNextTarget()
{
    float centerX = 0.0f;
    float centerZ = 0.0f;
    float maxDist = 0.0f;
    if (!m_cuboidColliders.empty()) {
        for (const auto& cube : m_cuboidColliders) {
            centerX += cube.x;
            centerZ += cube.z;
        }
        centerX /= m_cuboidColliders.size();
        centerZ /= m_cuboidColliders.size();

        for (const auto& cube : m_cuboidColliders) {
            float dx = cube.x - centerX;
            float dz = cube.z - centerZ;
            maxDist = std::max(maxDist, std::sqrt(dx * dx + dz * dz));
        }
    }

    if (m_isFlightModel) {
        float cycleRadius = std::max(10.0f, (maxDist + 1.2f) * 5.0f);
        float step = 0.75f + static_cast<float>(QRandomGenerator::global()->bounded(20, 81)) * 0.01f;
        m_cycleAngle += step;
        while (m_cycleAngle > 2.0f * M_PI)
            m_cycleAngle -= 2.0f * M_PI;

        float radius = cycleRadius * (1.05f + static_cast<float>(QRandomGenerator::global()->bounded(0, 35)) * 0.01f);
        m_targetX = centerX + std::cos(m_cycleAngle) * radius + m_baseOffsetX;
        m_targetZ = centerZ + std::sin(m_cycleAngle) * radius + m_baseOffsetZ;
        if (std::isnan(m_targetX) || std::isnan(m_targetZ)) {
            m_targetX = centerX + m_baseOffsetX;
            m_targetZ = centerZ + m_baseOffsetZ;
        }
        m_state = movingState();
        return;
    }

    if (!m_initialEscapeDone) {
        std::vector<QVector2D> dropCenters;
        if (m_modelWidget)
            dropCenters = m_modelWidget->dropSpawnCenters();

        float escapeX = centerX;
        float escapeZ = centerZ;
        float dirX = 1.0f;
        float dirZ = 0.0f;
        float currentDist = std::sqrt((m_posX - escapeX) * (m_posX - escapeX) + (m_posZ - escapeZ) * (m_posZ - escapeZ));
        float escapeRadius = std::max(3.8f, maxDist + 2.8f);

        if (!dropCenters.empty()) {
            QVector2D dropCenter(0.0f, 0.0f);
            for (const auto& point : dropCenters)
                dropCenter += point;
            dropCenter /= static_cast<float>(dropCenters.size());

            escapeX = dropCenter.x();
            escapeZ = dropCenter.y();

            dirX = m_posX - dropCenter.x();
            dirZ = m_posZ - dropCenter.y();
            float len = std::sqrt(dirX * dirX + dirZ * dirZ);
            currentDist = len;
            if (len < 1e-5f) {
                float clusterDx = centerX - dropCenter.x();
                float clusterDz = centerZ - dropCenter.y();
                float clusterLen = std::sqrt(clusterDx * clusterDx + clusterDz * clusterDz);
                if (clusterLen > 1e-5f) {
                    dirX = -clusterDx;
                    dirZ = -clusterDz;
                    len = clusterLen;
                } else {
                    dirX = 1.0f;
                    dirZ = 0.0f;
                    len = 1.0f;
                }
            }

            dirX /= len;
            dirZ /= len;

            float safeDistance = 0.0f;
            for (const auto& cube : m_cuboidColliders) {
                float bx = cube.x - escapeX;
                float bz = cube.z - escapeZ;
                float proj = bx * dirX + bz * dirZ;
                float cubeRadius = std::max(cube.width, cube.depth) * 0.65f + 0.35f;
                safeDistance = std::max(safeDistance, proj + cubeRadius);
            }
            escapeRadius = std::max(escapeRadius, safeDistance + 1.8f);
        }

        escapeRadius = std::max(escapeRadius, currentDist * 10.0f);

        dirX = std::max(-1.0f, std::min(1.0f, dirX));
        dirZ = std::max(-1.0f, std::min(1.0f, dirZ));
        float dirLen = std::sqrt(dirX * dirX + dirZ * dirZ);
        if (dirLen < 1e-5f) {
            dirX = 1.0f;
            dirZ = 0.0f;
            dirLen = 1.0f;
        }
        m_escapeDirectionX = -dirX / dirLen;
        m_escapeDirectionZ = -dirZ / dirLen;

        m_targetX = m_posX + m_escapeDirectionX * escapeRadius;
        m_targetZ = m_posZ + m_escapeDirectionZ * escapeRadius;

        if (std::isnan(m_targetX) || std::isnan(m_targetZ)) {
            m_targetX = m_posX + m_escapeDirectionX * escapeRadius;
            m_targetZ = m_posZ + m_escapeDirectionZ * escapeRadius;
        }

        m_state = movingState();
        return;
    }

    float cycleRadius = std::max(1.05f, maxDist + 0.45f);
    float step = 0.75f + static_cast<float>(QRandomGenerator::global()->bounded(20, 81)) * 0.01f;
    m_cycleAngle += step;
    while (m_cycleAngle > 2.0f * M_PI)
        m_cycleAngle -= 2.0f * M_PI;

    float radius = cycleRadius * (0.88f + static_cast<float>(QRandomGenerator::global()->bounded(0, 25)) * 0.01f);
    m_targetX = centerX + std::cos(m_cycleAngle) * radius;
    m_targetZ = centerZ + std::sin(m_cycleAngle) * radius;

    float offsetX = static_cast<float>(QRandomGenerator::global()->bounded(-10, 11)) * 0.01f;
    float offsetZ = static_cast<float>(QRandomGenerator::global()->bounded(-10, 11)) * 0.01f;
    m_targetX += offsetX;
    m_targetZ += offsetZ;

    m_targetX += m_baseOffsetX;
    m_targetZ += m_baseOffsetZ;

    if (std::isnan(m_targetX) || std::isnan(m_targetZ)) {
        m_targetX = centerX;
        m_targetZ = centerZ;
    }
}

void PreviewOverlayController::updateWanderMovement(float dt)
{
    if (m_modelWidget)
        m_cuboidColliders = m_modelWidget->nameCubes();
    if (m_isFlightModel)
        m_cuboidColliders.clear();

    if (m_idleTimer > 0.0f) {
        m_idleTimer -= dt;
        bool threatNearby = false;
        for (const auto& cube : m_cuboidColliders) {
            float ox = m_posX - cube.x;
            float oz = m_posZ - cube.z;
            float radius = std::max(cube.width, cube.depth) * 0.65f + 0.40f;
            float dist = std::sqrt(ox * ox + oz * oz);
            if (dist < radius) {
                threatNearby = true;
                break;
            }
        }
        if (m_idleTimer > 0.0f && !threatNearby)
            return;

        if (m_state == State::Eat)
            scheduleNextTarget();

        if (m_cuboidColliders.empty()) {
            m_state = hoverState();
            return;
        }

        float dx = m_targetX - m_posX;
        float dz = m_targetZ - m_posZ;
        float distance = std::sqrt(dx * dx + dz * dz);
        if (distance < 0.18f) {
            scheduleNextTarget();
            return;
        }

        m_state = slowMovingState();
        return;
    }

    if (m_cuboidColliders.empty()) {
        float dx = m_targetX - m_posX;
        float dz = m_targetZ - m_posZ;
        float distance = std::sqrt(dx * dx + dz * dz);
        if (distance < 0.18f) {
            if (m_state == movingState()) {
                m_state = hoverState();
                m_idleTimer = 0.8f + static_cast<float>(QRandomGenerator::global()->bounded(0, 120)) * 0.01f;
                return;
            }
            scheduleNextTarget();
            return;
        }

        float steerX = dx;
        float steerZ = dz;
        float steerLen = std::sqrt(steerX * steerX + steerZ * steerZ);
        if (steerLen > 1e-6f) {
            steerX /= steerLen;
            steerZ /= steerLen;
            m_state = movingState();
            float desiredYaw = std::atan2(steerX, steerZ);
            float yawDelta = desiredYaw - m_yaw;
            while (yawDelta > M_PI)
                yawDelta -= 2.0f * M_PI;
            while (yawDelta < -M_PI)
                yawDelta += 2.0f * M_PI;

            float maxTurn = dt * 3.5f;
            float turn = std::max(-maxTurn, std::min(maxTurn, yawDelta));
            m_yaw += turn;
            while (m_yaw > M_PI)
                m_yaw -= 2.0f * M_PI;
            while (m_yaw < -M_PI)
                m_yaw += 2.0f * M_PI;

            float speed = m_animationSets[static_cast<int>(m_state)].movementSpeed;
            if (speed <= 0.01f)
                speed = 2.8f;

            float forwardX = std::sin(m_yaw);
            float forwardZ = std::cos(m_yaw);
            m_posX += forwardX * speed * dt;
            m_posZ += forwardZ * speed * dt;
        }
        return;
    }

    float dx = m_targetX - m_posX;
    float dz = m_targetZ - m_posZ;
    float distance = std::sqrt(dx * dx + dz * dz);
    if (!m_initialEscapeDone) {
        if (distance < 0.18f) {
            m_initialEscapeDone = true;
            scheduleNextTarget();
            return;
        }

        m_state = movingState();
        if (m_escapeDirectionX == 0.0f && m_escapeDirectionZ == 0.0f) {
            float len = std::sqrt(dx * dx + dz * dz);
            m_escapeDirectionX = (len > 1e-5f) ? dx / len : 1.0f;
            m_escapeDirectionZ = (len > 1e-5f) ? dz / len : 0.0f;
        }

        m_yaw = std::atan2(m_escapeDirectionX, m_escapeDirectionZ);

        float speed = m_animationSets[static_cast<int>(m_state)].movementSpeed;
        if (speed <= 0.01f)
            speed = 2.8f;
        speed = std::max(speed, 4.0f);

        m_posX += m_escapeDirectionX * speed * dt;
        m_posZ += m_escapeDirectionZ * speed * dt;
        return;
    }
    if (distance < 0.18f) {
        if (m_state == movingState()) {
            m_state = hoverState();
            return;
        }

        float pick = static_cast<float>(QRandomGenerator::global()->bounded(100)) / 100.0f;
        if (pick < 0.35f) {
            m_state = State::Eat;
            m_idleTimer = 1.4f + static_cast<float>(QRandomGenerator::global()->bounded(0, 120)) * 0.01f;
        } else {
            m_state = hoverState();
            m_idleTimer = 0.8f + static_cast<float>(QRandomGenerator::global()->bounded(0, 120)) * 0.01f;
            if (pick < 0.70f)
                scheduleNextTarget();
        }
        return;
    }

    float steerX = dx;
    float steerZ = dz;
    bool avoid = false;
    for (const auto& cube : m_cuboidColliders) {
        float ox = m_posX - cube.x;
        float oz = m_posZ - cube.z;
        float radius = std::max(cube.width, cube.depth) * 0.65f + 0.24f;
        float dist = std::sqrt(ox * ox + oz * oz);
        if (dist < radius && dist > 1e-6f) {
            float repel = (radius - dist) * 2.0f;
            steerX += ox / dist * repel;
            steerZ += oz / dist * repel;
            avoid = true;
            if (dist < 0.9f) {
                m_targetX = m_posX + ox / dist * (radius + 0.8f);
                m_targetZ = m_posZ + oz / dist * (radius + 0.8f);
            }
        }
    }

    float steerLen = std::sqrt(steerX * steerX + steerZ * steerZ);
    if (steerLen <= 1e-6f) {
        if (m_state == movingState()) {
            steerX = std::sin(m_yaw);
            steerZ = std::cos(m_yaw);
            steerLen = 1.0f;
        } else {
            return;
        }
    }
    steerX /= steerLen;
    steerZ /= steerLen;

    if (!m_initialEscapeDone) {
        m_state = movingState();
    } else if (avoid || distance > 1.0f) {
        bool shouldRun = (distance > 2.0f || avoid || QRandomGenerator::global()->bounded(100) < 55);
        if (shouldRun && (m_state == slowMovingState() || m_state == movingState()))
            m_state = movingState();
        else
            m_state = slowMovingState();
    } else {
        m_state = slowMovingState();
    }

    float desiredYaw = std::atan2(steerX, steerZ);
    float yawDelta = desiredYaw - m_yaw;
    while (yawDelta > M_PI)
        yawDelta -= 2.0f * M_PI;
    while (yawDelta < -M_PI)
        yawDelta += 2.0f * M_PI;

    float maxTurn = dt * 3.5f;
    float turn = std::max(-maxTurn, std::min(maxTurn, yawDelta));
    m_yaw += turn;
    while (m_yaw > M_PI)
        m_yaw -= 2.0f * M_PI;
    while (m_yaw < -M_PI)
        m_yaw += 2.0f * M_PI;

    float speed = m_animationSets[static_cast<int>(m_state)].movementSpeed;
    if (speed <= 0.01f)
        speed = (m_state == movingState()) ? 2.2f : 0.65f;
    if (!m_initialEscapeDone && m_state == movingState())
        speed = std::max(speed, 4.0f);

    float forwardX = std::sin(m_yaw);
    float forwardZ = std::cos(m_yaw);
    m_posX += forwardX * speed * dt;
    m_posZ += forwardZ * speed * dt;
}

void PreviewOverlayController::translateMesh(ModelMesh& mesh, float offsetX, float offsetZ)
{
    ModelOpenGLVertex* vertices = mesh.triangleVertices();
    int count = mesh.triangleVertexCount();
    float cosYaw = std::cos(m_yaw);
    float sinYaw = std::sin(m_yaw);
    for (int i = 0; i < count; ++i) {
        float localX = vertices[i].posX;
        float localZ = vertices[i].posZ;
        vertices[i].posX = localX * cosYaw - localZ * sinYaw + offsetX;
        vertices[i].posZ = localX * sinYaw + localZ * cosYaw + offsetZ;
        vertices[i].posY += m_posY;
    }
}

const std::vector<ModelMesh>* PreviewOverlayController::currentAnimationFrames() const
{
    const auto& currentFrames = m_animationSets[static_cast<int>(m_state)].frames;
    if (!currentFrames.empty())
        return &currentFrames;

    auto fallbackFrames = [&](std::initializer_list<State> order) -> const std::vector<ModelMesh>* {
        for (State state : order) {
            const auto& frames = m_animationSets[static_cast<int>(state)].frames;
            if (!frames.empty())
                return &frames;
        }
        return nullptr;
    };

    switch (m_state) {
    case State::Glide:
        return fallbackFrames({ State::Hover, State::Idle, State::Walk, State::Run, State::Eat, State::Glide });
    case State::Hover:
        return fallbackFrames({ State::Glide, State::Idle, State::Walk, State::Run, State::Eat, State::Hover });
    case State::Run:
        return fallbackFrames({ State::Walk, State::Idle, State::Eat, State::Run });
    case State::Walk:
        return fallbackFrames({ State::Run, State::Idle, State::Eat, State::Walk });
    case State::Eat:
        return fallbackFrames({ State::Idle, State::Walk, State::Run, State::Eat });
    case State::Idle:
    default:
        return fallbackFrames({ State::Walk, State::Run, State::Eat, State::Idle });
    }
}

int PreviewOverlayController::currentFrameIntervalMs() const
{
    const auto& data = m_animationSets[static_cast<int>(m_state)];
    if (!data.frames.empty())
        return data.frameIntervalMs;
    for (int i = 0; i < static_cast<int>(State::Count); ++i) {
        if (!m_animationSets[i].frames.empty())
            return m_animationSets[i].frameIntervalMs;
    }
    return 100;
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

    bool haveFrames = false;
    for (int i = 0; i < static_cast<int>(State::Count); ++i) {
        if (!m_animationSets[i].frames.empty()) {
            haveFrames = true;
            break;
        }
    }

    if (haveFrames || m_previewFallbackMesh) {
        if (!m_previewFrameTimer) {
            m_previewFrameTimer = new QTimer(this);
            connect(m_previewFrameTimer, &QTimer::timeout, this, &PreviewOverlayController::onPreviewFrameTimeout);
        }
        m_previewFrameTimer->start();
        return;
    }

    m_previewDocumentLoading = true;
    QThreadPool::globalInstance()->start(new PreviewDocumentLoader(this, m_previewResourcePath));
}

void PreviewOverlayController::stop()
{
    m_previewActive = false;

    if (m_previewFrameTimer)
        m_previewFrameTimer->stop();

    for (auto& set : m_animationSets)
        set.frames.clear();
    m_previewFallbackMesh.reset();
    m_previewCurrentFrame = 0;
    m_idleTimer = 0.0f;
    m_posX = 0.0f;
    m_posZ = 0.0f;
    m_targetX = 0.0f;
    m_targetZ = 0.0f;
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

    const auto& rigStructure = m_previewDocument->getActualRigStructure();
    if (rigStructure.bones.empty()) {
        qWarning() << "Preview document rig has no bones";
        return;
    }
    if (!m_previewDocument->currentRigObject()) {
        qWarning() << "Preview document rig object missing";
        return;
    }

    std::vector<dust3d::Uuid> animationIds;
    m_previewDocument->getAllAnimationIds(animationIds);
    m_hasRunAnimation = false;
    m_hasEatAnimation = false;
    m_hasFlyAnimation = false;
    m_hasGlideAnimation = false;
    for (const auto& animationId : animationIds) {
        const auto* anim = m_previewDocument->findAnimation(animationId);
        if (!anim)
            continue;
        QString lowerName = anim->name.trimmed().toLower();
        QString lowerType = anim->type.trimmed().toLower();
        if (lowerName == "run" || lowerType.endsWith("run") || lowerName.contains("run"))
            m_hasRunAnimation = true;
        if (lowerName == "eat" || lowerType.endsWith("eat") || lowerName.contains("eat"))
            m_hasEatAnimation = true;
        if (lowerName == "fly" || lowerType.endsWith("fly") || lowerName.contains("fly"))
            m_hasFlyAnimation = true;
        if (lowerName == "glide" || lowerType.endsWith("glide") || lowerName.contains("glide"))
            m_hasGlideAnimation = true;
    }

    m_isFlightModel = m_hasFlyAnimation;
    m_stateOrder.clear();
    if (m_isFlightModel) {
        m_stateOrder.push_back(State::Hover);
        m_stateOrder.push_back(State::Glide);
        if (m_hasEatAnimation)
            m_stateOrder.push_back(State::Eat);
    } else {
        m_stateOrder.push_back(State::Idle);
        m_stateOrder.push_back(State::Walk);
        if (m_hasRunAnimation)
            m_stateOrder.push_back(State::Run);
        if (m_hasEatAnimation)
            m_stateOrder.push_back(State::Eat);
    }
    m_animationGenerationIndex = 0;
    startNextAnimationSet();
}

void PreviewOverlayController::onPreviewAnimationReady(int setIndex)
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

    auto& animationSet = m_animationSets[setIndex];
    animationSet.frames = m_previewAnimationWorker->takePreviewMeshes();
    animationSet.durationSeconds = m_previewAnimationWorker->durationSeconds();
    animationSet.movementSpeed = m_previewAnimationWorker->movementSpeed();
    if (!animationSet.frames.empty()) {
        animationSet.frameIntervalMs = std::max(1, static_cast<int>((animationSet.durationSeconds * 1000.0) / animationSet.frames.size()));
    }

    m_previewAnimationWorker.reset();
    m_animationGenerationIndex++;
    if (m_animationGenerationIndex < static_cast<int>(m_stateOrder.size())) {
        startNextAnimationSet();
        return;
    }

    bool anyFrames = false;
    for (int i = 0; i < static_cast<int>(State::Count); ++i) {
        if (!m_animationSets[i].frames.empty()) {
            anyFrames = true;
            break;
        }
    }
    if (!anyFrames) {
        qWarning() << "Preview animation produced no frames";
        return;
    }

    // Keep a fallback static mesh when no skinned frames are available.
    bool allSkeletonOnly = true;
    for (int i = 0; i < static_cast<int>(State::Count); ++i) {
        for (const auto& frame : m_animationSets[i].frames) {
            if (frame.skeletonVertexCount() < frame.triangleVertexCount()) {
                allSkeletonOnly = false;
                break;
            }
        }
        if (!allSkeletonOnly)
            break;
    }
    if (allSkeletonOnly && m_previewDocument) {
        ModelMesh* fallbackMesh = m_previewDocument->takeResultMesh();
        if (fallbackMesh && fallbackMesh->triangleVertexCount() > 0) {
            m_previewFallbackMesh.reset(fallbackMesh);
        } else {
            delete fallbackMesh;
        }
    }

    if (m_isFlightModel) {
        if (m_animationSets[static_cast<int>(State::Glide)].frames.empty()) {
            const auto& hoverSet = m_animationSets[static_cast<int>(State::Hover)];
            if (!hoverSet.frames.empty()) {
                m_animationSets[static_cast<int>(State::Glide)] = hoverSet;
                m_animationSets[static_cast<int>(State::Glide)].movementSpeed = std::max(2.2f, hoverSet.movementSpeed * 1.2f);
            }
        }
        if (m_animationSets[static_cast<int>(State::Hover)].frames.empty()) {
            const auto& glideSet = m_animationSets[static_cast<int>(State::Glide)];
            if (!glideSet.frames.empty()) {
                m_animationSets[static_cast<int>(State::Hover)] = glideSet;
                m_animationSets[static_cast<int>(State::Hover)].movementSpeed = std::max(0.5f, glideSet.movementSpeed * 0.6f);
            }
        }
    } else {
        if (m_animationSets[static_cast<int>(State::Run)].frames.empty()) {
            const auto& walkSet = m_animationSets[static_cast<int>(State::Walk)];
            const auto& idleSet = m_animationSets[static_cast<int>(State::Idle)];
            if (!walkSet.frames.empty()) {
                m_animationSets[static_cast<int>(State::Run)] = walkSet;
                m_animationSets[static_cast<int>(State::Run)].movementSpeed = std::max(2.2f, walkSet.movementSpeed * 1.4f);
            } else if (!idleSet.frames.empty()) {
                m_animationSets[static_cast<int>(State::Run)] = idleSet;
                m_animationSets[static_cast<int>(State::Run)].movementSpeed = 2.2f;
            }
        }
    }

    initializeWanderState();
    updateWanderMovement(1.0f / 60.0f);
    if (!m_previewFrameTimer) {
        m_previewFrameTimer = new QTimer(this);
        connect(m_previewFrameTimer, &QTimer::timeout, this, &PreviewOverlayController::onPreviewFrameTimeout);
    }
    m_previewFrameTimer->setInterval(currentFrameIntervalMs());
    m_previewFrameTimer->start();
    displayPreviewFrame();

    if (m_modelWidget)
        m_modelWidget->triggerDropSimulation();
}

void PreviewOverlayController::onPreviewFrameTimeout()
{
    const std::vector<ModelMesh>* frames = currentAnimationFrames();
    if (!frames || frames->empty())
        return;

    float dt = currentFrameIntervalMs() * 0.001f;
    updateWanderMovement(dt);

    m_previewCurrentFrame = (m_previewCurrentFrame + 1) % (int)frames->size();
    if (m_previewFrameTimer)
        m_previewFrameTimer->setInterval(currentFrameIntervalMs());
    displayPreviewFrame();
}
