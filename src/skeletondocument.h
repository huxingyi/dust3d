#ifndef SKELETON_DOCUMENT_H
#define SKELETON_DOCUMENT_H
#include <QObject>
#include <QUuid>
#include <vector>
#include <map>
#include <set>
#include <QImage>
#include "skeletonsnapshot.h"
#include "mesh.h"
#include "meshgenerator.h"

enum class SkeletonNodeRootMarkMode
{
    Auto = 0,
    MarkAsRoot,
    MarkAsNotRoot
};

const char *SkeletonNodeRootMarkModeToString(SkeletonNodeRootMarkMode mode);
SkeletonNodeRootMarkMode SkeletonNodeRootMarkModeFromString(const QString &mode);

class SkeletonNode
{
public:
    SkeletonNode(const QUuid &withId=QUuid()) :
        x(0),
        y(0),
        z(0),
        radius(0),
        rootMarkMode(SkeletonNodeRootMarkMode::Auto)
    {
        id = withId.isNull() ? QUuid::createUuid() : withId;
    }
    QUuid id;
    QUuid partId;
    QString name;
    float x;
    float y;
    float z;
    float radius;
    SkeletonNodeRootMarkMode rootMarkMode;
    std::vector<QUuid> edgeIds;
};

enum class SkeletonEdgeBranchMode
{
    Auto = 0,
    Trivial,
    NoTrivial
};

const char *SkeletonEdgeBranchModeToString(SkeletonEdgeBranchMode mode);
SkeletonEdgeBranchMode SkeletonEdgeBranchModeFromString(const QString &mode);

class SkeletonEdge
{
public:
    SkeletonEdge(const QUuid &withId=QUuid()) :
        branchMode(SkeletonEdgeBranchMode::Auto)
    {
        id = withId.isNull() ? QUuid::createUuid() : withId;
    }
    QUuid id;
    QUuid partId;
    QString name;
    SkeletonEdgeBranchMode branchMode;
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
    QImage preview;
    std::vector<QUuid> nodeIds;
    bool previewIsObsolete;
    SkeletonPart(const QUuid &withId=QUuid()) :
        visible(true),
        previewIsObsolete(true)
    {
        id = withId.isNull() ? QUuid::createUuid() : withId;
    }
};

class SkeletonHistoryItem
{
public:
    SkeletonSnapshot snapshot;
    QImage preview;
};

enum class SkeletonDocumentEditMode
{
    Add = 0,
    Select,
    Drag,
    ZoomIn,
    ZoomOut
};

enum class SkeletonProfile
{
    Unknown = 0,
    Main,
    Side
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
public:
    SkeletonDocument();
    ~SkeletonDocument();
    std::map<QUuid, SkeletonPart> partMap;
    std::map<QUuid, SkeletonNode> nodeMap;
    std::map<QUuid, SkeletonEdge> edgeMap;
    std::vector<SkeletonHistoryItem> historyItems;
    std::vector<QUuid> partIds;
    QImage turnaround;
    SkeletonDocumentEditMode editMode;
    void toSnapshot(SkeletonSnapshot *snapshot);
    void fromSnapshot(const SkeletonSnapshot &snapshot);
    const SkeletonNode *findNode(QUuid nodeId) const;
    const SkeletonEdge *findEdge(QUuid edgeId) const;
    const SkeletonPart *findPart(QUuid partId) const;
    const SkeletonEdge *findEdgeByNodes(QUuid firstNodeId, QUuid secondNodeId) const;
    Mesh *takeResultMesh();
    QImage preview;
    void updateTurnaround(const QImage &image);
public slots:
    void removeNode(QUuid nodeId);
    void removeEdge(QUuid edgeId);
    void removePart(QUuid partId);
    void showPart(QUuid partId);
    void hidePart(QUuid partId);
    void addNode(float x, float y, float z, float radius, QUuid fromNodeId);
    void scaleNodeByAddRadius(QUuid nodeId, float amount);
    void moveNodeBy(QUuid nodeId, float x, float y, float z);
    void setNodeOrigin(QUuid nodeId, float x, float y, float z);
    void setNodeRadius(QUuid nodeId, float radius);
    void addEdge(QUuid fromNodeId, QUuid toNodeId);
    void setEditMode(SkeletonDocumentEditMode mode);
    void setEdgeBranchMode(QUuid edgeId, SkeletonEdgeBranchMode mode);
    void setNodeRootMarkMode(QUuid nodeId, SkeletonNodeRootMarkMode mode);
    void uiReady();
    void generateMesh();
    void meshReady();
private:
    void splitPartByNode(std::vector<std::vector<QUuid>> *groups, QUuid nodeId);
    void joinNodeAndNeiborsToGroup(std::vector<QUuid> *group, QUuid nodeId, std::set<QUuid> *visitMap, QUuid noUseEdgeId=QUuid());
    void splitPartByEdge(std::vector<std::vector<QUuid>> *groups, QUuid edgeId);
private:
    bool m_resultMeshIsObsolete;
    MeshGenerator *m_meshGenerator;
    Mesh *m_resultMesh;
};

#endif
