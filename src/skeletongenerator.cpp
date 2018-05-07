#include <set>
#include <QGuiApplication>
#include <QDebug>
#include <vector>
#include "skeletongenerator.h"
#include "positionmap.h"
#include "meshlite.h"

SkeletonGenerator::SkeletonGenerator(const MeshResultContext &meshResultContext) :
    m_resultSkeletonMesh(nullptr)
{
    m_meshResultContext = new MeshResultContext;
    *m_meshResultContext = meshResultContext;
}

SkeletonGenerator::~SkeletonGenerator()
{
    delete m_resultSkeletonMesh;
    delete m_meshResultContext;
}

MeshLoader *SkeletonGenerator::takeResultSkeletonMesh()
{
    MeshLoader *resultSkeletonMesh = m_resultSkeletonMesh;
    m_resultSkeletonMesh = nullptr;
    return resultSkeletonMesh;
}

MeshResultContext *SkeletonGenerator::takeResultContext()
{
    MeshResultContext *resultContext = m_meshResultContext;
    m_meshResultContext = nullptr;
    return resultContext;
}

MeshLoader *SkeletonGenerator::createSkeletonMesh()
{
    void *meshliteContext = meshlite_create_context();
    int sklt = meshlite_skeletonmesh_create(meshliteContext);
    
    std::map<std::pair<int, int>, BmeshNode *> nodeIndexMap;
    for (auto i = 0u; i < m_meshResultContext->bmeshNodes.size(); i++) {
        BmeshNode *bmeshNode = &m_meshResultContext->bmeshNodes[i];
        nodeIndexMap[std::make_pair(bmeshNode->bmeshId, bmeshNode->nodeId)] = bmeshNode;
    }
    
    for (const auto &it: m_meshResultContext->bmeshEdges) {
        const auto &from = nodeIndexMap.find(std::make_pair(it.fromBmeshId, it.fromNodeId));
        const auto &to = nodeIndexMap.find(std::make_pair(it.toBmeshId, it.toNodeId));
        if (from == nodeIndexMap.end()) {
            qDebug() << "fromNodeId not found in nodeIndexMap:" << it.fromBmeshId << it.fromNodeId;
            continue;
        }
        if (to == nodeIndexMap.end()) {
            qDebug() << "toNodeId not found in nodeIndexMap:" << it.toBmeshId << it.toNodeId;
            continue;
        }
        BmeshNode *fromNode = from->second;
        BmeshNode *toNode = to->second;
        meshlite_skeletonmesh_add_bone(meshliteContext, sklt,
            fromNode->origin.x(), fromNode->origin.y(), fromNode->origin.z(),
            toNode->origin.x(), toNode->origin.y(), toNode->origin.z());
    }
    
    int meshId = meshlite_skeletonmesh_generate_mesh(meshliteContext, sklt);
    MeshLoader *skeletonMesh = new MeshLoader(meshliteContext, meshId, -1, Theme::green);
    
    meshlite_destroy_context(meshliteContext);
    
    return skeletonMesh;
}

struct BmeshNodeDistWithWorldCenter
{
    BmeshNode *bmeshNode;
    float dist2;
};

void SkeletonGenerator::process()
{
    m_resultSkeletonMesh = createSkeletonMesh();
    
    this->moveToThread(QGuiApplication::instance()->thread());
    emit finished();
}
