#ifndef DUST3D_MOTIONS_GENERATOR_H
#define DUST3D_MOTIONS_GENERATOR_H
#include <QObject>
#include <vector>
#include <map>
#include <set>
#include "model.h"
#include "simpleshadermesh.h"
#include "rigger.h"
#include "jointnodetree.h"
#include "document.h"

class MotionsGenerator : public QObject
{
    Q_OBJECT
public:
    MotionsGenerator(RigType rigType,
        const std::vector<RiggerBone> &bones,
        const std::map<int, RiggerVertexWeights> &rigWeights,
        const Object &object);
    ~MotionsGenerator();
    void addMotion(const QUuid &motionId, const std::map<QString, QString> &parameters);
    Model *takeResultSnapshotMesh(const QUuid &motionId);
    std::vector<std::pair<float, SimpleShaderMesh *>> takeResultPreviewMeshes(const QUuid &motionId);
    std::vector<std::pair<float, JointNodeTree>> takeResultJointNodeTrees(const QUuid &motionId);
    const std::set<QUuid> &generatedMotionIds();
    void enablePreviewMeshes();
    void enableSnapshotMeshes();
    void generate();
signals:
    void finished();
    
public slots:
    void process();
    
private:
    RigType m_rigType = RigType::None;
    std::vector<RiggerBone> m_bones;
    std::map<int, RiggerVertexWeights> m_rigWeights;
    Object m_object;
    std::map<QUuid, std::map<QString, QString>> m_motions;
    std::set<QUuid> m_generatedMotionIds;
    std::map<QUuid, Model *> m_resultSnapshotMeshes;
    std::map<QUuid, std::vector<std::pair<float, SimpleShaderMesh *>>> m_resultPreviewMeshes;
    std::map<QUuid, std::vector<std::pair<float, JointNodeTree>>> m_resultJointNodeTrees;
    bool m_previewMeshesEnabled = false;
    bool m_snapshotMeshesEnabled = false;
    
    void generateMotion(const QUuid &motionId);
};

#endif
