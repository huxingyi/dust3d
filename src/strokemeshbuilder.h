#ifndef DUST3D_STOKE_MESH_BUILDER_H
#define DUST3D_STOKE_MESH_BUILDER_H
#include <QVector3D>
#include <QVector2D>
#include <vector>
#include <map>
#include <set>
#include <QMatrix4x4>
#include <QImage>
#include "positionkey.h"

class StrokeMeshBuilder
{
public:
    struct CutFaceTransform
    {
        QVector3D translation;
        float scale;
        QMatrix4x4 rotation;
        QVector3D uFactor;
        QVector3D vFactor;
        bool reverse = false;
    };
    
    struct Node
    {
        float radius;
        QVector3D position;
        std::vector<QVector2D> cutTemplate;
        float cutRotation;
        int nearOriginNodeIndex = -1;
        int farOriginNodeIndex = -1;
        
        size_t index;
        std::vector<size_t> neighbors;
        size_t next;
        QVector3D cutNormal;
        QVector3D traverseDirection;
        QVector3D baseNormal;
        size_t traverseOrder;
        
        size_t nextOrNeighborOtherThan(size_t neighborIndex) const;
    };
    
    size_t addNode(const QVector3D &position, float radius, const std::vector<QVector2D> &cutTemplate, float cutRotation);
    void addEdge(size_t firstNodeIndex, size_t secondNodeIndex);
    void setNodeOriginInfo(size_t nodeIndex, int nearOriginNodeIndex, int farOriginNodeIndex);
    void setDeformThickness(float thickness);
    void setDeformWidth(float width);
    void setDeformMapImage(const QImage *image);
    void setDeformMapScale(float scale);
    void setHollowThickness(float hollowThickness);
    void enableBaseNormalOnX(bool enabled);
    void enableBaseNormalOnY(bool enabled);
    void enableBaseNormalOnZ(bool enabled);
    void enableBaseNormalAverage(bool enabled);
    bool buildBaseNormalsOnly();
    const std::vector<Node> &nodes() const;
    const std::vector<size_t> &nodeIndices() const;
    const QVector3D &nodeTraverseDirection(size_t nodeIndex) const;
    const QVector3D &nodeBaseNormal(size_t nodeIndex) const;
    size_t nodeTraverseOrder(size_t nodeIndex) const;
    bool build();
    const std::vector<QVector3D> &generatedVertices();
    const std::vector<std::vector<size_t>> &generatedFaces();
    const std::vector<size_t> &generatedVerticesSourceNodeIndices();

private:
    struct GeneratedVertexInfo
    {
        size_t orderInCut;
        size_t cutSize;
    };
    
    std::vector<Node> m_nodes;
    float m_deformThickness = 1.0f;
    float m_deformWidth = 1.0f;
    float m_cutRotation = 0.0f;
    bool m_baseNormalOnX = true;
    bool m_baseNormalOnY = true;
    bool m_baseNormalOnZ = true;
    bool m_baseNormalAverageEnabled = false;
    const QImage *m_deformMapImage = nullptr;
    float m_deformMapScale = 0.0f;
    float m_hollowThickness = 0.0f;
    
    bool m_isRing = false;
    std::vector<size_t> m_nodeIndices;
    std::vector<QVector3D> m_generatedVertices;
    std::vector<QVector3D> m_generatedVerticesCutDirects;
    std::vector<size_t> m_generatedVerticesSourceNodeIndices;
    std::vector<GeneratedVertexInfo> m_generatedVerticesInfos;
    std::vector<std::vector<size_t>> m_generatedFaces;
    
    bool prepare();
    QVector3D calculateBaseNormalFromTraverseDirection(const QVector3D &traverseDirection);
    std::vector<QVector3D> makeCut(const QVector3D &cutCenter, 
        float radius, 
        const std::vector<QVector2D> &cutTemplate, 
        const QVector3D &cutNormal,
        const QVector3D &baseNormal);
    void insertCutVertices(const std::vector<QVector3D> &cut,
        std::vector<size_t> *vertices,
        size_t nodeIndex,
        const QVector3D &cutNormal);
    void buildMesh();
    std::vector<size_t> sortedNodeIndices(bool *isRing);
    bool calculateStartingNodeIndex(size_t *startingNodeIndex, 
        bool *isRing);
    void reviseTraverseDirections();
    void localAverageBaseNormals();
    void unifyBaseNormals();
    std::vector<size_t> edgeloopFlipped(const std::vector<size_t> &edgeLoop);
    void reviseNodeBaseNormal(Node &node);
    static QVector3D calculateDeformPosition(const QVector3D &vertexPosition, const QVector3D &ray, const QVector3D &deformNormal, float deformFactor);
    void applyDeform();
};

#endif
