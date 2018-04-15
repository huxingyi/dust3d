#ifndef SKELETON_DOCUMENT_H
#define SKELETON_DOCUMENT_H
#include <QObject>
#include <QUuid>
#include <vector>
#include <map>
#include <set>
#include <deque>
#include <QImage>
#include "skeletonsnapshot.h"
#include "mesh.h"
#include "meshgenerator.h"

class SkeletonNode
{
public:
    SkeletonNode(const QUuid &withId=QUuid()) :
        x(0),
        y(0),
        z(0),
        radius(0)
    {
        id = withId.isNull() ? QUuid::createUuid() : withId;
    }
    void setRadius(float toRadius)
    {
        if (toRadius < 0.01)
            toRadius = 0.01;
        else if (toRadius > 1)
            toRadius = 1;
        radius = toRadius;
    }
    QUuid id;
    QUuid partId;
    QString name;
    float x;
    float y;
    float z;
    float radius;
    std::vector<QUuid> edgeIds;
};

class SkeletonEdge
{
public:
    SkeletonEdge(const QUuid &withId=QUuid())
    {
        id = withId.isNull() ? QUuid::createUuid() : withId;
    }
    QUuid id;
    QUuid partId;
    QString name;
    std::vector<QUuid> nodeIds;
    QUuid neighborOf(QUuid nodeId) const
    {
        if (nodeIds.size() != 2)
            return QUuid();
        return nodeIds[0] == nodeId ? nodeIds[1] : nodeIds[0];
    }
};

class SkeletonPart
{
public:
    QUuid id;
    QString name;
    bool visible;
    bool locked;
    bool subdived;
    bool disabled;
    bool xMirrored;
    bool zMirrored;
    QImage preview;
    std::vector<QUuid> nodeIds;
    SkeletonPart(const QUuid &withId=QUuid()) :
        visible(true),
        locked(false),
        subdived(false),
        disabled(false),
        xMirrored(false),
        zMirrored(false)
    {
        id = withId.isNull() ? QUuid::createUuid() : withId;
    }
    void copyAttributes(const SkeletonPart &other)
    {
        visible = other.visible;
        locked = other.locked;
        subdived = other.subdived;
        disabled = other.disabled;
        xMirrored = other.xMirrored;
        zMirrored = other.zMirrored;
    }
};

enum class SkeletonProfile
{
    Unknown = 0,
    Main,
    Side
};

class SkeletonHistoryItem
{
public:
    SkeletonSnapshot snapshot;
};

enum class SkeletonDocumentEditMode
{
    Add = 0,
    Select,
    Drag,
    ZoomIn,
    ZoomOut
};

class SkeletonDocument : public QObject
{
    Q_OBJECT
signals:
    void partAdded(QUuid partId);
    void nodeAdded(QUuid nodeId);
    void edgeAdded(QUuid edgeId);
    void partRemoved(QUuid partId);
    void partListChanged();
    void nodeRemoved(QUuid nodeId);
    void edgeRemoved(QUuid edgeId);
    void nodeRadiusChanged(QUuid nodeId);
    void nodeOriginChanged(QUuid nodeId);
    void edgeChanged(QUuid edgeId);
    void partChanged(QUuid partId);
    void partPreviewChanged(QUuid partId);
    void resultMeshChanged();
    void turnaroundChanged();
    void editModeChanged();
    void skeletonChanged();
    void partLockStateChanged(QUuid partId);
    void partVisibleStateChanged(QUuid partId);
    void partSubdivStateChanged(QUuid partId);
    void partDisableStateChanged(QUuid partId);
    void partXmirrorStateChanged(QUuid partId);
    void partZmirrorStateChanged(QUuid partId);
    void cleanup();
    void originChanged();
public: // need initialize
    float originX;
    float originY;
    float originZ;
    SkeletonDocumentEditMode editMode;
public:
    SkeletonDocument();
    ~SkeletonDocument();
    std::map<QUuid, SkeletonPart> partMap;
    std::map<QUuid, SkeletonNode> nodeMap;
    std::map<QUuid, SkeletonEdge> edgeMap;
    std::vector<QUuid> partIds;
    QImage turnaround;
    QImage preview;
    void toSnapshot(SkeletonSnapshot *snapshot, const std::set<QUuid> &limitNodeIds=std::set<QUuid>()) const;
    void fromSnapshot(const SkeletonSnapshot &snapshot);
    void addFromSnapshot(const SkeletonSnapshot &snapshot);
    const SkeletonNode *findNode(QUuid nodeId) const;
    const SkeletonEdge *findEdge(QUuid edgeId) const;
    const SkeletonPart *findPart(QUuid partId) const;
    const SkeletonEdge *findEdgeByNodes(QUuid firstNodeId, QUuid secondNodeId) const;
    Mesh *takeResultMesh();
    void updateTurnaround(const QImage &image);
    bool hasPastableContentInClipboard() const;
    bool undoable() const;
    bool redoable() const;
    bool isNodeEditable(QUuid nodeId) const;
    bool isEdgeEditable(QUuid edgeId) const;
    bool originSettled() const;
public slots:
    void removeNode(QUuid nodeId);
    void removeEdge(QUuid edgeId);
    void removePart(QUuid partId);
    void addNode(float x, float y, float z, float radius, QUuid fromNodeId);
    void scaleNodeByAddRadius(QUuid nodeId, float amount);
    void moveNodeBy(QUuid nodeId, float x, float y, float z);
    void setNodeOrigin(QUuid nodeId, float x, float y, float z);
    void setNodeRadius(QUuid nodeId, float radius);
    void moveOriginBy(float x, float y, float z);
    void addEdge(QUuid fromNodeId, QUuid toNodeId);
    void setEditMode(SkeletonDocumentEditMode mode);
    void uiReady();
    void generateMesh();
    void meshReady();
    void setPartLockState(QUuid partId, bool locked);
    void setPartVisibleState(QUuid partId, bool visible);
    void setPartSubdivState(QUuid partId, bool subdived);
    void setPartDisableState(QUuid partId, bool disabled);
    void setPartXmirrorState(QUuid partId, bool mirrored);
    void setPartZmirrorState(QUuid partId, bool mirrored);
    void saveSnapshot();
    void undo();
    void redo();
    void paste();
    void batchChangeBegin();
    void batchChangeEnd();
    void reset();
    void breakEdge(QUuid edgeId);
private:
    void splitPartByNode(std::vector<std::vector<QUuid>> *groups, QUuid nodeId);
    void joinNodeAndNeiborsToGroup(std::vector<QUuid> *group, QUuid nodeId, std::set<QUuid> *visitMap, QUuid noUseEdgeId=QUuid());
    void splitPartByEdge(std::vector<std::vector<QUuid>> *groups, QUuid edgeId);
    bool isPartReadonly(QUuid partId) const;
    QUuid createNode(float x, float y, float z, float radius, QUuid fromNodeId);
    void settleOrigin();
private: // need initialize
    bool m_resultMeshIsObsolete;
    MeshGenerator *m_meshGenerator;
    Mesh *m_resultMesh;
    int m_batchChangeRefCount;
private:
    static unsigned long m_maxSnapshot;
    std::deque<SkeletonHistoryItem> m_undoItems;
    std::deque<SkeletonHistoryItem> m_redoItems;
};

#endif
