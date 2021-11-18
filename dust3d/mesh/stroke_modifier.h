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

#ifndef DUST3D_MESH_STROKE_MODIFIER_H_
#define DUST3D_MESH_STROKE_MODIFIER_H_

#include <vector>
#include <dust3d/base/vector3.h>

namespace dust3d
{
    
class StrokeModifier
{
public:
    struct Node
    {
        bool isOriginal = false;
        Vector3 position;
        float radius = 0.0;
        std::vector<Vector2> cutTemplate;
        float cutRotation = 0.0;
        int nearOriginNodeIndex = -1;
        int farOriginNodeIndex = -1;
        int originNodeIndex = 0;
        float averageCutTemplateLength;
    };
    
    struct Edge
    {
        size_t firstNodeIndex;
        size_t secondNodeIndex;
    };
    
    size_t addNode(const Vector3 &position, float radius, const std::vector<Vector2> &cutTemplate, float cutRotation);
    size_t addEdge(size_t firstNodeIndex, size_t secondNodeIndex);
    void subdivide();
    void roundEnd();
    void enableIntermediateAddition();
    void enableSmooth();
    const std::vector<Node> &nodes() const;
    const std::vector<Edge> &edges() const;
    void finalize();
    
    static void subdivideFace(std::vector<Vector2> *face);
    
private:
    
    std::vector<Node> m_nodes;
    std::vector<Edge> m_edges;
    bool m_intermediateAdditionEnabled = false;
    bool m_smooth = false;
    
    void createIntermediateNode(const Node &firstNode, const Node &secondNode, float factor, Node *resultNode);
    float averageCutTemplateEdgeLength(const std::vector<Vector2> &cutTemplate);
    void createIntermediateCutTemplateEdges(std::vector<Vector2> &cutTemplate, float averageCutTemplateLength);
    void smooth();
};

}

#endif
