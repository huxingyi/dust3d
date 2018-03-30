#include "skeletontomesh.h"
#include "meshlite.h"
#include "skeletoneditnodeitem.h"
#include "skeletoneditedgeitem.h"
#include "skeletonsnapshot.h"
#include "unionmesh.h"
#include <vector>

struct NodeItemInfo
{
    int index;
    int neighborCount;
};

SkeletonToMesh::SkeletonToMesh(SkeletonSnapshot *snapshot) :
    m_mesh(NULL),
    m_snapshot(*snapshot)
{
}

SkeletonToMesh::~SkeletonToMesh()
{
    delete m_mesh;
}

Mesh *SkeletonToMesh::takeResultMesh()
{
    Mesh *mesh = m_mesh;
    m_mesh = NULL;
    return mesh;
}

#if USE_CARVE == 1
#define ExternalMesh                    carve::poly::Polyhedron
#define makeExternalMeshFromMeshlite    makeCarveMeshFromMeshlite
#define unionExternalMeshs              unionCarveMeshs
#define makeMeshliteMeshFromExternal    makeMeshliteMeshFromCarve
#endif

#if USE_CGAL == 1
#define ExternalMesh                    CgalMesh
#define makeExternalMeshFromMeshlite    makeCgalMeshFromMeshlite
#define unionExternalMeshs              unionCgalMeshs
#define makeMeshliteMeshFromExternal    makeMeshliteMeshFromCgal
#endif

void SkeletonToMesh::process()
{
    std::vector<SkeletonSnapshot> groups;
    QRectF globalFront = m_snapshot.boundingBoxFront();
    QRectF globalSide = m_snapshot.boundingBoxSide();
    QString rootNodeId = m_snapshot.rootNode();
    printf("rootNodeId:%s\n", rootNodeId.toUtf8().constData());
    float frontMiddleX = m_snapshot.nodes[rootNodeId]["x"].toFloat();
    float frontMiddleY = m_snapshot.nodes[rootNodeId]["y"].toFloat();
    float sideMiddleX = m_snapshot.nodes[m_snapshot.nodes[rootNodeId]["nextSidePair"]]["x"].toFloat();
    bool combineEnabled = "true" == m_snapshot.canvas["combine"];
    bool unionEnabled = "true" == m_snapshot.canvas["union"];
    bool subdivEnabled = "true" == m_snapshot.canvas["subdiv"];
    m_snapshot.splitByConnectivity(&groups);
    
    std::vector<int> meshIds;
    void *meshliteContext = meshlite_create_context();
    for (size_t i = 0; i < groups.size(); i++) {
        SkeletonSnapshot *skeleton = &groups[i];
        //QRectF front = skeleton->boundingBoxFront();
        //QRectF side = skeleton->boundingBoxSide();
        //float canvasWidth = skeleton->canvas["width"].toFloat();
        //float canvasHeight = skeleton->canvas["height"].toFloat();
        float canvasHeight = globalFront.height();
        
        std::map<QString, int> bmeshNodeMap;
        
        std::map<QString, std::map<QString, QString>>::iterator nodeIterator;
        int bmeshId = meshlite_bmesh_create(meshliteContext);
        for (nodeIterator = skeleton->nodes.begin(); nodeIterator != skeleton->nodes.end(); nodeIterator++) {
            if ("red" != nodeIterator->second["sideColorName"])
                continue;
            std::map<QString, std::map<QString, QString>>::iterator nextSidePair = skeleton->nodes.find(nodeIterator->second["nextSidePair"]);
            if (nextSidePair == skeleton->nodes.end())
                continue;
            float x = (nodeIterator->second["x"].toFloat() - frontMiddleX) / canvasHeight;
            float y = (nodeIterator->second["y"].toFloat() - frontMiddleY) / canvasHeight;
            float z = (nextSidePair->second["x"].toFloat() - sideMiddleX) / canvasHeight;
            float r = nodeIterator->second["radius"].toFloat() / canvasHeight;
            float t = nextSidePair->second["radius"].toFloat() / canvasHeight;
            int bmeshNodeId = meshlite_bmesh_add_node(meshliteContext, bmeshId, x, y, z, r, t);
            printf("meshlite_bmesh_add_node x:%f y:%f z:%f r:%f t:%f nodeName:%s bmeshNodeId:%d\n", x, y, z, r, t, nodeIterator->first.toUtf8().constData(), bmeshNodeId);
            bmeshNodeMap[nodeIterator->first] = bmeshNodeId;
        }
        
        std::map<QString, std::map<QString, QString>>::iterator edgeIterator;
        for (edgeIterator = skeleton->edges.begin(); edgeIterator != skeleton->edges.end(); edgeIterator++) {
            std::map<QString, int>::iterator from = bmeshNodeMap.find(edgeIterator->second["from"]);
            if (from == bmeshNodeMap.end())
                continue;
            std::map<QString, int>::iterator to = bmeshNodeMap.find(edgeIterator->second["to"]);
            if (to == bmeshNodeMap.end())
                continue;
            printf("meshlite_bmesh_add_edge %d -> %d\n", from->second, to->second);
            meshlite_bmesh_add_edge(meshliteContext, bmeshId, from->second, to->second);
        }
        
        if (bmeshNodeMap.size() > 0 && !skeleton->rootNode().isEmpty()) {
            int meshId = meshlite_bmesh_generate_mesh(meshliteContext, bmeshId, bmeshNodeMap[skeleton->rootNode()]);
            meshIds.push_back(meshId);
        }
        
        meshlite_bmesh_destroy(meshliteContext, bmeshId);
    }

    if (meshIds.size() > 0) {
        int mergedMeshId = 0;
        if (unionEnabled) {
            mergedMeshId = unionMeshs(meshliteContext, meshIds);
        } else {
            mergedMeshId = mergeMeshs(meshliteContext, meshIds);
        }
        if (subdivEnabled) {
            if (mergedMeshId > 0) {
                mergedMeshId = meshlite_subdivide(meshliteContext, mergedMeshId);
            }
        }
        if (mergedMeshId > 0) {
            if (combineEnabled) {
                mergedMeshId = meshlite_combine_adj_faces(meshliteContext, mergedMeshId);
            }
            m_mesh = new Mesh(meshliteContext, mergedMeshId);
        }
    }
    
    meshlite_destroy_context(meshliteContext);
    emit finished();
}
