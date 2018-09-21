#ifndef POSE_PREVIEWS_GENERATOR_H
#define POSE_PREVIEWS_GENERATOR_H
#include <QObject>
#include <map>
#include <QUuid>
#include <vector>
#include "meshloader.h"
#include "autorigger.h"
#include "meshresultcontext.h"

class PosePreviewsGenerator : public QObject
{
    Q_OBJECT
public:
    PosePreviewsGenerator(const std::vector<AutoRiggerBone> *rigBones,
        const std::map<int, AutoRiggerVertexWeights> *rigWeights,
        const MeshResultContext &meshResultContext);
    ~PosePreviewsGenerator();
    void addPose(QUuid poseId, const std::map<QString, std::map<QString, QString>> &pose);
    const std::set<QUuid> &generatedPreviewPoseIds();
    MeshLoader *takePreview(QUuid poseId);
signals:
    void finished();
public slots:
    void process();
private:
    std::vector<AutoRiggerBone> m_rigBones;
    std::map<int, AutoRiggerVertexWeights> m_rigWeights;
    MeshResultContext *m_meshResultContext = nullptr;
    std::vector<std::pair<QUuid, std::map<QString, std::map<QString, QString>>>> m_poses;
    std::map<QUuid, MeshLoader *> m_previews;
    std::set<QUuid> m_generatedPoseIds;
};

#endif
