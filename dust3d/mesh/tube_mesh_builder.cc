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

void TubeMeshBuilder::buildNodePositionAndDirections()
{
    m_nodePositions.resize(m_nodes.size());
    for (size_t i = 0; i < m_nodes.size(); ++i)
        m_nodePositions[i] = m_nodes[i].origin;

    std::vector<Vector3> rawDirections;
    rawDirections.resize(m_nodePositions.size());
    m_nodeForwardDirections.resize(m_nodePositions.size());
    if (m_nodePositions.size() >= 2) {
        if (m_isCircle) {
            for (size_t i = 0; i < m_nodePositions.size(); ++i) {
                size_t j = (i + 1) % m_nodePositions.size();
                rawDirections[i] = (m_nodePositions[j] - m_nodePositions[i]).normalized();
            }
            for (size_t i = 0; i < m_nodePositions.size(); ++i) {
                size_t j = (i + 1) % m_nodePositions.size();
                m_nodeForwardDirections[j] = (rawDirections[i] + rawDirections[j]).normalized();
            }
        } else {
            for (size_t j = 1; j < m_nodePositions.size(); ++j) {
                size_t i = j - 1;
                rawDirections[i] = (m_nodePositions[j] - m_nodePositions[i]).normalized();
            }
            rawDirections[m_nodeForwardDirections.size() - 1] = rawDirections[m_nodeForwardDirections.size() - 2];
            m_nodeForwardDirections.front() = rawDirections.front();
            for (size_t j = 1; j + 1 < m_nodePositions.size(); ++j) {
                size_t i = j - 1;
                m_nodeForwardDirections[j] = (rawDirections[i] + rawDirections[j]).normalized();
            }
            m_nodeForwardDirections.back() = rawDirections.back();
        }
    }
}

std::vector<Vector3> TubeMeshBuilder::buildCutFaceVertices(const Vector3 &origin,
    double radius,
    const Vector3 &forwardDirection)
{
    std::vector<Vector3> cutFaceVertices(m_buildParameters.cutFace.size());
    Vector3 u = m_baseNormal;
    Vector3 v = Vector3::crossProduct(forwardDirection, m_baseNormal).normalized();
    auto uFactor = u * radius;
    auto vFactor = v * radius;
    for (size_t i = 0; i < m_buildParameters.cutFace.size(); ++i) {
        const auto &t = m_buildParameters.cutFace[i];
        cutFaceVertices[i] = origin + (uFactor * t.x() + vFactor * t.y());
    }
    return cutFaceVertices;
}

void TubeMeshBuilder::build()
{
    preprocessNodes();

    if (m_nodes.empty())
        return;

    buildNodePositionAndDirections();

    m_baseNormal = m_isCircle ? 
        BaseNormal::calculateCircleBaseNormal(m_nodePositions) : 
        BaseNormal::calculateTubeBaseNormal(m_nodePositions);
    
    std::vector<std::vector<size_t>> cutFaceIndices(m_nodePositions.size());
    for (size_t i = 0; i < m_nodePositions.size(); ++i) {
        auto cutFaceVertices = buildCutFaceVertices(m_nodePositions[i],
            m_nodes[i].radius,
            m_nodeForwardDirections[i]);
        cutFaceIndices[i].resize(cutFaceVertices.size());
        for (size_t k = 0; k < cutFaceVertices.size(); ++k) {
            cutFaceIndices[i][k] = m_generatedVertices.size();
            m_generatedVertices.emplace_back(cutFaceVertices[k]);
        }
    }
    for (size_t j = m_isCircle ? 0 : 1; j < cutFaceIndices.size(); ++j) {
        size_t i = (j + cutFaceIndices.size() - 1) % cutFaceIndices.size();
        const auto &cutFaceI = cutFaceIndices[i];
        const auto &cutFaceJ = cutFaceIndices[j];
        for (size_t m = 0; m < cutFaceI.size(); ++m) {
            size_t n = (m + 1) % cutFaceI.size();
            m_generatedFaces.emplace_back(std::vector<size_t> {
                cutFaceI[m], cutFaceI[n], cutFaceJ[n], cutFaceJ[m]
            });
        }
    }
}

}
