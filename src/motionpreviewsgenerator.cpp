#include <QGuiApplication>
#include <QElapsedTimer>
#include "motionpreviewsgenerator.h"
#include "tetrapodposer.h"
#include "posemeshcreator.h"

MotionPreviewsGenerator::MotionPreviewsGenerator(const std::vector<AutoRiggerBone> *rigBones,
        const std::map<int, AutoRiggerVertexWeights> *rigWeights,
        const MeshResultContext &meshResultContext) :
    m_rigBones(*rigBones),
    m_rigWeights(*rigWeights),
    m_meshResultContext(meshResultContext)
{
}

MotionPreviewsGenerator::~MotionPreviewsGenerator()
{
    for (auto &item: m_resultPreviewMeshs) {
        for (auto &subItem: item.second) {
            delete subItem.second;
        }
    }
}

void MotionPreviewsGenerator::addPoseToLibrary(const QUuid &poseId, const std::map<QString, std::map<QString, QString>> &parameters)
{
    m_poses[poseId] = parameters;
}

void MotionPreviewsGenerator::addMotionToLibrary(const QUuid &motionId, const std::vector<HermiteControlNode> &controlNodes,
    const std::vector<std::pair<float, QUuid>> &keyframes)
{
    m_motions[motionId] = {controlNodes, keyframes};
}

void MotionPreviewsGenerator::addPreviewRequirement(const QUuid &motionId)
{
    m_requiredMotionIds.insert(motionId);
}

const std::set<QUuid> &MotionPreviewsGenerator::requiredMotionIds()
{
    return m_requiredMotionIds;
}

const std::set<QUuid> &MotionPreviewsGenerator::generatedMotionIds()
{
    return m_generatedMotionIds;
}

void MotionPreviewsGenerator::generateForMotion(const QUuid &motionId)
{
    auto findMotionResult = m_motions.find(motionId);
    if (findMotionResult == m_motions.end())
        return;
    const std::vector<HermiteControlNode> &controlNodes = findMotionResult->second.first;
    std::vector<std::pair<float, QUuid>> keyframes = findMotionResult->second.second;
    if (keyframes.empty())
        return;
    
    auto firstFrame = keyframes[0];
    auto lastFrame = keyframes[keyframes.size() - 1];
    
    if (keyframes[0].first > 0) {
        // Insert the last keyframe as start frame
        keyframes.insert(keyframes.begin(), {0, lastFrame.second});
    }
    
    if (keyframes[keyframes.size() - 1].first < 1) {
        // Insert the first keyframe as stop frame
        keyframes.push_back({1.0, firstFrame.second});
    }
    
    std::vector<std::map<QString, std::map<QString, QString>>> keyframesParameters;
    for (const auto &item: keyframes) {
        const auto &poseId = item.second;
        keyframesParameters.push_back(m_poses[poseId]);
    }
    
    auto findLboundKeyframes = [=, &keyframes](float knot) {
        for (decltype(keyframes.size()) i = 0; i < keyframes.size(); i++) {
            //qDebug() << "Compare knot:" << knot << "keyframe[" << i << "] knot:" << keyframes[i].first;
            if (knot >= keyframes[i].first && i + 1 < keyframes.size() && knot <= keyframes[i + 1].first)
                return (int)i;
        }
        return -1;
    };
    
    TetrapodPoser *poser = new TetrapodPoser(m_rigBones);
    float interval = 1.0 / m_fps;
    float lastKnot = 0;
    for (float knot = 0; knot <= 1; knot += interval) {
        int firstKeyframeIndex = findLboundKeyframes(knot);
        if (-1 == firstKeyframeIndex) {
            continue;
        }
        poser->parameters() = keyframesParameters[firstKeyframeIndex];
        poser->commit();
        auto firstKeyframeJointNodeTree = poser->resultJointNodeTree();
        poser->reset();
        poser->parameters() = keyframesParameters[firstKeyframeIndex + 1];
        poser->commit();
        auto secondKeyframeJointNodeTree = poser->resultJointNodeTree();
        float firstKeyframeKnot = keyframes[firstKeyframeIndex].first;
        float secondKeyframeKnot = keyframes[firstKeyframeIndex + 1].first;
        QVector2D firstKeyframePosition = calculateHermiteInterpolation(controlNodes, firstKeyframeKnot);
        QVector2D secondKeyframePosition = calculateHermiteInterpolation(controlNodes, secondKeyframeKnot);
        QVector2D currentFramePosition = calculateHermiteInterpolation(controlNodes, knot);
        float length = secondKeyframePosition.y() - firstKeyframePosition.y();
        if (qFuzzyIsNull(length))
            continue;
        float t = (currentFramePosition.y() - firstKeyframePosition.y()) / length;
        auto resultJointNodeTree = JointNodeTree::slerp(firstKeyframeJointNodeTree, secondKeyframeJointNodeTree, t);
        
        PoseMeshCreator *poseMeshCreator = new PoseMeshCreator(resultJointNodeTree.nodes(), m_meshResultContext, m_rigWeights);
        poseMeshCreator->createMesh();
        m_resultPreviewMeshs[motionId].push_back({(knot - lastKnot) * 2, poseMeshCreator->takeResultMesh()});
        lastKnot = knot;
        delete poseMeshCreator;
    }
    delete poser;
}

std::vector<std::pair<float, MeshLoader *>> MotionPreviewsGenerator::takeResultPreviewMeshs(const QUuid &motionId)
{
    auto findResult = m_resultPreviewMeshs.find(motionId);
    if (findResult == m_resultPreviewMeshs.end())
        return {};
    auto result = findResult->second;
    m_resultPreviewMeshs.erase(findResult);
    return result;
}

void MotionPreviewsGenerator::generate()
{
    for (const auto &motionId: m_requiredMotionIds) {
        generateForMotion(motionId);
        m_generatedMotionIds.insert(motionId);
    }
}

void MotionPreviewsGenerator::process()
{
    QElapsedTimer countTimeConsumed;
    countTimeConsumed.start();
    
    generate();
    
    qDebug() << "The motion previews generation took" << countTimeConsumed.elapsed() << "milliseconds";
    
    this->moveToThread(QGuiApplication::instance()->thread());
    emit finished();
}
