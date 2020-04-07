#include <QMatrix4x4>
#include <QQuaternion>
#include <set>
#include "meshstroketifier.h"

void MeshStroketifier::setCutRotation(float cutRotation)
{
	m_cutRotation = cutRotation;
}

bool MeshStroketifier::prepare(const std::vector<Node> &strokeNodes, 
        const std::vector<QVector3D> &vertices)
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
        m_modelOrigin = QVector3D((boundingBoxMinX + boundingBoxMaxX) * 0.5, 
            boundingBoxMinY, 
            (boundingBoxMinZ + boundingBoxMaxZ) * 0.5);
        m_modelAlignDirection = QVector3D(0.0, 1.0, 0.0);
        m_modelLength = yLength;
    } else if (zLength >= xLength && zLength >= yLength) {
        // Z-axis
        m_modelOrigin = QVector3D((boundingBoxMinX + boundingBoxMaxX) * 0.5, 
            (boundingBoxMinY + boundingBoxMaxY) * 0.5, 
            boundingBoxMinZ);
        m_modelAlignDirection = QVector3D(0.0, 0.0, 1.0);
        m_modelLength = zLength;
    } else {
        // X-axis
        m_modelOrigin = QVector3D(boundingBoxMinX, 
            (boundingBoxMinY + boundingBoxMaxY) * 0.5, 
            (boundingBoxMinZ + boundingBoxMaxZ) * 0.5);
        m_modelAlignDirection = QVector3D(1.0, 0.0, 0.0);
        m_modelLength = xLength;
    }
    
    std::vector<float> strokeSegmentLengths;
    float strokeLength = calculateStrokeLengths(strokeNodes, &strokeSegmentLengths);
    
    if (strokeLength > 0)
        m_scaleAmount = strokeLength / (m_modelLength + std::numeric_limits<float>::epsilon());
    
    if (!strokeSegmentLengths.empty()) {
        float offset = 0;
        for (size_t i = 0; i < strokeSegmentLengths.size(); ++i) {
            offset += strokeSegmentLengths[i];
            m_modelJointPositions.push_back(m_modelAlignDirection * offset);
        }

        QMatrix4x4 matrix;
        
        if (!qFuzzyIsNull(m_cutRotation)) {
			QMatrix4x4 cutRotationMatrix;
			cutRotationMatrix.rotate(m_cutRotation * 180, m_modelAlignDirection);
			matrix *= cutRotationMatrix;
		}
        
        QVector3D rotateFromDirection = m_modelAlignDirection;
        for (size_t i = 1; i < strokeNodes.size(); ++i) {
            size_t h = i - 1;
            
            auto newDirection = (strokeNodes[i].position - strokeNodes[h].position).normalized();
            auto rotation = QQuaternion::rotationTo(rotateFromDirection, newDirection);
            rotateFromDirection = newDirection;
            
            QMatrix4x4 rotationMatrix;
            rotationMatrix.rotate(rotation);
            
            matrix *= rotationMatrix;
            
            m_modelTransforms.push_back(matrix);
        }
    }
    
    return true;
}

void MeshStroketifier::stroketify(std::vector<QVector3D> *vertices)
{
    translate(vertices);
    scale(vertices);
    deform(vertices);
}

void MeshStroketifier::stroketify(std::vector<Node> *nodes)
{
    std::vector<QVector3D> positions(nodes->size());
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

void MeshStroketifier::calculateBoundingBox(const std::vector<QVector3D> &vertices,
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

void MeshStroketifier::translate(std::vector<QVector3D> *vertices)
{
    for (auto &it: *vertices)
        it += -m_modelOrigin;
}

void MeshStroketifier::scale(std::vector<QVector3D> *vertices)
{
    for (auto &it: *vertices)
        it *= m_scaleAmount;
}

void MeshStroketifier::deform(std::vector<QVector3D> *vertices)
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
            if (QVector3D::dotProduct(((*vertices)[index] - m_modelJointPositions[i]).normalized(), 
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
