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

#include <string>
#include <iostream>
#include <dust3d/mesh/isotropic_remesher.h>
#include <dust3d/mesh/isotropic_halfedge_mesh.h>

namespace dust3d
{

IsotropicRemesher::~IsotropicRemesher()
{
    delete m_halfedgeMesh;
    delete m_axisAlignedBoundingBoxTree;
    delete m_triangleBoxes;
    delete m_triangleNormals;
}

void IsotropicRemesher::setSharpEdgeIncludedAngle(double degrees)
{
    m_sharpEdgeThresholdRadians = (180 - degrees) * (Math::Pi / 180.0);
}

void IsotropicRemesher::setTargetEdgeLength(double edgeLength)
{
    m_targetEdgeLength = edgeLength;
}

void IsotropicRemesher::setTargetTriangleCount(size_t triangleCount)
{
    m_targetTriangleCount = triangleCount;
}

void IsotropicRemesher::buildAxisAlignedBoundingBoxTree()
{
    m_triangleBoxes = new std::vector<AxisAlignedBoudingBox>(m_triangles->size());
    
    for (size_t i = 0; i < (*m_triangleBoxes).size(); ++i) {
        addTriagleToAxisAlignedBoundingBox((*m_triangles)[i], &(*m_triangleBoxes)[i]);
        (*m_triangleBoxes)[i].updateCenter();
    }
    
    std::vector<size_t> faceIndices;
    for (size_t i = 0; i < (*m_triangleBoxes).size(); ++i)
        faceIndices.push_back(i);
    
    AxisAlignedBoudingBox groupBox;
    for (const auto &i: faceIndices) {
        addTriagleToAxisAlignedBoundingBox((*m_triangles)[i], &groupBox);
    }
    groupBox.updateCenter();
    
    delete m_axisAlignedBoundingBoxTree;
    m_axisAlignedBoundingBoxTree = new AxisAlignedBoudingBoxTree(m_triangleBoxes, 
        faceIndices, groupBox);
}

IsotropicRemesher::IsotropicRemesher(const std::vector<Vector3> *vertices,
        const std::vector<std::vector<size_t>> *triangles) :
    m_vertices(vertices),
    m_triangles(triangles)
{
    m_halfedgeMesh = new IsotropicHalfedgeMesh(*m_vertices, *m_triangles);
    m_initialAverageEdgeLength = m_halfedgeMesh->averageEdgeLength();
}

double IsotropicRemesher::initialAverageEdgeLength()
{
    return m_initialAverageEdgeLength;
}

void IsotropicRemesher::remesh(size_t iteration)
{
    delete m_triangleNormals;
    m_triangleNormals = new std::vector<Vector3>;
    m_triangleNormals->reserve(m_triangles->size());
    for (const auto &it: *m_triangles) {
        m_triangleNormals->push_back(
            Vector3::normal((*m_vertices)[it[0]], 
                (*m_vertices)[it[1]], 
                (*m_vertices)[it[2]])
        );
    }

    double targetLength = m_targetEdgeLength > 0 ? m_targetEdgeLength : m_initialAverageEdgeLength;
    
    if (m_targetTriangleCount > 0) {
        double totalArea = 0.0;
        for (const auto &it: *m_triangles) {
            totalArea += Vector3::area((*m_vertices)[it[0]], (*m_vertices)[it[1]], (*m_vertices)[it[2]]);
        }
        double triangleArea = totalArea / m_targetTriangleCount;
        targetLength = std::sqrt(triangleArea / (0.86602540378 * 0.5));
    }

    double minTargetLength = 4.0 / 5.0 * targetLength;
    double maxTargetLength= 4.0 / 3.0 * targetLength;
    
    double minTargetLengthSquared = minTargetLength * minTargetLength;
    double maxTargetLengthSquared = maxTargetLength * maxTargetLength;
    
    buildAxisAlignedBoundingBoxTree();
    
    bool skipSplitOnce = true;
    if (m_sharpEdgeThresholdRadians > 0) {
        splitLongEdges(maxTargetLengthSquared);
        m_halfedgeMesh->updateTriangleNormals();
        m_halfedgeMesh->featureEdges(m_sharpEdgeThresholdRadians);
    } else {
        splitLongEdges(maxTargetLengthSquared);
        m_halfedgeMesh->featureBoundaries();
    }
    
    for (size_t i = 0; i < iteration; ++i) {
        if (skipSplitOnce) {
            skipSplitOnce = false;
        } else {
            splitLongEdges(maxTargetLengthSquared);
        }
        collapseShortEdges(minTargetLengthSquared, maxTargetLengthSquared);
        flipEdges();
        shiftVertices();
        projectVertices();
        
    }
}

void IsotropicRemesher::splitLongEdges(double maxEdgeLengthSquared)
{
    for (IsotropicHalfedgeMesh::Face *face = m_halfedgeMesh->moveToNextFace(nullptr); 
            nullptr != face; 
            ) {
        const auto &startHalfedge = face->halfedge;
        face = m_halfedgeMesh->moveToNextFace(face);
        IsotropicHalfedgeMesh::Halfedge *halfedge = startHalfedge;
        do {
            const auto &nextHalfedge = halfedge->nextHalfedge;
            double lengthSquared = (halfedge->startVertex->position - nextHalfedge->startVertex->position).lengthSquared();
            if (lengthSquared > maxEdgeLengthSquared) {
                m_halfedgeMesh->breakEdge(halfedge);
                break;
            }
            halfedge = nextHalfedge;
        } while (halfedge != startHalfedge);
    }
}

IsotropicHalfedgeMesh *IsotropicRemesher::remeshedHalfedgeMesh()
{
    return m_halfedgeMesh;
}

void IsotropicRemesher::collapseShortEdges(double minEdgeLengthSquared, double maxEdgeLengthSquared)
{
    for (IsotropicHalfedgeMesh::Face *face = m_halfedgeMesh->moveToNextFace(nullptr); 
            nullptr != face; 
            ) {
        if (face->removed) {
            face = m_halfedgeMesh->moveToNextFace(face);
            continue;
        }
        size_t f = face->debugIndex;
        const auto &startHalfedge = face->halfedge;
        face = m_halfedgeMesh->moveToNextFace(face);
        IsotropicHalfedgeMesh::Halfedge *halfedge = startHalfedge;
        size_t loopCount = 0;
        do {
            const auto &nextHalfedge = halfedge->nextHalfedge;
            double lengthSquared = (halfedge->startVertex->position - nextHalfedge->startVertex->position).lengthSquared();
            if (lengthSquared < minEdgeLengthSquared) {
                if (!halfedge->startVertex->featured && !nextHalfedge->startVertex->featured) {
                    if (m_halfedgeMesh->collapseEdge(halfedge, maxEdgeLengthSquared)) {
                        break;
                    }
                }
            }
            halfedge = nextHalfedge;
        } while (halfedge != startHalfedge);
    }
}

void IsotropicRemesher::flipEdges()
{
    for (IsotropicHalfedgeMesh::Face *face = m_halfedgeMesh->moveToNextFace(nullptr); 
            nullptr != face; 
            ) {
        const auto &startHalfedge = face->halfedge;
        face = m_halfedgeMesh->moveToNextFace(face);
        IsotropicHalfedgeMesh::Halfedge *halfedge = startHalfedge;
        do {
            const auto &nextHalfedge = halfedge->nextHalfedge;
            if (nullptr != halfedge->oppositeHalfedge) {
                if (!halfedge->startVertex->featured && !nextHalfedge->startVertex->featured) {
                    if (m_halfedgeMesh->flipEdge(halfedge)) {
                        break;
                    }
                }
            }
            halfedge = nextHalfedge;
        } while (halfedge != startHalfedge);
    }
}

void IsotropicRemesher::shiftVertices()
{
    m_halfedgeMesh->updateVertexValences();
    m_halfedgeMesh->updateTriangleNormals();
    m_halfedgeMesh->updateVertexNormals();
    
    for (IsotropicHalfedgeMesh::Vertex *vertex = m_halfedgeMesh->moveToNextVertex(nullptr); 
            nullptr != vertex;
            vertex = m_halfedgeMesh->moveToNextVertex(vertex)) {
        if (vertex->featured)
            continue;
        m_halfedgeMesh->relaxVertex(vertex);
    }
}

void IsotropicRemesher::projectVertices()
{
    for (IsotropicHalfedgeMesh::Vertex *vertex = m_halfedgeMesh->moveToNextVertex(nullptr); 
            nullptr != vertex;
            vertex = m_halfedgeMesh->moveToNextVertex(vertex)) {
        if (vertex->featured)
            continue;
        
        const auto &startHalfedge = vertex->firstHalfedge;
        if (nullptr == startHalfedge)
            continue;

        std::vector<AxisAlignedBoudingBox> rayBox(1);
        auto &box = rayBox[0];
        box.update(vertex->position);

        IsotropicHalfedgeMesh::Halfedge *loopHalfedge = startHalfedge;
        do {
            box.update(loopHalfedge->nextHalfedge->startVertex->position);
            if (nullptr == loopHalfedge->oppositeHalfedge) {
                loopHalfedge = startHalfedge;
                do {
                    box.update(loopHalfedge->previousHalfedge->startVertex->position);
                    loopHalfedge = loopHalfedge->previousHalfedge->oppositeHalfedge;
                    if (nullptr == loopHalfedge)
                        break;
                } while (loopHalfedge != startHalfedge);
                break;
            }
            loopHalfedge = loopHalfedge->oppositeHalfedge->nextHalfedge;
        } while (loopHalfedge != startHalfedge);
        
        AxisAlignedBoudingBoxTree testTree(&rayBox,
            {0},
            rayBox[0]);
        std::vector<std::pair<size_t, size_t>> pairs;
        m_axisAlignedBoundingBoxTree->test(m_axisAlignedBoundingBoxTree->root(), testTree.root(), &rayBox, &pairs);
        
        std::vector<std::pair<Vector3, double>> hits;
        
        auto boundingBoxSize = box.upperBound() - box.lowerBound();
        Vector3 segment = vertex->_normal * (boundingBoxSize[0] + boundingBoxSize[1] + boundingBoxSize[2]);
        for (const auto &it: pairs) {
            const auto &triangle = (*m_triangles)[it.first];
            std::vector<Vector3> trianglePositions = {
                (*m_vertices)[triangle[0]],
                (*m_vertices)[triangle[1]],
                (*m_vertices)[triangle[2]]
            };
            Vector3 intersection;
            if (Vector3::intersectSegmentAndPlane(vertex->position - segment, vertex->position + segment,
                    trianglePositions[0], 
                    (*m_triangleNormals)[it.first],
                    &intersection)) {
                std::vector<Vector3> normals;
                for (size_t i = 0; i < 3; ++i) {
                    size_t j = (i + 1) % 3;
                    normals.push_back(Vector3::normal(intersection, trianglePositions[i], trianglePositions[j]));
                }
                if (Vector3::dotProduct(normals[0], normals[1]) > 0 && 
                        Vector3::dotProduct(normals[0], normals[2]) > 0) {
                    hits.push_back({intersection, (vertex->position - intersection).lengthSquared()});
                }
            }
        }
        
        if (!hits.empty()) {
            vertex->position = std::min_element(hits.begin(), hits.end(), [](const std::pair<Vector3, double> &first,
                    const std::pair<Vector3, double> &second) {
                return first.second < second.second;
            })->first;
        }
    }
}

}
