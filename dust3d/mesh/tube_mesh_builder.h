/*
 *  Copyright (c) 2016-2022 Jeremy HU <jeremy-at-dust3d dot org>. All rights reserved. 
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

#ifndef DUST3D_MESH_TUBE_MESH_BUILDER_H_
#define DUST3D_MESH_TUBE_MESH_BUILDER_H_

#include <dust3d/base/uuid.h>
#include <dust3d/base/vector2.h>
#include <dust3d/base/vector3.h>
#include <dust3d/mesh/mesh_node.h>

namespace dust3d
{

class TubeMeshBuilder
{
public:
    struct BuildParameters
    {
        std::vector<Vector2> cutFace;
        double deformThickness = 1.0;
        double deformWidth = 1.0;
        bool deformUnified = false;
        double baseNormalRotation = 0.0;
    };

    TubeMeshBuilder(const BuildParameters &buildParameters, std::vector<MeshNode> &&nodes, bool isCircle);
    void build();
    const Vector3 &generatedBaseNormal();
    const std::vector<Vector3> &generatedVertices();
    const std::vector<std::vector<size_t>> &generatedFaces();
private:
    BuildParameters m_buildParameters;
    std::vector<MeshNode> m_nodes;
    std::vector<Vector3> m_nodePositions;
    std::vector<Vector3> m_nodeForwardDirections;
    std::vector<double> m_nodeForwardDistances;
    std::vector<Vector3> m_generatedVertices;
    std::vector<std::vector<size_t>> m_generatedFaces;
    Vector3 m_generatedBaseNormal;
    bool m_isCircle = false;
    double m_maxNodeRadius = 0.0;
    void preprocessNodes();
    void buildNodePositionAndDirections();
    std::vector<Vector3> buildCutFaceVertices(const Vector3 &origin,
        double radius,
        const Vector3 &forwardDirection);
};

};

#endif
