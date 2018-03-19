#ifndef SKELETON_TO_MESH_H
#define SKELETON_TO_MESH_H
#include <QObject>
#include <QList>
#include <vector>
#include <map>
#include "skeletoneditgraphicsview.h"
#include "mesh.h"

struct SkeletonNode
{
    float originX;
    float originY;
    float originZ;
    float radius;
    float thickness;
    int bmeshNodeId;
};

struct SkeletonEdge
{
    int firstNode;
    int secondNode;
};

struct SkeletonGroup
{
    std::vector<SkeletonNode> nodes;
    std::vector<SkeletonEdge> edges;
    int rootNode;
    int maxNeighborCount;
    int bmeshId;
    int meshId;
};

class SkeletonToMesh : public QObject 
{
    Q_OBJECT
public:
    SkeletonToMesh(SkeletonEditGraphicsView *graphicsView);
    ~SkeletonToMesh();
    Mesh *takeResultMesh();
signals:
    void finished();
public slots:
    void process();
private:
    Mesh *m_mesh;
    std::vector<SkeletonGroup> m_groups;
};

#endif
