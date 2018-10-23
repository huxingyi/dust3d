#include <QGuiApplication>
#include <QElapsedTimer>
#include <cmath>
#include "motionsgenerator.h"
#include "tetrapodposer.h"
#include "posemeshcreator.h"

MotionsGenerator::MotionsGenerator(const std::vector<AutoRiggerBone> *rigBones,
        const std::map<int, AutoRiggerVertexWeights> *rigWeights,
        const MeshResultContext &meshResultContext) :
    m_rigBones(*rigBones),
    m_rigWeights(*rigWeights),
    m_meshResultContext(meshResultContext)
{
}

MotionsGenerator::~MotionsGenerator()
{
    for (auto &item: m_resultPreviewMeshs) {
        for (auto &subItem: item.second) {
            delete subItem.second;
        }
    }
    delete m_poser;
}

void MotionsGenerator::addPoseToLibrary(const QUuid &poseId, const std::map<QString, std::map<QString, QString>> &parameters)
{
    m_poses[poseId] = parameters;
}

void MotionsGenerator::addMotionToLibrary(const QUuid &motionId, const std::vector<SkeletonMotionClip> &clips)
{
    m_motions[motionId] = clips;
}

void MotionsGenerator::addRequirement(const QUuid &motionId)
{
    m_requiredMotionIds.insert(motionId);
}

const std::set<QUuid> &MotionsGenerator::requiredMotionIds()
{
    return m_requiredMotionIds;
}

const std::set<QUuid> &MotionsGenerator::generatedMotionIds()
{
    return m_generatedMotionIds;
}

std::vector<SkeletonMotionClip> *MotionsGenerator::findMotionClips(const QUuid &motionId)
{
    auto findMotionResult = m_motions.find(motionId);
    if (findMotionResult == m_motions.end())
        return nullptr;
    std::vector<SkeletonMotionClip> &clips = findMotionResult->second;
    return &clips;
}

void MotionsGenerator::generatePreviewsForOutcomes(const std::vector<std::pair<float, JointNodeTree>> &outcomes, std::vector<std::pair<float, MeshLoader *>> &previews)
{
    for (const auto &item: outcomes) {
        PoseMeshCreator *poseMeshCreator = new PoseMeshCreator(item.second.nodes(), m_meshResultContext, m_rigWeights);
        poseMeshCreator->createMesh();
        previews.push_back({item.first, poseMeshCreator->takeResultMesh()});
        delete poseMeshCreator;
    }
}

float MotionsGenerator::calculateMotionDuration(const QUuid &motionId, std::set<QUuid> &visited)
{
    const std::vector<SkeletonMotionClip> *motionClips = findMotionClips(motionId);
    if (!motionClips || motionClips->empty())
        return 0;
    if (visited.find(motionId) != visited.end()) {
        qDebug() << "Found recursive motion link";
        return 0;
    }
    float totalDuration = 0;
    visited.insert(motionId);
    for (const auto &clip: *motionClips) {
        if (clip.clipType == SkeletonMotionClipType::Interpolation)
            totalDuration += clip.duration;
        else if (clip.clipType == SkeletonMotionClipType::Pose)
            totalDuration += clip.duration;
        else if (clip.clipType == SkeletonMotionClipType::Motion)
            totalDuration += calculateMotionDuration(clip.linkToId, visited);
    }
    return totalDuration;
}

void MotionsGenerator::generateMotion(const QUuid &motionId, std::set<QUuid> &visited, std::vector<std::pair<float, JointNodeTree>> &outcomes)
{
    if (visited.find(motionId) != visited.end()) {
        qDebug() << "Found recursive motion link";
        return;
    }
    
    visited.insert(motionId);
    
    std::vector<SkeletonMotionClip> *motionClips = findMotionClips(motionId);
    if (!motionClips || motionClips->empty())
        return;
    
    std::vector<float> timePoints;
    float totalDuration = 0;
    for (auto &clip: *motionClips) {
        if (clip.clipType == SkeletonMotionClipType::Motion) {
            std::set<QUuid> subVisited;
            clip.duration = calculateMotionDuration(clip.linkToId, subVisited);
        }
        timePoints.push_back(totalDuration);
        totalDuration += clip.duration;
    }
    
    auto findClipIndexByProgress = [=](float progress) {
        for (size_t i = 0; i < timePoints.size(); ++i) {
            if (progress >= timePoints[i] && i + 1 < timePoints.size() && progress <= timePoints[i + 1])
                return (int)i;
        }
        return (int)timePoints.size() - 1;
    };
    
    float interval = 1.0 / m_fps;
    float lastProgress = 0;
    for (float progress = 0; progress < totalDuration; ) {
        int clipIndex = findClipIndexByProgress(progress);
        if (-1 == clipIndex) {
            qDebug() << "findClipIndexByProgress failed, progress:" << progress << "total duration:" << totalDuration << "interval:" << interval;
            break;
        }
        float clipLocalProgress = progress - timePoints[clipIndex];
        const SkeletonMotionClip &progressClip = (*motionClips)[clipIndex];
        if (SkeletonMotionClipType::Interpolation == progressClip.clipType) {
            if (clipIndex <= 0) {
                qDebug() << "Clip type is interpolation, but clip sit at begin";
                break;
            }
            if (clipIndex >= (int)motionClips->size() - 1) {
                qDebug() << "Clip type is interpolation, but clip sit at end";
                break;
            }
            const JointNodeTree *beginJointNodeTree = findClipBeginJointNodeTree((*motionClips)[clipIndex - 1]);
            if (nullptr == beginJointNodeTree) {
                qDebug() << "findClipBeginJointNodeTree failed";
                break;
            }
            const JointNodeTree *endJointNodeTree = findClipEndJointNodeTree((*motionClips)[clipIndex + 1]);
            if (nullptr == endJointNodeTree) {
                qDebug() << "findClipEndJointNodeTree failed";
                break;
            }
            outcomes.push_back({progress - lastProgress,
                generateInterpolation(progressClip.interpolationType, *beginJointNodeTree, *endJointNodeTree, clipLocalProgress / std::max((float)0.01, progressClip.duration))});
            lastProgress = progress;
            progress += interval;
            continue;
        } else if (SkeletonMotionClipType::Pose == progressClip.clipType) {
            const JointNodeTree *beginJointNodeTree = findClipBeginJointNodeTree((*motionClips)[clipIndex]);
            if (nullptr == beginJointNodeTree) {
                qDebug() << "findClipBeginJointNodeTree failed";
                break;
            }
            outcomes.push_back({progress - lastProgress, *beginJointNodeTree});
            lastProgress = progress;
            progress += interval;
            continue;
        } else if (SkeletonMotionClipType::Motion == progressClip.clipType) {
            generateMotion(progressClip.linkToId, visited, outcomes);
            progress += progressClip.duration;
            continue;
        }
        progress += interval;
    }
}

JointNodeTree MotionsGenerator::generateInterpolation(InterpolationType interpolationType, const JointNodeTree &first, const JointNodeTree &second, float progress)
{
    return JointNodeTree::slerp(first, second, calculateInterpolation(interpolationType, progress));
}

const JointNodeTree &MotionsGenerator::poseJointNodeTree(const QUuid &poseId)
{
    auto findResult = m_poseJointNodeTreeMap.find(poseId);
    if (findResult != m_poseJointNodeTreeMap.end())
        return findResult->second;
    
    m_poser->reset();
    m_poser->parameters() = m_poses[poseId];
    m_poser->commit();
    auto insertResult = m_poseJointNodeTreeMap.insert({poseId, m_poser->resultJointNodeTree()});
    return insertResult.first->second;
}

const JointNodeTree *MotionsGenerator::findClipBeginJointNodeTree(const SkeletonMotionClip &clip)
{
    if (SkeletonMotionClipType::Pose == clip.clipType) {
        const JointNodeTree &jointNodeTree = poseJointNodeTree(clip.linkToId);
        return &jointNodeTree;
    } else if (SkeletonMotionClipType::Motion == clip.clipType) {
        const std::vector<SkeletonMotionClip> *motionClips = findMotionClips(clip.linkToId);
        if (nullptr != motionClips && !motionClips->empty()) {
            return findClipBeginJointNodeTree((*motionClips)[0]);
        }
        return nullptr;
    }
    return nullptr;
}

const JointNodeTree *MotionsGenerator::findClipEndJointNodeTree(const SkeletonMotionClip &clip)
{
    if (SkeletonMotionClipType::Pose == clip.clipType) {
        const JointNodeTree &jointNodeTree = poseJointNodeTree(clip.linkToId);
        return &jointNodeTree;
    } else if (SkeletonMotionClipType::Motion == clip.clipType) {
        const std::vector<SkeletonMotionClip> *motionClips = findMotionClips(clip.linkToId);
        if (nullptr != motionClips && !motionClips->empty()) {
            return findClipEndJointNodeTree((*motionClips)[motionClips->size() - 1]);
        }
        return nullptr;
    }
    return nullptr;
}

std::vector<std::pair<float, MeshLoader *>> MotionsGenerator::takeResultPreviewMeshs(const QUuid &motionId)
{
    auto findResult = m_resultPreviewMeshs.find(motionId);
    if (findResult == m_resultPreviewMeshs.end())
        return {};
    auto result = findResult->second;
    m_resultPreviewMeshs.erase(findResult);
    return result;
}

std::vector<std::pair<float, JointNodeTree>> MotionsGenerator::takeResultJointNodeTrees(const QUuid &motionId)
{
    auto findResult = m_resultJointNodeTrees.find(motionId);
    if (findResult == m_resultJointNodeTrees.end())
        return {};
    return findResult->second;
}

void MotionsGenerator::generate()
{
    m_poser = new TetrapodPoser(m_rigBones);
    
    for (const auto &motionId: m_requiredMotionIds) {
        std::set<QUuid> visited;
        generateMotion(motionId, visited, m_resultJointNodeTrees[motionId]);
        generatePreviewsForOutcomes(m_resultJointNodeTrees[motionId], m_resultPreviewMeshs[motionId]);
        m_generatedMotionIds.insert(motionId);
    }
}

void MotionsGenerator::process()
{
    QElapsedTimer countTimeConsumed;
    countTimeConsumed.start();
    
    generate();
    
    qDebug() << "The motions generation took" << countTimeConsumed.elapsed() << "milliseconds";
    
    this->moveToThread(QGuiApplication::instance()->thread());
    emit finished();
}
