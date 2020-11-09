#include "blockmesh.h"
 
std::vector<size_t> BlockMesh::buildFace(const QVector3D &origin, 
        const QVector3D &faceNormal, 
        const QVector3D &startDirection, 
        double radius)
{
    std::vector<size_t> face;
    face.push_back(m_resultVertices->size() + 0);
    face.push_back(m_resultVertices->size() + 1);
    face.push_back(m_resultVertices->size() + 2);
    face.push_back(m_resultVertices->size() + 3);
    
    auto upDirection = QVector3D::crossProduct(startDirection, faceNormal);
    
    m_resultVertices->push_back(origin + startDirection * radius);
    m_resultVertices->push_back(origin - upDirection * radius);
    m_resultVertices->push_back(origin - startDirection * radius);
    m_resultVertices->push_back(origin + upDirection * radius);
    
    return face;
}

QVector3D BlockMesh::calculateStartDirection(const QVector3D &direction)
{
    const std::vector<QVector3D> axisList = {
        QVector3D {1, 0, 0},
        QVector3D {0, 1, 0},
        QVector3D {0, 0, 1},
    };
    float maxDot = -1;
    size_t nearAxisIndex = 0;
    bool reversed = false;
    for (size_t i = 0; i < axisList.size(); ++i) {
        const auto axis = axisList[i];
        auto dot = QVector3D::dotProduct(axis, direction);
        auto positiveDot = std::abs(dot);
        if (positiveDot >= maxDot) {
            reversed = dot < 0;
            maxDot = positiveDot;
            nearAxisIndex = i;
        }
    }
    const auto& choosenAxis = axisList[(nearAxisIndex + 1) % 3];
    auto startDirection = QVector3D::crossProduct(direction, choosenAxis).normalized();
    return reversed ? -startDirection : startDirection;
}
    
void BlockMesh::buildBlock(const Block &block)
{
    QVector3D fromFaceNormal = (block.toPosition - block.fromPosition).normalized();
    QVector3D startDirection = calculateStartDirection(-fromFaceNormal);
    std::vector<size_t> fromFaces = buildFace(block.fromPosition, 
        -fromFaceNormal, startDirection, block.fromRadius);
    std::vector<size_t> toFaces = buildFace(block.toPosition, 
        -fromFaceNormal, startDirection, block.toRadius);
    
    m_resultQuads->push_back(fromFaces);
    for (size_t i = 0; i < fromFaces.size(); ++i) {
        size_t j = (i + 1) % fromFaces.size();
        m_resultQuads->push_back({fromFaces[j], fromFaces[i], toFaces[i], toFaces[j]});
    }
    std::reverse(toFaces.begin(), toFaces.end());
    m_resultQuads->push_back(toFaces);
}
    
void BlockMesh::build()
{
    delete m_resultVertices;
    m_resultVertices = new std::vector<QVector3D>;
    
    delete m_resultQuads;
    m_resultQuads = new std::vector<std::vector<size_t>>;
    
    for (const auto &block: m_blocks)
        buildBlock(block);
    
    delete m_resultFaces;
    m_resultFaces = new std::vector<std::vector<size_t>>;
    m_resultFaces->reserve(m_resultQuads->size() * 2);
    for (const auto &quad: *m_resultQuads) {
        m_resultFaces->push_back({quad[0], quad[1], quad[2]});
        m_resultFaces->push_back({quad[2], quad[3], quad[0]});
    }
}

