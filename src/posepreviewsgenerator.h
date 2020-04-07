#ifndef DUST3D_POSE_PREVIEWS_GENERATOR_H
#define DUST3D_POSE_PREVIEWS_GENERATOR_H
#include <QObject>
#include <map>
#include <QUuid>
#include <vector>
#include "model.h"
#include "rigger.h"
#include "outcome.h"
#include "rigtype.h"

class PosePreviewsGenerator : public QObject
{
    Q_OBJECT
public:
    PosePreviewsGenerator(RigType rigType,
        const std::vector<RiggerBone> *rigBones,
        const std::map<int, RiggerVertexWeights> *rigWeights,
        const Outcome &outcome);
    ~PosePreviewsGenerator();
    void addPose(std::pair<QUuid, int> idAndFrame, const std::map<QString, std::map<QString, QString>> &pose);
    const std::set<std::pair<QUuid, int>> &generatedPreviewPoseIdAndFrames();
    Model *takePreview(std::pair<QUuid, int> idAndFrame);
signals:
    void finished();
public slots:
    void process();
private:
    RigType m_rigType = RigType::None;
    std::vector<RiggerBone> m_rigBones;
    std::map<int, RiggerVertexWeights> m_rigWeights;
    Outcome *m_outcome = nullptr;
    std::vector<std::pair<std::pair<QUuid, int>, std::map<QString, std::map<QString, QString>>>> m_poses;
    std::map<std::pair<QUuid, int>, Model *> m_previews;
    std::set<std::pair<QUuid, int>> m_generatedPoseIdAndFrames;
};

#endif
