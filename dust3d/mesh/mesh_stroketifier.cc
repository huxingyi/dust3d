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

#include <set>
#include <dust3d/base/matrix4x4.h>
#include <dust3d/base/quaternion.h>
#include <dust3d/mesh/mesh_stroketifier.h>
#include <dust3d/mesh/stroke_mesh_builder.h>

namespace dust3d
{

void MeshStroketifier::setCutRotation(float cutRotation)
{
	m_cutRotation = cutRotation;
}

void MeshStroketifier::setDeformThickness(float deformThickness)
{
    m_deformThickness = deformThickness;
}

void MeshStroketifier::setDeformWidth(float deformWidth)
{
    m_deformWidth = deformWidth;
}

bool MeshStroketifier::prepare(const std::vector<Node> &strokeNodes, 
        const std::vector<Vector3> &vertices)
{
    if (strokeNodes.empty() || vertices.empty())
        return false;
    
    float boundingBoxMinX = 0.0;
    float boundingBoxMaxX = 0.0;
    float boundingBoxMinY = 0.0;
    float boundingBoxMaxY = 0.0;
    float boundingBoxMinZ = 0.0;
    float boundingBoxMaxZ = 0.0;
    calculateBoundingBox(vertices,
        &boundingBoxMinX, &boundingBoxMaxX,
        &boundingBoxMinY, &boundingBoxMaxY,
        &boundingBoxMinZ, &boundingBoxMaxZ);
    float xLength = boundingBoxMaxX - boundingBoxMinX;
    float yLength = boundingBoxMaxY - boundingBoxMinY;
    float zLength = boundingBoxMaxZ - boundingBoxMinZ;
    if (yLength >= xLength && yLength >= zLength) {
        // Y-axis
        m_modelOrigin = Vector3((boundingBoxMinX + boundingBoxMaxX) * 0.5, 
            boundingBoxMinY, 
            (boundingBoxMinZ + boundingBoxMaxZ) * 0.5);
        m_modelAlignDirection = Vector3(0.0, 1.0, 0.0);
        m_modelBaseNormal = StrokeMeshBuilder::calculateBaseNormalFromTraverseDirection(m_modelAlignDirection);
        m_modelLength = yLength;
    } else if (zLength >= xLength && zLength >= yLength) {
        // Z-axis
        m_modelOrigin = Vector3((boundingBoxMinX + boundingBoxMaxX) * 0.5, 
            (boundingBoxMinY + boundingBoxMaxY) * 0.5, 
            boundingBoxMinZ);
        m_modelAlignDirection = Vector3(0.0, 0.0, 1.0);
        m_modelBaseNormal = StrokeMeshBuilder::calculateBaseNormalFromTraverseDirection(m_modelAlignDirection);
        m_modelLength = zLength;
    } else {
        // X-axis
        m_modelOrigin = Vector3(boundingBoxMinX, 
            (boundingBoxMinY + boundingBoxMaxY) * 0.5, 
            (boundingBoxMinZ + boundingBoxMaxZ) * 0.5);
        m_modelAlignDirection = Vector3(1.0, 0.0, 0.0);
        m_modelBaseNormal = StrokeMeshBuilder::calculateBaseNormalFromTraverseDirection(m_modelAlignDirection);
        m_modelLength = xLength;
    }
    
    std::vector<float> strokeSegmentLengths;
    float strokeLength = calculateStrokeLengths(strokeNodes, &strokeSegmentLengths);
    
    if (strokeLength > 0)
        m_scaleAmount = strokeLength / (m_modelLength + std::numeric_limits<float>::epsilon());
    
    if (!Math::isZero(m_cutRotation)) {
    	m_modelRotation.rotate(m_modelAlignDirection, Math::radiansFromDegrees(m_cutRotation * 180));
    }
    
    if (!strokeSegmentLengths.empty()) {
        float offset = 0;
        for (size_t i = 0; i < strokeSegmentLengths.size(); ++i) {
            offset += strokeSegmentLengths[i];
            m_modelJointPositions.push_back(m_modelAlignDirection * offset);
        }

        Matrix4x4 matrix;

        Vector3 rotateFromDirection = m_modelAlignDirection;
        for (size_t i = 1; i < strokeNodes.size(); ++i) {
            size_t h = i - 1;
            
            auto newDirection = (strokeNodes[i].position - strokeNodes[h].position).normalized();
            auto rotation = Quaternion::rotationTo(rotateFromDirection, newDirection);
            rotateFromDirection = newDirection;
            
            Matrix4x4 rotationMatrix;
            rotationMatrix.rotate(rotation);
            
            matrix *= rotationMatrix;
            
            m_modelTransforms.push_back(matrix);
        }
    }
    
    return true;
}

void MeshStroketifier::stroketify(std::vector<Vector3> *vertices)
{
    translate(vertices);
    scale(vertices);
    deform(vertices);
    rotate(vertices);
    transform(vertices);
}

void MeshStroketifier::stroketify(std::vector<Node> *nodes)
{
    std::vector<Vector3> positions(nodes->size());
    for (size_t i = 0; i < nodes->size(); ++i)
        positions[i] = (*nodes)[i].position;
    stroketify(&positions);
    for (size_t i = 0; i < nodes->size(); ++i) {
        auto &node = (*nodes)[i];
        node.position = positions[i];
        node.radius *= m_scaleAmount;
    }
}

float MeshStroketifier::calculateStrokeLengths(const std::vector<Node> &strokeNodes,
    std::vector<float> *lengths)
{
    float total = 0.0;
    for (size_t i = 1; i < strokeNodes.size(); ++i) {
        size_t h = i - 1;
        const auto &strokeNodeH = strokeNodes[h];
        const auto &strokeNodeI = strokeNodes[i];
        float distance = (strokeNodeH.position - strokeNodeI.position).length();
        total += distance;
        if (nullptr != lengths)
            lengths->push_back(distance);
    }
    return total;
}

void MeshStroketifier::calculateBoundingBox(const std::vector<Vector3> &vertices,
    float *minX, float *maxX,
    float *minY, float *maxY,
    float *minZ, float *maxZ)
{
    *minX = *minY = *minZ = std::numeric_limits<float>::max();
    *maxX = *maxY = *maxZ = std::numeric_limits<float>::lowest();
    for (const auto &it: vertices) {
        if (it.x() < *minX)
            *minX = it.x();
        if (it.x() > *maxX)
            *maxX = it.x();
        if (it.y() < *minY)
            *minY = it.y();
        if (it.y() > *maxY)
            *maxY = it.y();
        if (it.z() < *minZ)
            *minZ = it.z();
        if (it.z() > *maxZ)
            *maxZ = it.z();
    }
}

void MeshStroketifier::translate(std::vector<Vector3> *vertices)
{
    for (auto &it: *vertices)
        it += -m_modelOrigin;
}

void MeshStroketifier::scale(std::vector<Vector3> *vertices)
{
    for (auto &it: *vertices)
        it *= m_scaleAmount;
}

void MeshStroketifier::rotate(std::vector<Vector3> *vertices)
{
    for (auto &it: *vertices)
        it = m_modelRotation * it;
}

void MeshStroketifier::deform(std::vector<Vector3> *vertices)
{
    for (auto &position: *vertices) {
        const auto &cutDirect = m_modelAlignDirection;
        auto nodePosition = Vector3::projectPointOnLine(position, Vector3(0.0, 0.0, 0.0), cutDirect);
        auto ray = position - nodePosition;
        Vector3 sum;
        size_t count = 0;
        if (!Math::isEqual(m_deformThickness, (float)1.0)) {
            auto deformedPosition = StrokeMeshBuilder::calculateDeformPosition(position, ray, m_modelBaseNormal, m_deformThickness);
            sum += deformedPosition;
            ++count;
        }
        if (!Math::isEqual(m_deformWidth, (float)1.0)) {
            auto deformedPosition = StrokeMeshBuilder::calculateDeformPosition(position, ray, Vector3::crossProduct(m_modelBaseNormal, cutDirect), m_deformWidth);
            sum += deformedPosition;
            ++count;
        }
        if (count > 0)
            position = sum / count;
    }
}

void MeshStroketifier::transform(std::vector<Vector3> *vertices)
{
    if (m_modelJointPositions.empty() || 
            m_modelTransforms.empty() ||
            m_modelJointPositions.size() != m_modelTransforms.size()) {
        return;
    }
    
    std::set<size_t> remaining;
    for (size_t i = 0; i < vertices->size(); ++i)
        remaining.insert(i);
    
    std::vector<std::vector<size_t>> skinnedIndices;
    for (size_t i = 0; i < m_modelJointPositions.size(); ++i) {
        std::vector<size_t> indices;
        for (const auto &index: remaining) {
            if (Vector3::dotProduct(((*vertices)[index] - m_modelJointPositions[i]).normalized(), 
                    m_modelAlignDirection) < 0) {
                indices.push_back(index);
            }
        }
        for (const auto &index: indices)
            remaining.erase(index);
        skinnedIndices.push_back(indices);
    }
    if (!remaining.empty())
        skinnedIndices.back().insert(skinnedIndices.back().end(), remaining.begin(), remaining.end());
    
    for (size_t i = 0; i < skinnedIndices.size() && i < m_modelTransforms.size(); ++i) {
        const auto &matrix = m_modelTransforms[i];
        for (const auto &index: skinnedIndices[i])
            (*vertices)[index] = matrix * (*vertices)[index];
    }
}

}
