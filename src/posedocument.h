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
    void reset();
    
    void toParameters(std::map<QString, std::map<QString, QString>> &parameters, const std::set<QUuid> &limitNodeIds=std::set<QUuid>()) const;
    void fromParameters(const std::vector<RiggerBone> *rigBones,
        const std::map<QString, std::map<QString, QString>> &parameters);
    
public slots:
    void saveHistoryItem();
    void clearHistories();
    void undo() override;
    void redo() override;
    void paste() override;
    
    void moveNodeBy(QUuid nodeId, float x, float y, float z);
    void setNodeOrigin(QUuid nodeId, float x, float y, float z);
    void switchChainSide(const std::set<QUuid> nodeIds);
    
public:
    static const float m_nodeRadius;
    static const float m_groundPlaneHalfThickness;
    static const bool m_hideRootAndVirtual;
    static const float m_outcomeScaleFactor;
    
private:
    QString findBoneNameByNodeId(const QUuid &nodeId);
    float findFootBottomY() const;
    float findFirstSpineY() const;
    float findLegHeight() const;
    void parametersToNodes(const std::vector<RiggerBone> *rigBones,
        const float heightAboveGroundLevel,
        std::map<QString, std::pair<QUuid, QUuid>> *boneNameToIdsMap,
        //QUuid *groundPartId,
        QUuid *bonesPartId,
        //QUuid *groundEdgeId,
        bool isOther=false);
    void updateBonesAndHeightAboveGroundLevelFromParameters(std::vector<RiggerBone> *bones,
        float *heightAboveGroundLevel,
        const std::map<QString, std::map<QString, QString>> &parameters);

    std::map<QString, std::pair<QUuid, QUuid>> m_boneNameToIdsMap;
    //QUuid m_groundPartId;
    QUuid m_bonesPartId;
    //QUuid m_groundEdgeId;
    std::deque<PoseHistoryItem> m_undoItems;
    std::deque<PoseHistoryItem> m_redoItems;
    std::vector<RiggerBone> m_riggerBones;
    std::vector<std::map<QString, std::map<QString, QString>>> m_otherFramesParameters;
    std::set<QUuid> m_otherIds;
    
    static float fromOutcomeX(float x);
    static float toOutcomeX(float x);
    static float fromOutcomeY(float y);
    static float toOutcomeY(float y);
    static float fromOutcomeZ(float z);
    static float toOutcomeZ(float z);
};

#endif
