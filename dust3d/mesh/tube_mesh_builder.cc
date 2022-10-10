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

#include <dust3d/mesh/base_normal.h>
#include <dust3d/mesh/tube_mesh_builder.h>

namespace dust3d
{

TubeMeshBuilder::TubeMeshBuilder(const BuildParameters &buildParameters, std::vector<Node> &&nodes, bool isCircle):
    m_buildParameters(buildParameters),
    m_nodes(std::move(nodes)),
    m_isCircle(isCircle)
{
}

const Vector3 &TubeMeshBuilder::resultBaseNormal()
{
    return m_baseNormal;
}

void TubeMeshBuilder::preprocessNodes()
{
    // TODO: Interpolate...
}

void TubeMeshBuilder::finalizeDataForNodes()
{
    m_nodePositions.resize(m_nodes.size());
    for (size_t i = 0; i < m_nodes.size(); ++i)
        m_nodePositions[i] = m_nodes[i].origin;

    m_nodeForwardDirections.resize(m_nodes.size());
    if (m_isCircle) {
        // TODO:
    } else {
        // TODO:
    }
}

void TubeMeshBuilder::build()
{
    preprocessNodes();

    if (m_nodes.empty())
        return;

    m_baseNormal = m_isCircle ? 
        BaseNormal::calculateCircleBaseNormal(m_nodePositions) : 
        BaseNormal::calculateTubeBaseNormal(m_nodePositions);
    
    // TODO:
}

}
