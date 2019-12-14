#ifndef DUST3D_OUTCOME_H
#define DUST3D_OUTCOME_H
#include <vector>
#include <set>
#include <QVector3D>
#include <QUuid>
#include <QColor>
#include <QVector2D>
#include <QRectF>
#include "bonemark.h"

#define MAX_WEIGHT_NUM  4

struct OutcomeNode
{
    QUuid partId;
    QUuid nodeId;
    QVector3D origin;
    float radius = 0;
    QColor color;
    float colorSolubility = 0;
    QUuid materialId;
    bool countershaded = false;
    QUuid mirrorFromPartId;
    QUuid mirroredByPartId;
    BoneMark boneMark;
    QVector3D direction;
};

struct OutcomePaintNode
{
    int originNodeIndex;
    QUuid originNodeId;
    QVector3D origin;
    float radius = 0;
    QVector3D baseNormal;
    QVector3D direction;
    size_t order;
    std::vector<QVector3D> vertices;
};

struct OutcomePaintMap
{
    QUuid partId;
    std::vector<OutcomePaintNode> paintNodes;
    
    void clear()
    {
        paintNodes.clear();
    };
};

class Outcome
{
public:
    std::vector<OutcomeNode> nodes;
    std::vector<std::pair<QVector3D, std::pair<QUuid, QUuid>>> nodeVertices;
    std::vector<QVector3D> vertices;
    std::vector<std::pair<QUuid, QUuid>> vertexSourceNodes;
    std::vector<std::vector<size_t>> triangleAndQuads;
    std::vector<std::vector<size_t>> triangles;
    std::vector<QVector3D> triangleNormals;
    std::vector<QColor> triangleColors;
    std::vector<OutcomePaintMap> paintMaps;
    quint64 meshId = 0;
    
    const std::vector<std::pair<QUuid, QUuid>> *triangleSourceNodes() const
    {
        if (!m_hasTriangleSourceNodes)
            return nullptr;
        return &m_triangleSourceNodes;
    }
    void setTriangleSourceNodes(const std::vector<std::pair<QUuid, QUuid>> &sourceNodes)
    {
        Q_ASSERT(sourceNodes.size() == triangles.size());
        m_triangleSourceNodes = sourceNodes;
        m_hasTriangleSourceNodes = true;
    }
    
    const std::vector<std::vector<QVector2D>> *triangleVertexUvs() const
    {
        if (!m_hasTriangleVertexUvs)
            return nullptr;
        return &m_triangleVertexUvs;
    }
    void setTriangleVertexUvs(const std::vector<std::vector<QVector2D>> &uvs)
    {
        Q_ASSERT(uvs.size() == triangles.size());
        m_triangleVertexUvs = uvs;
        m_hasTriangleVertexUvs = true;
    }
    
    const std::vector<std::vector<QVector3D>> *triangleVertexNormals() const
    {
        if (!m_hasTriangleVertexNormals)
            return nullptr;
        return &m_triangleVertexNormals;
    }
    void setTriangleVertexNormals(const std::vector<std::vector<QVector3D>> &normals)
    {
        Q_ASSERT(normals.size() == triangles.size());
        m_triangleVertexNormals = normals;
        m_hasTriangleVertexNormals = true;
    }
    
    const std::vector<QVector3D> *triangleTangents() const
    {
        if (!m_hasTriangleTangents)
            return nullptr;
        return &m_triangleTangents;
    }
    void setTriangleTangents(const std::vector<QVector3D> &tangents)
    {
        Q_ASSERT(tangents.size() == triangles.size());
        m_triangleTangents = tangents;
        m_hasTriangleTangents = true;
    }
    
    const std::map<QUuid, std::vector<QRectF>> *partUvRects() const
    {
        if (!m_hasPartUvRects)
            return nullptr;
        return &m_partUvRects;
    }
    void setPartUvRects(const std::map<QUuid, std::vector<QRectF>> &uvRects)
    {
        m_partUvRects = uvRects;
        m_hasPartUvRects = true;
    }
    
    const std::vector<std::pair<std::pair<size_t, size_t>, std::pair<size_t, size_t>>> *triangleLinks() const
    {
        if (!m_hasTriangleLinks)
            return nullptr;
        return &m_triangleLinks;
    }
    void setTriangleLinks(const std::vector<std::pair<std::pair<size_t, size_t>, std::pair<size_t, size_t>>> &triangleLinks)
    {
        m_triangleLinks = triangleLinks;
        m_hasTriangleLinks = true;
    }
    
private:
    bool m_hasTriangleSourceNodes = false;
    std::vector<std::pair<QUuid, QUuid>> m_triangleSourceNodes;
    
    bool m_hasTriangleVertexUvs = false;
    std::vector<std::vector<QVector2D>> m_triangleVertexUvs;
    
    bool m_hasTriangleVertexNormals = false;
    std::vector<std::vector<QVector3D>> m_triangleVertexNormals;
    
    bool m_hasTriangleTangents = false;
    std::vector<QVector3D> m_triangleTangents;
    
    bool m_hasPartUvRects = false;
    std::map<QUuid, std::vector<QRectF>> m_partUvRects;
    
    bool m_hasTriangleLinks = false;
    std::vector<std::pair<std::pair<size_t, size_t>, std::pair<size_t, size_t>>> m_triangleLinks;
};

#endif
