#ifndef SKINNED_MESH_H
#define SKINNED_MESH_H
#include <QVector3D>
#include <vector>
#include "meshresultcontext.h"
#include "rigcontroller.h"
#include "meshloader.h"

struct SkinnedMeshWeight
{
    int jointIndex;
    float amount;
};

struct SkinnedMeshVertex
{
    QVector3D position;
    QVector3D normal;
    QVector3D posPosition;
    QVector3D posNormal;
    SkinnedMeshWeight weights[MAX_WEIGHT_NUM];
};

struct SkinnedMeshTriangle
{
    int indicies[3];
    QColor color;
};

class SkinnedMesh
{
public:
    SkinnedMesh(const MeshResultContext &resultContext, const JointNodeTree &jointNodeTree);
    ~SkinnedMesh();
    void startRig();
    RigController *rigController();
    JointNodeTree *jointNodeTree();
    void applyRigFrameToMesh(const RigFrame &frame);
    MeshLoader *toMeshLoader();
private:
    void fromMeshResultContext(MeshResultContext &resultContext);
private:
    MeshResultContext m_resultContext;
    RigController *m_rigController;
    JointNodeTree *m_jointNodeTree;
private:
    Q_DISABLE_COPY(SkinnedMesh);
    std::vector<SkinnedMeshVertex> m_vertices;
    std::vector<SkinnedMeshTriangle> m_triangles;
};

#endif
