#ifndef DUST3D_MOTIONS_GENERATOR_H
#define DUST3D_MOTIONS_GENERATOR_H
#include <QObject>
#include <vector>
#include <map>
#include <set>
#include "model.h"
#include "rigger.h"
#include "jointnodetree.h"
#include "document.h"
#include "poser.h"

#define ENABLE_PROCEDURAL_DEBUG     1

class MotionsGenerator : public QObject
{
    Q_OBJECT
public:
    MotionsGenerator(RigType rigType,
        const std::vector<RiggerBone> *rigBones,
        const std::map<int, RiggerVertexWeights> *rigWeights,
        const Outcome &outcome);
    ~MotionsGenerator();
    void addPoseToLibrary(const QUuid &poseId, const std::vector<std::pair<std::map<QString, QString>, std::map<QString, std::map<QString, QString>>>> &frames, float yTranslationScale);
    void addMotionToLibrary(const QUuid &motionId, const std::vector<MotionClip> &clips);
    void addRequirement(const QUuid &motionId);
    std::vector<std::pair<float, Model *>> takeResultPreviewMeshs(const QUuid &motionId);
    std::vector<std::pair<float, JointNodeTree>> takeResultJointNodeTrees(const QUuid &motionId);
    const std::set<QUuid> &requiredMotionIds();
    const std::set<QUuid> &generatedMotionIds();
    void generate();
signals:
    void finished();
    
public slots:
    void process();
    
private:
    void generateMotion(const QUuid &motionId, std::set<QUuid> &visited, std::vector<std::pair<float, JointNodeTree>> &outcomes,
            std::vector<Model *> *previews=nullptr);
    const JointNodeTree &poseJointNodeTree(const QUuid &poseId, int frame);
    JointNodeTree generateInterpolation(InterpolationType interpolationType, const JointNodeTree &first, const JointNodeTree &second, float progress);
    const JointNodeTree *findClipBeginJointNodeTree(const MotionClip &clip);
    const JointNodeTree *findClipEndJointNodeTree(const MotionClip &clip);
    std::vector<MotionClip> *findMotionClips(const QUuid &motionId);
    std::vector<std::pair<std::map<QString, QString>, std::map<QString, std::map<QString, QString>>>> *findPoseFrames(const QUuid &poseId);
    void generatePreviewsForOutcomes(const std::vector<std::pair<float, JointNodeTree>> &outcomes, std::vector<std::pair<float, Model *>> &previews);
    float calculateMotionDuration(const QUuid &motionId, std::set<QUuid> &visited);
    float calculatePoseDuration(const QUuid &poseId);
    float calculateProceduralAnimationDuration(ProceduralAnimation proceduralAnimation,
        const JointNodeTree *initialJointNodeTree=nullptr);
    const std::vector<std::pair<float, JointNodeTree>> &getProceduralAnimation(ProceduralAnimation proceduralAnimation,
        const JointNodeTree *initialJointNodeTree=nullptr);
    
    RigType m_rigType = RigType::None;
    std::vector<RiggerBone> m_rigBones;
    std::map<int, RiggerVertexWeights> m_rigWeights;
    std::map<int, std::vector<std::pair<float, JointNodeTree>>> m_proceduralAnimations;
#if ENABLE_PROCEDURAL_DEBUG
    std::map<int, std::vector<Model *>> m_proceduralDebugPreviews;
#endif
    Outcome m_outcome;
    std::map<QUuid, std::vector<std::pair<std::map<QString, QString>, std::map<QString, std::map<QString, QString>>>>> m_poses;
    std::map<QUuid, float> m_posesYtranslationScales;
    std::map<QUuid, std::vector<MotionClip>> m_motions;
    std::set<QUuid> m_requiredMotionIds;
    std::set<QUuid> m_generatedMotionIds;
    std::map<QUuid, std::vector<std::pair<float, Model *>>> m_resultPreviewMeshs;
    std::map<QUuid, std::vector<std::pair<float, JointNodeTree>>> m_resultJointNodeTrees;
    std::map<std::pair<QUuid, int>, JointNodeTree> m_poseJointNodeTreeMap;
    Poser *m_poser = nullptr;
    int m_fps = 30;
};

#endif
