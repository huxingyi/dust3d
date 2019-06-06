#include <QGuiApplication>
#include <QElapsedTimer>
#include <cmath>
#include "motionsgenerator.h"
#include "posemeshcreator.h"
#include "poserconstruct.h"

MotionsGenerator::MotionsGenerator(RigType rigType,
        const std::vector<RiggerBone> *rigBones,
        const std::map<int, RiggerVertexWeights> *rigWeights,
        const Outcome &outcome) :
    m_rigType(rigType),
    m_rigBones(*rigBones),
    m_rigWeights(*rigWeights),
    m_outcome(outcome)
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

void MotionsGenerator::addPoseToLibrary(const QUuid &poseId, const std::vector<std::pair<std::map<QString, QString>, std::map<QString, std::map<QString, QString>>>> &frames)
{
    m_poses[poseId] = frames;
}

void MotionsGenerator::addMotionToLibrary(const QUuid &motionId, const std::vector<MotionClip> &clips)
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

std::vector<MotionClip> *MotionsGenerator::findMotionClips(const QUuid &motionId)
{
    auto findMotionResult = m_motions.find(motionId);
    if (findMotionResult == m_motions.end())
        return nullptr;
    std::vector<MotionClip> &clips = findMotionResult->second;
    return &clips;
}

std::vector<std::pair<std::map<QString, QString>, std::map<QString, std::map<QString, QString>>>> *MotionsGenerator::findPoseFrames(const QUuid &poseId)
{
    auto findPoseResult = m_poses.find(poseId);
    if (findPoseResult == m_poses.end())
        return nullptr;
    std::vector<std::pair<std::map<QString, QString>, std::map<QString, std::map<QString, QString>>>> &frames = findPoseResult->second;
    return &frames;
}

void MotionsGenerator::generatePreviewsForOutcomes(const std::vector<std::pair<float, JointNodeTree>> &outcomes, std::vector<std::pair<float, MeshLoader *>> &previews)
{
    for (const auto &item: outcomes) {
        PoseMeshCreator *poseMeshCreator = new PoseMeshCreator(item.second.nodes(), m_outcome, m_rigWeights);
        poseMeshCreator->createMesh();
        previews.push_back({item.first, poseMeshCreator->takeResultMesh()});
        delete poseMeshCreator;
    }
}

float MotionsGenerator::calculatePoseDuration(const QUuid &poseId)
{
    const std::vector<std::pair<std::map<QString, QString>, std::map<QString, std::map<QString, QString>>>> *pose = findPoseFrames(poseId);
    if (nullptr == pose)
        return 0;
    float totalDuration = 0;
    if (pose->size() > 1) {
        // Pose with only one frame has zero duration
        for (const auto &frame: *pose) {
            totalDuration += valueOfKeyInMapOrEmpty(frame.first, "duration").toFloat();
        }
    }
    return totalDuration;
}

float MotionsGenerator::calculateMotionDuration(const QUuid &motionId, std::set<QUuid> &visited)
{
    const std::vector<MotionClip> *motionClips = findMotionClips(motionId);
    if (!motionClips || motionClips->empty())
        return 0;
    if (visited.find(motionId) != visited.end()) {
        qDebug() << "Found recursive motion link";
        return 0;
    }
    float totalDuration = 0;
    visited.insert(motionId);
    for (const auto &clip: *motionClips) {
        if (clip.clipType == MotionClipType::Interpolation)
            totalDuration += clip.duration;
        else if (clip.clipType == MotionClipType::Pose)
            totalDuration += calculatePoseDuration(clip.linkToId);
        else if (clip.clipType == MotionClipType::Motion)
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
    
    std::vector<MotionClip> *motionClips = findMotionClips(motionId);
    if (!motionClips || motionClips->empty())
        return;
    
    std::vector<float> timePoints;
    float totalDuration = 0;
    for (auto &clip: *motionClips) {
        if (clip.clipType == MotionClipType::Motion) {
            std::set<QUuid> subVisited;
            clip.duration = calculateMotionDuration(clip.linkToId, subVisited);
        } else if (clip.clipType == MotionClipType::Pose) {
            clip.duration = calculatePoseDuration(clip.linkToId);
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
    if (totalDuration < interval)
        totalDuration = interval;
    for (float progress = 0; progress < totalDuration; ) {
        int clipIndex = findClipIndexByProgress(progress);
        if (-1 == clipIndex) {
            qDebug() << "findClipIndexByProgress failed, progress:" << progress << "total duration:" << totalDuration << "interval:" << interval;
            break;
        }
        float clipLocalProgress = progress - timePoints[clipIndex];
        const MotionClip &progressClip = (*motionClips)[clipIndex];
        if (MotionClipType::Interpolation == progressClip.clipType) {
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
                generateInterpolation(progressClip.interpolationType, *beginJointNodeTree, *endJointNodeTree, clipLocalProgress / std::max((float)0.0001, progressClip.duration))});
            lastProgress = progress;
            progress += interval;
            continue;
        } else if (MotionClipType::Pose == progressClip.clipType) {
            const auto &frames = findPoseFrames(progressClip.linkToId);
            float clipDuration = std::max((float)0.0001, progressClip.duration);
            int frame = clipLocalProgress * frames->size() / clipDuration;
            if (frame >= (int)frames->size())
                frame = frames->size() - 1;
            int previousFrame = frame - 1;
            if (previousFrame < 0)
                previousFrame = frames->size() - 1;
            int nextFrame = frame + 1;
            if (nextFrame >= (int)frames->size())
                nextFrame = 0;
            if (frame >= 0 && frame < (int)frames->size()) {
                const JointNodeTree previousJointNodeTree = poseJointNodeTree(progressClip.linkToId, previousFrame);
                const JointNodeTree jointNodeTree = poseJointNodeTree(progressClip.linkToId, frame);
                const JointNodeTree nextJointNodeTree = poseJointNodeTree(progressClip.linkToId, nextFrame);
                const JointNodeTree middleJointNodeTree = generateInterpolation(InterpolationType::Linear, previousJointNodeTree, nextJointNodeTree, 0.5);
                outcomes.push_back({progress - lastProgress,
                    generateInterpolation(InterpolationType::Linear, jointNodeTree, middleJointNodeTree, 0.5)});
                lastProgress = progress;
            }
            progress += interval;
            continue;
        } else if (MotionClipType::Motion == progressClip.clipType) {
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

const JointNodeTree &MotionsGenerator::poseJointNodeTree(const QUuid &poseId, int frame)
{
    auto findResult = m_poseJointNodeTreeMap.find({poseId, frame});
    if (findResult != m_poseJointNodeTreeMap.end())
        return findResult->second;
    
    const auto &frames = m_poses[poseId];
    
    m_poser->reset();
    if (frame < (int)frames.size()) {
        const auto &parameters = frames[frame].second;
        m_poser->parameters() = parameters;
    }
    m_poser->commit();
    auto insertResult = m_poseJointNodeTreeMap.insert({{poseId, frame}, m_poser->resultJointNodeTree()});
    return insertResult.first->second;
}

const JointNodeTree *MotionsGenerator::findClipBeginJointNodeTree(const MotionClip &clip)
{
    if (MotionClipType::Pose == clip.clipType) {
        const JointNodeTree &jointNodeTree = poseJointNodeTree(clip.linkToId, 0);
        return &jointNodeTree;
    } else if (MotionClipType::Motion == clip.clipType) {
        const std::vector<MotionClip> *motionClips = findMotionClips(clip.linkToId);
        if (nullptr != motionClips && !motionClips->empty()) {
            return findClipBeginJointNodeTree((*motionClips)[0]);
        }
        return nullptr;
    }
    return nullptr;
}

const JointNodeTree *MotionsGenerator::findClipEndJointNodeTree(const MotionClip &clip)
{
    if (MotionClipType::Pose == clip.clipType) {
        const std::vector<std::pair<std::map<QString, QString>, std::map<QString, std::map<QString, QString>>>> *poseFrames = findPoseFrames(clip.linkToId);
        if (nullptr != poseFrames && !poseFrames->empty()) {
            return &poseJointNodeTree(clip.linkToId, poseFrames->size() - 1);
        }
        return nullptr;
    } else if (MotionClipType::Motion == clip.clipType) {
        const std::vector<MotionClip> *motionClips = findMotionClips(clip.linkToId);
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
    m_poser = newPoser(m_rigType, m_rigBones);
    if (nullptr == m_poser)
        return;
    
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
