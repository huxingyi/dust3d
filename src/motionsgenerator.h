#ifndef DUST3D_MOTIONS_GENERATOR_H
#define DUST3D_MOTIONS_GENERATOR_H
#include <QObject>
#include <vector>
#include <map>
#include <set>
#include "meshloader.h"
#include "rigger.h"
#include "jointnodetree.h"
#include "document.h"
#include "poser.h"

class MotionsGenerator : public QObject
{
    Q_OBJECT
public:
    MotionsGenerator(RigType rigType,
        const std::vector<RiggerBone> *rigBones,
        const std::map<int, RiggerVertexWeights> *rigWeights,
        const Outcome &outcome);
    ~MotionsGenerator();
    void addPoseToLibrary(const QUuid &poseId, const std::map<QString, std::map<QString, QString>> &parameters);
    void addMotionToLibrary(const QUuid &motionId, const std::vector<MotionClip> &clips);
    void addRequirement(const QUuid &motionId);
    std::vector<std::pair<float, MeshLoader *>> takeResultPreviewMeshs(const QUuid &motionId);
    std::vector<std::pair<float, JointNodeTree>> takeResultJointNodeTrees(const QUuid &motionId);
    const std::set<QUuid> &requiredMotionIds();
    const std::set<QUuid> &generatedMotionIds();
    void generate();
signals:
    void finished();
    
public slots:
    void process();
    
private:
    void generateMotion(const QUuid &motionId, std::set<QUuid> &visited, std::vector<std::pair<float, JointNodeTree>> &outcomes);
    const JointNodeTree &poseJointNodeTree(const QUuid &poseId);
    JointNodeTree generateInterpolation(InterpolationType interpolationType, const JointNodeTree &first, const JointNodeTree &second, float progress);
    const JointNodeTree *findClipBeginJointNodeTree(const MotionClip &clip);
    const JointNodeTree *findClipEndJointNodeTree(const MotionClip &clip);
    std::vector<MotionClip> *findMotionClips(const QUuid &motionId);
    void generatePreviewsForOutcomes(const std::vector<std::pair<float, JointNodeTree>> &outcomes, std::vector<std::pair<float, MeshLoader *>> &previews);
    float calculateMotionDuration(const QUuid &motionId, std::set<QUuid> &visited);
    
    RigType m_rigType = RigType::None;
    std::vector<RiggerBone> m_rigBones;
    std::map<int, RiggerVertexWeights> m_rigWeights;
    Outcome m_outcome;
    std::map<QUuid, std::map<QString, std::map<QString, QString>>> m_poses;
    std::map<QUuid, std::vector<MotionClip>> m_motions;
    std::set<QUuid> m_requiredMotionIds;
    std::set<QUuid> m_generatedMotionIds;
    std::map<QUuid, std::vector<std::pair<float, MeshLoader *>>> m_resultPreviewMeshs;
    std::map<QUuid, std::vector<std::pair<float, JointNodeTree>>> m_resultJointNodeTrees;
    std::map<QUuid, JointNodeTree> m_poseJointNodeTreeMap;
    Poser *m_poser = nullptr;
    int m_fps = 30;
};

#endif
