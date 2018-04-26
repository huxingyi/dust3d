#include <set>
#include "vertexskingenerator.h"
#include "positionmap.h"

float VertexSkinGenerator::m_positionMapGridSize = 0.01;

VertexSkinGenerator::VertexSkinGenerator(MeshResultContext *meshResultContext) :
    m_meshResultContext(meshResultContext)
{
}

void VertexSkinGenerator::process()
{
    /*
    PositionMap resultPositionMap(m_positionMapGridSize);
    for (auto i = 0u; i < m_meshResultContext->resultVertices.size(); i++) {
        ResultVertex *resultVertex = &m_meshResultContext->resultVertices[i];
        resultPositionMap.addPosition(resultVertex->position.x(), resultVertex->position.y(), resultVertex->position.z(), i);
    }
    */
    
    /*
    std::set<std::pair<int, int>> validNodes;
    for (const auto &it: m_meshResultContext->bmeshVertices) {
        if (!resultPositionMap.find(it.position.x(), it.position.y(), it.position.z()))
            continue;
        validNodes.insert(std::make_pair(it.bmeshId, it.nodeId));
    }*/
}
