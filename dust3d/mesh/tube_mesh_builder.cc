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

#include <algorithm>
#include <dust3d/base/debug.h>
#include <dust3d/mesh/base_normal.h>
#include <dust3d/mesh/section_remesher.h>
#include <dust3d/mesh/tube_mesh_builder.h>

namespace dust3d {

TubeMeshBuilder::TubeMeshBuilder(const BuildParameters& buildParameters, std::vector<MeshNode>&& nodes, bool isCircle)
    : m_buildParameters(buildParameters)
    , m_nodes(std::move(nodes))
    , m_isCircle(isCircle)
{
}

const Vector3& TubeMeshBuilder::generatedBaseNormal()
{
    return m_generatedBaseNormal;
}

const std::vector<Vector3>& TubeMeshBuilder::generatedVertices()
{
    return m_generatedVertices;
}

const std::vector<Uuid>& TubeMeshBuilder::generatedVertexSources()
{
    return m_generatedVertexSources;
}

const std::vector<std::vector<size_t>>& TubeMeshBuilder::generatedFaces()
{
    return m_generatedFaces;
}

const std::vector<std::vector<Vector2>>& TubeMeshBuilder::generatedFaceUvs()
{
    return m_generatedFaceUvs;
}

void TubeMeshBuilder::applyRoundEnd()
{
    if (m_isCircle)
        return;

    if (m_nodes.size() <= 1)
        return;

    if (m_buildParameters.frontEndRounded) {
        auto newNode = m_nodes.front();
        newNode.radius *= 0.5;
        newNode.origin += (m_nodes[0].origin - m_nodes[1].origin).normalized() * newNode.radius;
        m_nodes.insert(m_nodes.begin(), newNode);
    }

    if (m_buildParameters.backEndRounded) {
        auto newNode = m_nodes.back();
        newNode.radius *= 0.5;
        newNode.origin += (m_nodes[m_nodes.size() - 1].origin - m_nodes[m_nodes.size() - 2].origin).normalized() * newNode.radius;
        m_nodes.emplace_back(newNode);
    }
}

void TubeMeshBuilder::applyInterpolation()
{
    if (!m_buildParameters.interpolationEnabled)
        return;

    if (m_nodes.size() <= 1)
        return;

    std::vector<MeshNode> interpolatedNodes;
    interpolatedNodes.push_back(m_nodes.front());
    for (size_t j = 1; j < m_nodes.size(); ++j) {
        size_t i = j - 1;
        double distance = (m_nodes[i].origin - m_nodes[j].origin).length();
        double radiusDistance = m_nodes[i].radius + m_nodes[j].radius;
        if (radiusDistance <= distance) {
            double targetDistance = radiusDistance;
            size_t segments = distance / targetDistance;
            for (size_t k = 1; k < segments; ++k) {
                double ratio = (double)k / segments;
                MeshNode newNode;
                newNode.origin = m_nodes[i].origin * (1.0 - ratio) + m_nodes[j].origin * ratio;
                newNode.radius = m_nodes[i].radius * (1.0 - ratio) + m_nodes[j].radius * ratio;
                interpolatedNodes.push_back(newNode);
            }
        }
        interpolatedNodes.push_back(m_nodes[j]);
    }

    m_nodes = std::move(interpolatedNodes);
}

void TubeMeshBuilder::preprocessNodes()
{
    applyInterpolation();
    applyRoundEnd();
}

void TubeMeshBuilder::buildNodePositionAndDirections()
{
    m_nodePositions.resize(m_nodes.size());
    for (size_t i = 0; i < m_nodes.size(); ++i)
        m_nodePositions[i] = m_nodes[i].origin;

    std::vector<Vector3> rawDirections;
    rawDirections.resize(m_nodePositions.size());
    m_nodeForwardDistances.resize(m_nodePositions.size());
    m_nodeForwardDirections.resize(m_nodePositions.size());
    if (m_nodePositions.size() >= 2) {
        if (m_isCircle) {
            for (size_t i = 0; i < m_nodePositions.size(); ++i) {
                size_t j = (i + 1) % m_nodePositions.size();
                rawDirections[i] = (m_nodePositions[j] - m_nodePositions[i]);
                m_nodeForwardDistances[i] = rawDirections[i].length();
                rawDirections[i].normalize();
            }
            for (size_t i = 0; i < m_nodePositions.size(); ++i) {
                size_t j = (i + 1) % m_nodePositions.size();
                m_nodeForwardDirections[j] = (rawDirections[i] + rawDirections[j]).normalized();
            }
        } else {
            for (size_t j = 1; j < m_nodePositions.size(); ++j) {
                size_t i = j - 1;
                rawDirections[i] = (m_nodePositions[j] - m_nodePositions[i]);
                m_nodeForwardDistances[i] = rawDirections[i].length();
                rawDirections[i].normalize();
            }
            rawDirections[rawDirections.size() - 1] = rawDirections[rawDirections.size() - 2];
            m_nodeForwardDirections.front() = rawDirections.front();
            for (size_t j = 1; j + 1 < m_nodePositions.size(); ++j) {
                size_t i = j - 1;
                m_nodeForwardDirections[j] = (rawDirections[i] * m_nodeForwardDistances[j] + rawDirections[j] * m_nodeForwardDistances[i]).normalized();
            }
            m_nodeForwardDirections.back() = rawDirections.back();
        }
    }
}

std::vector<Vector3> TubeMeshBuilder::buildCutFaceVertices(const Vector3& origin,
    double radius,
    const Vector3& forwardDirection)
{
    std::vector<Vector3> cutFaceVertices(m_buildParameters.cutFace.size());
    Vector3 u = m_generatedBaseNormal.rotated(-forwardDirection, m_buildParameters.baseNormalRotation);
    Vector3 v = Vector3::crossProduct(forwardDirection, u).normalized();
    u = Vector3::crossProduct(v, forwardDirection).normalized();
    auto uFactor = u * radius * m_buildParameters.deformWidth;
    auto vFactor = v * radius * m_buildParameters.deformThickness;
    if (m_buildParameters.deformUnified) {
        if (!Math::isEqual(m_buildParameters.deformWidth, 1.0)) {
            uFactor *= m_maxNodeRadius / radius;
        }
        if (!Math::isEqual(m_buildParameters.deformThickness, 1.0)) {
            vFactor *= m_maxNodeRadius / radius;
        }
    }
    for (size_t i = 0; i < m_buildParameters.cutFace.size(); ++i) {
        const auto& t = m_buildParameters.cutFace[i];
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

    for (const auto& it : m_nodes)
        m_maxNodeRadius = std::max(m_maxNodeRadius, it.radius);

    m_generatedBaseNormal = m_isCircle ? BaseNormal::calculateCircleBaseNormal(m_nodePositions) : BaseNormal::calculateTubeBaseNormal(m_nodePositions);

    // Prepare V of UV for cap
    double vOffsetBecauseOfFrontCap = 0.0;
    double vTubeRatio = 1.0;
    if (!m_isCircle) {
        double totalTubeLength = 0.0;
        for (const auto& it : m_nodeForwardDistances)
            totalTubeLength += it;
        double totalLength = m_nodes.front().radius + totalTubeLength + m_nodes.back().radius;
        vTubeRatio = totalTubeLength / totalLength;
        vOffsetBecauseOfFrontCap = m_nodes.front().radius / totalLength;
    }
    auto tubeUv = [=](const Vector2& uv) {
        return Vector2(uv[0], uv[1] * vTubeRatio + vOffsetBecauseOfFrontCap);
    };

    // Build all vertex Positions
    std::vector<std::vector<Vector3>> cutFaceVertexPositions;
    for (size_t i = 0; i < m_nodePositions.size(); ++i) {
        cutFaceVertexPositions.emplace_back(buildCutFaceVertices(m_nodePositions[i],
            m_nodes[i].radius,
            m_nodeForwardDirections[i]));
    }

    // Build all vertex Uvs
    auto cutFaceVertexPositionsForUv = cutFaceVertexPositions;
    for (size_t n = 0; n < cutFaceVertexPositionsForUv.size(); ++n) {
        cutFaceVertexPositionsForUv[n].push_back(cutFaceVertexPositions[n].front());
    }
    std::vector<std::vector<Vector2>> cutFaceVertexUvs(cutFaceVertexPositionsForUv.size());
    std::vector<double> maxUs(cutFaceVertexPositionsForUv.size(), 0.0);
    std::vector<double> maxVs(cutFaceVertexPositionsForUv.front().size(), 0.0);
    for (size_t n = 0; n < cutFaceVertexPositionsForUv.size(); ++n) {
        const auto& cutFaceVertices = cutFaceVertexPositionsForUv[n];
        double offsetU = 0;
        if (n > 0) {
            size_t m = n - 1;
            for (size_t i = 0; i < cutFaceVertices.size(); ++i) {
                maxVs[i] += (cutFaceVertexPositionsForUv[n][i] - cutFaceVertexPositionsForUv[m][i]).length();
            }
        }
        std::vector<Vector2> uvCoords = { Vector2 { offsetU, maxVs[0] } };
        for (size_t j = 1; j < cutFaceVertices.size(); ++j) {
            size_t i = j - 1;
            offsetU += (cutFaceVertices[j] - cutFaceVertices[i]).length();
            uvCoords.push_back({ offsetU, maxVs[j] });
        }
        cutFaceVertexUvs[n] = uvCoords;
        maxUs[n] = offsetU;
    }
    for (size_t n = 0; n < cutFaceVertexUvs.size(); ++n) {
        for (size_t k = 0; k < cutFaceVertexUvs[n].size(); ++k) {
            cutFaceVertexUvs[n][k][0] /= std::max(maxUs[n], std::numeric_limits<double>::epsilon());
            cutFaceVertexUvs[n][k][1] /= std::max(maxVs[k], std::numeric_limits<double>::epsilon());
        }
    }

    // Generate vertex indices
    std::vector<std::vector<size_t>> cutFaceIndices(m_nodePositions.size());
    for (size_t i = 0; i < cutFaceVertexPositions.size(); ++i) {
        const auto& cutFaceVertices = cutFaceVertexPositions[i];
        cutFaceIndices[i].resize(cutFaceVertices.size());
        for (size_t k = 0; k < cutFaceVertices.size(); ++k) {
            cutFaceIndices[i][k] = m_generatedVertices.size();
            m_generatedVertices.emplace_back(cutFaceVertices[k]);
            m_generatedVertexSources.emplace_back(m_nodes[i].sourceId);
        }
    }

    // Generate faces
    for (size_t j = m_isCircle ? 0 : 1; j < cutFaceIndices.size(); ++j) {
        size_t i = (j + cutFaceIndices.size() - 1) % cutFaceIndices.size();
        const auto& cutFaceI = cutFaceIndices[i];
        const auto& cutFaceJ = cutFaceIndices[j];
        size_t halfSize = cutFaceI.size() / 2;
        for (size_t m = 0; m < halfSize; ++m) {
            size_t n = (m + 1) % cutFaceI.size();
            // KEEP QUAD ORDER TO MAKE THE TRIANLES NO CROSSING OVER ON EACH SIDE (1)
            // The following quad vertices should follow the order strictly,
            // This will group two points from I, and one point from J as a triangle in the later quad to triangles processing.
            // If not follow this order, the front triangle and back triangle maybe cross over because of not be parallel.
            m_generatedFaces.emplace_back(std::vector<size_t> {
                cutFaceI[m], cutFaceI[n], cutFaceJ[n], cutFaceJ[m] });
            m_generatedFaceUvs.emplace_back(std::vector<Vector2> {
                tubeUv(cutFaceVertexUvs[i][m]),
                tubeUv(cutFaceVertexUvs[i][m + 1]),
                tubeUv(cutFaceVertexUvs[j][m + 1]),
                tubeUv(cutFaceVertexUvs[j][m]) });
        }
        for (size_t m = halfSize; m < cutFaceI.size(); ++m) {
            size_t n = (m + 1) % cutFaceI.size();
            // KEEP QUAD ORDER TO MAKE THE TRIANLES NO CROSSING OVER ON EACH SIDE (2)
            // The following quad vertices should follow the order strictly,
            // This will group two points from I, and one point from J as a triangle in the later quad to triangles processing.
            // If not follow this order, the front triangle and back triangle maybe cross over because of not be parallel.
            m_generatedFaces.emplace_back(std::vector<size_t> {
                cutFaceJ[m], cutFaceI[m], cutFaceI[n], cutFaceJ[n] });
            m_generatedFaceUvs.emplace_back(std::vector<Vector2> {
                tubeUv(cutFaceVertexUvs[j][m]),
                tubeUv(cutFaceVertexUvs[i][m]),
                tubeUv(cutFaceVertexUvs[i][m + 1]),
                tubeUv(cutFaceVertexUvs[j][m + 1]) });
        }
    }
    if (!m_isCircle) {
        addCap(cutFaceIndices.back(), vOffsetBecauseOfFrontCap + vTubeRatio, 1.0, false);
        addCap(cutFaceIndices.front(), vOffsetBecauseOfFrontCap, 0.0, true);
    }
}

void TubeMeshBuilder::addCap(const std::vector<size_t>& section, double ringV, double centerV, bool reverseU)
{
    std::vector<size_t> vertexIndices = section;
    std::vector<Vector3> ringVertices(vertexIndices.size());
    for (size_t i = 0; i < ringVertices.size(); ++i) {
        ringVertices[i] = m_generatedVertices[vertexIndices[i]];
    }
    SectionRemesher sectionRemesher(ringVertices, ringV, centerV);
    sectionRemesher.remesh();
    const std::vector<Vector3>& resultVertices = sectionRemesher.generatedVertices();
    for (size_t i = ringVertices.size(); i < resultVertices.size(); ++i) {
        vertexIndices.push_back(m_generatedVertices.size());
        m_generatedVertices.push_back(resultVertices[i]);
    }
    for (const auto& it : sectionRemesher.generatedFaces()) {
        std::vector<size_t> newFace(it.size());
        for (size_t i = 0; i < it.size(); ++i)
            newFace[i] = vertexIndices[it[i]];
        m_generatedFaces.emplace_back(newFace);
        if (reverseU)
            std::reverse(m_generatedFaces.back().begin(), m_generatedFaces.back().end());
    }
    for (const auto& it : sectionRemesher.generatedFaceUvs()) {
        m_generatedFaceUvs.emplace_back(it);
        if (reverseU)
            std::reverse(m_generatedFaceUvs.back().begin(), m_generatedFaceUvs.back().end());
    }
}

}
