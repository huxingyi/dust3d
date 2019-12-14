#ifndef DUST3D_BUILDER_H
#define DUST3D_BUILDER_H
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
    
    size_t addNode(const QVector3D &position, float radius, const std::vector<QVector2D> &cutTemplate, float cutRotation);
    size_t addEdge(size_t firstNodeIndex, size_t secondNodeIndex);
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
    const std::vector<QVector3D> &generatedVertices();
    const std::vector<std::vector<size_t>> &generatedFaces();
    const std::vector<size_t> &generatedVerticesSourceNodeIndices();
    void exportAsObj(const QString &filename);
    bool build();
    const CutFaceTransform *nodeAdjustableCutFaceTransform(size_t nodeIndex);
    const QVector3D &nodeTraverseDirection(size_t nodeIndex) const;
    const QVector3D &nodeBaseNormal(size_t nodeIndex) const;
    size_t nodeTraverseOrder(size_t nodeIndex) const;

private:

    struct Edge;

    struct Node
    {
        float radius;
        QVector3D position;
        std::vector<size_t> edges;
        std::vector<QVector2D> cutTemplate;
        float cutRotation;
        std::vector<QVector3D> raysToNeibors;
        QVector3D cutNormal;
        CutFaceTransform cutFaceTransform;
        QVector3D initialTraverseDirection;
        QVector3D traverseDirection;
        QVector3D growthDirection;
        QVector3D initialBaseNormal;
        QVector3D baseNormal;
        size_t reversedTraverseOrder;
        bool hasInitialBaseNormal = false;
        bool baseNormalResolved = false;
        bool baseNormalSearched = false;
        bool hasInitialTraverseDirection = false;
        bool hasAdjustableCutFace = false;
        int nearOriginNodeIndex = -1;
        int farOriginNodeIndex = -1;
        
        size_t anotherEdge(size_t edgeIndex) const
        {
            if (edges.size() != 2)
                return edgeIndex;
            const auto &otherIndex = edges[0];
            if (otherIndex == edgeIndex)
                return edges[1];
            return otherIndex;
        }
    };
    
    struct Edge
    {
        std::vector<size_t> nodes;
        std::vector<std::pair<std::vector<size_t>, QVector3D>> cuts;
        
        size_t neiborOf(size_t nodeIndex) const
        {
            const auto &otherIndex = nodes[0];
            if (otherIndex == nodeIndex)
                return nodes[1];
            return otherIndex;
        }
        
        void updateNodeIndex(size_t fromNodeIndex, size_t toNodeIndex)
        {
            if (nodes[0] == fromNodeIndex) {
                nodes[0] = toNodeIndex;
                return;
            }
            nodes[1] = toNodeIndex;
        }
    };
    
    struct GeneratedVertexInfo
    {
        size_t orderInCut;
        size_t cutSize;
    };
    
    std::vector<Node> m_nodes;
    std::vector<Edge> m_edges;
    std::vector<QVector3D> m_generatedVertices;
    std::vector<QVector3D> m_generatedVerticesCutDirects;
    std::vector<size_t> m_generatedVerticesSourceNodeIndices;
    std::vector<GeneratedVertexInfo> m_generatedVerticesInfos;
    std::vector<std::vector<size_t>> m_generatedFaces;
    std::vector<size_t> m_sortedNodeIndices;
    std::map<size_t, size_t> m_weldMap;
    std::set<size_t> m_swallowedEdges;
    std::set<size_t> m_swallowedNodes;
    float m_deformThickness = 1.0;
    float m_deformWidth = 1.0;
    float m_cutRotation = 0.0;
    bool m_baseNormalOnX = true;
    bool m_baseNormalOnY = true;
    bool m_baseNormalOnZ = true;
    bool m_baseNormalAverageEnabled = false;
    const QImage *m_deformMapImage = nullptr;
    float m_deformMapScale = 0.0;
    float m_hollowThickness = 0.2;
    std::vector<std::vector<size_t>> m_endCuts;
    
    void sortNodeIndices();
    void prepareNode(size_t nodeIndex);
    std::pair<QVector3D, bool> calculateBaseNormal(const std::vector<QVector3D> &inputDirects,
        const std::vector<QVector3D> &inputPositions,
        const std::vector<float> &weights);
    bool validateNormal(const QVector3D &normal);
    void resolveBaseNormalRecursively(size_t nodeIndex);
    void resolveBaseNormalForLeavesRecursively(size_t nodeIndex, const QVector3D &baseNormal);
    std::pair<QVector3D, bool> searchBaseNormalFromNeighborsRecursively(size_t nodeIndex);
    QVector3D revisedBaseNormalAcordingToCutNormal(const QVector3D &baseNormal, const QVector3D &cutNormal);
    void resolveInitialTraverseDirectionRecursively(size_t nodeIndex, const QVector3D *from, std::set<size_t> *visited);
    void unifyBaseNormals();
    void localAverageBaseNormals();
    void resolveTraverseDirection(size_t nodeIndex);
    bool generateCutsForNode(size_t nodeIndex);
    bool tryWrapMultipleBranchesForNode(size_t nodeIndex, std::vector<float> &offsets, bool &offsetsChanged);
    void makeCut(const QVector3D &position,
        float radius,
        const std::vector<QVector2D> &cutTemplate,
        float cutRotation,
        QVector3D &baseNormal,
        QVector3D &cutNormal,
        const QVector3D &traverseDirection,
        std::vector<QVector3D> &resultCut,
        CutFaceTransform *cutFaceTransform=nullptr,
        bool *cutFlipped=nullptr);
    void insertCutVertices(const std::vector<QVector3D> &cut,
        std::vector<size_t> &vertices,
        size_t nodeIndex,
        const QVector3D &cutDirect,
        bool cutFlipped);
    void stitchEdgeCuts();
    void applyWeld();
    void applyDeform();
    void finalizeHollow();
    QVector3D calculateDeformPosition(const QVector3D &vertexPosition, const QVector3D &ray, const QVector3D &deformNormal, float deformFactor);
    bool swallowEdgeForNode(size_t nodeIndex, size_t edgeOrder);
    static QVector3D calculateBaseNormalFromTraverseDirection(const QVector3D &traverseDirection);
    void layoutNodes();
};

#endif
