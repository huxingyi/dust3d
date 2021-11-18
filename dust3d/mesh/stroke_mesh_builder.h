/*
 *  Copyright (c) 2016-2021 Jeremy HU <jeremy-at-dust3d dot org>. All rights reserved. 
 *
 *  Permission is hereby granted, free of charge, to any person obtaining a copy
 *  of this software and associated documentation files (the "Software"), to deal
 *  in the Software without restriction, including without limitation the rights
 *  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 *  copies of the Software, and to permit persons to whom the Software is
 *  furnished to do so, subject to the following conditions:

 *  The above copyright notice and this permission notice shall be included in all
 *  copies or substantial portions of the Software.

 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 *  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 *  SOFTWARE.
 */

#ifndef DUST3D_MESH_STOKE_MESH_BUILDER_H_
#define DUST3D_MESH_STOKE_MESH_BUILDER_H_

#include <dust3d/base/vector3.h>
#include <dust3d/base/vector2.h>
#include <vector>
#include <map>
#include <set>

namespace dust3d
{

class StrokeMeshBuilder
{
public:
    struct CutFaceTransform
    {
        Vector3 translation;
        float scale;
        Vector3 uFactor;
        Vector3 vFactor;
        bool reverse = false;
    };
    
    struct Node
    {
        float radius;
        Vector3 position;
        std::vector<Vector2> cutTemplate;
        float cutRotation;
        int nearOriginNodeIndex = -1;
        int farOriginNodeIndex = -1;
        
        size_t index;
        std::vector<size_t> neighbors;
        size_t next;
        Vector3 cutNormal;
        Vector3 traverseDirection;
        Vector3 baseNormal;
        size_t traverseOrder;
        
        size_t nextOrNeighborOtherThan(size_t neighborIndex) const;
    };
    
    size_t addNode(const Vector3 &position, float radius, const std::vector<Vector2> &cutTemplate, float cutRotation);
    void addEdge(size_t firstNodeIndex, size_t secondNodeIndex);
    void setNodeOriginInfo(size_t nodeIndex, int nearOriginNodeIndex, int farOriginNodeIndex);
    void setDeformThickness(float thickness);
    void setDeformWidth(float width);
    void setDeformUnified(bool unified);
    void setHollowThickness(float hollowThickness);
    void enableBaseNormalOnX(bool enabled);
    void enableBaseNormalOnY(bool enabled);
    void enableBaseNormalOnZ(bool enabled);
    void enableBaseNormalAverage(bool enabled);
    bool buildBaseNormalsOnly();
    const std::vector<Node> &nodes() const;
    const std::vector<size_t> &nodeIndices() const;
    const Vector3 &nodeTraverseDirection(size_t nodeIndex) const;
    const Vector3 &nodeBaseNormal(size_t nodeIndex) const;
    size_t nodeTraverseOrder(size_t nodeIndex) const;
    bool build();
    const std::vector<Vector3> &generatedVertices();
    const std::vector<std::vector<size_t>> &generatedFaces();
    const std::vector<size_t> &generatedVerticesSourceNodeIndices();
    
    static Vector3 calculateDeformPosition(const Vector3 &vertexPosition, const Vector3 &ray, const Vector3 &deformNormal, float deformFactor);
    static Vector3 calculateBaseNormalFromTraverseDirection(const Vector3 &traverseDirection);
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
    float m_hollowThickness = 0.0f;
    bool m_deformUnified = false;
    
    bool m_isRing = false;
    std::vector<size_t> m_nodeIndices;
    std::vector<Vector3> m_generatedVertices;
    std::vector<Vector3> m_generatedVerticesCutDirects;
    std::vector<size_t> m_generatedVerticesSourceNodeIndices;
    std::vector<GeneratedVertexInfo> m_generatedVerticesInfos;
    std::vector<std::vector<size_t>> m_generatedFaces;
    
    std::vector<std::vector<size_t>> m_cuts;
    
    bool prepare();
    std::vector<Vector3> makeCut(const Vector3 &cutCenter, 
        float radius, 
        const std::vector<Vector2> &cutTemplate, 
        const Vector3 &cutNormal,
        const Vector3 &baseNormal);
    void insertCutVertices(const std::vector<Vector3> &cut,
        std::vector<size_t> *vertices,
        size_t nodeIndex,
        const Vector3 &cutNormal);
    void buildMesh();
    std::vector<size_t> sortedNodeIndices(bool *isRing);
    bool calculateStartingNodeIndex(size_t *startingNodeIndex, 
        bool *isRing);
    void reviseTraverseDirections();
    void localAverageBaseNormals();
    void unifyBaseNormals();
    std::vector<size_t> edgeloopFlipped(const std::vector<size_t> &edgeLoop);
    void reviseNodeBaseNormal(Node &node);
    void applyDeform();
    void interpolateCutEdges();
    void stitchCuts();
};

}

#endif
