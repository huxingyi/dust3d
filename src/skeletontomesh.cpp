#include "skeletontomesh.h"
#include "meshlite.h"
#include "skeletoneditnodeitem.h"
#include "skeletoneditedgeitem.h"

// Modified from https://wiki.qt.io/QThreads_general_usage

struct NodeItemInfo
{
    int index;
    int neighborCount;
};

SkeletonToMesh::SkeletonToMesh(SkeletonEditGraphicsView *graphicsView) :
    m_mesh(NULL), m_rootNodeId(0)
{
    QList<QGraphicsItem *>::iterator it;
    QList<QGraphicsItem *> list = graphicsView->scene()->items();
    std::map<SkeletonEditNodeItem *, NodeItemInfo> nodeItemsMap;
    int maxNeighborCount = 0;
    for (it = list.begin(); it != list.end(); ++it) {
        if ((*it)->data(0).toString() == "edge") {
            SkeletonEditEdgeItem *edgeItem = static_cast<SkeletonEditEdgeItem *>(*it);
            SkeletonEditNodeItem *nodeItems[] = {edgeItem->firstNode(), edgeItem->secondNode()};
            int nodeIndices[] = {0, 0};
            for (int i = 0; i < 2; i++) {
                SkeletonEditNodeItem *nodeItem = nodeItems[i];
                std::map<SkeletonEditNodeItem *, NodeItemInfo>::iterator findResult = nodeItemsMap.find(nodeItem);
                if (findResult == nodeItemsMap.end()) {
                    SkeletonNode node;
                    NodeItemInfo info;
                    QPointF origin = nodeItem->origin();
                    
                    node.originX = origin.x();
                    node.originY = origin.y();
                    node.originZ = nodeItem->slave()->origin().x();
                    node.bmeshNodeId = -1;
                    node.radius = nodeItem->radius();
                    
                    info.index = m_nodes.size();
                    info.neighborCount = 1;
                    
                    nodeIndices[i] = info.index;
                    nodeItemsMap[nodeItem] = info;
                    m_nodes.push_back(node);
                } else {
                    nodeIndices[i] = findResult->second.index;
                    findResult->second.neighborCount++;
                    if (findResult->second.neighborCount > maxNeighborCount) {
                        m_rootNodeId = findResult->second.index;
                        maxNeighborCount = findResult->second.neighborCount;
                    }
                }
            }
            SkeletonEdge edge;
            edge.firstNode = nodeIndices[0];
            edge.secondNode = nodeIndices[1];
            m_edges.push_back(edge);
        }
    }
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

void SkeletonToMesh::process()
{
    if (m_nodes.size() <= 1) {
        emit finished();
        return;
    }
    float left = -1;
    float right = -1;
    float top = -1;
    float bottom = -1;
    float zLeft = -1;
    float zRight = -1;
    for (size_t i = 0; i < m_nodes.size(); i++) {
        SkeletonNode *node = &m_nodes[i];
        if (left < 0 || node->originX < left) {
            left = node->originX;
        }
        if (top < 0 || node->originY < top) {
            top = node->originY;
        }
        if (node->originX > right) {
            right = node->originX;
        }
        if (node->originY > bottom) {
            bottom = node->originY;
        }
        if (zLeft < 0 || node->originZ < zLeft) {
            zLeft = node->originZ;
        }
        if (node->originZ > zRight) {
            zRight = node->originZ;
        }
    }
    float height = bottom - top;
    if (height <= 0) {
        emit finished();
        return;
    }
    float zWidth = right - left;
    void *context = meshlite_create_context();
    int bmesh = meshlite_bmesh_create(context);
    for (size_t i = 0; i < m_nodes.size(); i++) {
        SkeletonNode *node = &m_nodes[i];
        float x = (node->originX - left - height / 2) / height;
        float y = (node->originY - top - height / 2) / height;
        float z = (node->originZ - zLeft - zWidth / 2) / height;
        float r = node->radius / height;
        node->bmeshNodeId = meshlite_bmesh_add_node(context, bmesh, x, y, z, r);
    }
    for (size_t i = 0; i < m_edges.size(); i++) {
        SkeletonNode *firstNode = &m_nodes[m_edges[i].firstNode];
        SkeletonNode *secondNode = &m_nodes[m_edges[i].secondNode];
        meshlite_bmesh_add_edge(context, bmesh, firstNode->bmeshNodeId, secondNode->bmeshNodeId);
    }
    int mesh = meshlite_bmesh_generate_mesh(context, bmesh, m_nodes[m_rootNodeId].bmeshNodeId);
    meshlite_bmesh_destroy(context, bmesh);
    int triangulate = meshlite_triangulate(context, mesh);
    meshlite_export(context, triangulate, "/Users/jeremy/testlib.obj");
    m_mesh = new Mesh(context, triangulate);
    meshlite_destroy_context(context);
    emit finished();
}
