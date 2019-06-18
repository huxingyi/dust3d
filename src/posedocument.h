#ifndef DUST3D_POSE_DOCUMENT_H
#define DUST3D_POSE_DOCUMENT_H
#include <map>
#include <QString>
#include <deque>
#include "skeletondocument.h"
#include "rigger.h"

struct PoseHistoryItem
{
    std::map<QString, std::map<QString, QString>> parameters;
};

class PoseDocument : public SkeletonDocument
{
    Q_OBJECT
signals:
    void turnaroundChanged();
    void cleanup();
    void nodeAdded(QUuid nodeId);
    void edgeAdded(QUuid edgeId);
    void nodeOriginChanged(QUuid nodeId);
    void parametersChanged();
    void sideVisibleStateChanged(SkeletonSide side);
    void partVisibleStateChanged(QUuid partId);
    
public:
    bool undoable() const override;
    bool redoable() const override;
    bool hasPastableNodesInClipboard() const override;
    bool originSettled() const override;
    bool isNodeEditable(QUuid nodeId) const override;
    bool isEdgeEditable(QUuid edgeId) const override;
    bool isNodeDeactivated(QUuid nodeId) const override;
    bool isEdgeDeactivated(QUuid edgeId) const override;
    void copyNodes(std::set<QUuid> nodeIdSet) const override;
    
    void updateTurnaround(const QImage &image);
    void updateOtherFramesParameters(const std::vector<std::map<QString, std::map<QString, QString>>> &otherFramesParameters);
    void resetWithoutNotifingParametersChanged();
    void reset();
    
    void toParameters(std::map<QString, std::map<QString, QString>> &parameters, const std::set<QUuid> &limitNodeIds=std::set<QUuid>()) const;
    void fromParameters(const std::vector<RiggerBone> *rigBones,
        const std::map<QString, std::map<QString, QString>> &parameters);
    
    bool isSideVisible(SkeletonSide side);
    
public slots:
    void saveHistoryItem();
    void clearHistories();
    void undo() override;
    void redo() override;
    void paste() override;
    
    void moveNodeBy(QUuid nodeId, float x, float y, float z);
    void setNodeOrigin(QUuid nodeId, float x, float y, float z);
    void switchChainSide(const std::set<QUuid> nodeIds);
    void setSideVisiableState(SkeletonSide side, bool visible);
    
public:
    static const float m_nodeRadius;
    static const float m_groundPlaneHalfThickness;
    static const bool m_hideRootAndVirtual;
    static const float m_outcomeScaleFactor;
    
private:
    QString findBoneNameByNodeId(const QUuid &nodeId);
    float findFootBottomY() const;
    void parametersToNodes(const std::vector<RiggerBone> *rigBones,
        std::map<QString, std::pair<QUuid, QUuid>> *boneNameToIdsMap,
        std::map<SkeletonSide, QUuid> *m_partIdMap,
        bool isOther=false);
    void updateBonesFromParameters(std::vector<RiggerBone> *bones,
        const std::map<QString, std::map<QString, QString>> &parameters,
        float firstSpineBoneLength,
        const QVector3D &firstSpineBonePosition,
        const QVector3D &neckJoint1BoneDirection);

    std::map<QString, std::pair<QUuid, QUuid>> m_boneNameToIdsMap;
    std::map<SkeletonSide, QUuid> m_partIdMap;
    std::deque<PoseHistoryItem> m_undoItems;
    std::deque<PoseHistoryItem> m_redoItems;
    std::vector<RiggerBone> m_riggerBones;
    std::vector<std::map<QString, std::map<QString, QString>>> m_otherFramesParameters;
    std::set<QUuid> m_otherIds;
    std::set<SkeletonSide> m_hiddenSides;
    
    static float fromOutcomeX(float x);
    static float toOutcomeX(float x);
    static float fromOutcomeY(float y);
    static float toOutcomeY(float y);
    static float fromOutcomeZ(float z);
    static float toOutcomeZ(float z);
    static void firstSpinePositionAndLengthFromParameters(const std::map<QString, std::map<QString, QString>> &parameters,
        float *length, QVector3D *position);
    static void neckJoint1DirectionFromParameters(const std::map<QString, std::map<QString, QString>> &parameters,
        QVector3D *direction);
};

#endif
