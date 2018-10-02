#ifndef MOTION_PREVIEWS_GENERATOR_H
#define MOTION_PREVIEWS_GENERATOR_H
#include <QObject>
#include <vector>
#include <map>
#include <set>
#include "meshloader.h"
#include "autorigger.h"
#include "curveutil.h"
#include "jointnodetree.h"

class MotionPreviewsGenerator : public QObject
{
    Q_OBJECT
public:
    MotionPreviewsGenerator(const std::vector<AutoRiggerBone> *rigBones,
        const std::map<int, AutoRiggerVertexWeights> *rigWeights,
        const MeshResultContext &meshResultContext);
    ~MotionPreviewsGenerator();
    void addPoseToLibrary(const QUuid &poseId, const std::map<QString, std::map<QString, QString>> &parameters);
    void addMotionToLibrary(const QUuid &motionId, const std::vector<HermiteControlNode> &controlNodes,
        const std::vector<std::pair<float, QUuid>> &keyframes);
    void addPreviewRequirement(const QUuid &motionId);
    std::vector<std::pair<float, MeshLoader *>> takeResultPreviewMeshs(const QUuid &motionId);
    const std::set<QUuid> &requiredMotionIds();
    const std::set<QUuid> &generatedMotionIds();
    void generateForMotion(const QUuid &motionId);
    void generate();
signals:
    void finished();
public slots:
    void process();
private:
    std::vector<AutoRiggerBone> m_rigBones;
    std::map<int, AutoRiggerVertexWeights> m_rigWeights;
    MeshResultContext m_meshResultContext;
    std::map<QUuid, std::map<QString, std::map<QString, QString>>> m_poses;
    std::map<QUuid, std::pair<std::vector<HermiteControlNode>, std::vector<std::pair<float, QUuid>>>> m_motions;
    std::set<QUuid> m_requiredMotionIds;
    std::set<QUuid> m_generatedMotionIds;
    std::map<QUuid, std::vector<std::pair<float, MeshLoader *>>> m_resultPreviewMeshs;
    int m_fps = 24;
};

#endif
