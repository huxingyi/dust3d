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

#include <unordered_map>
#include <algorithm>
#include <dust3d/base/debug.h>
#include <dust3d/mesh/isotropic_halfedge_mesh.h>

namespace dust3d
{

IsotropicHalfedgeMesh::~IsotropicHalfedgeMesh()
{
    while (nullptr != m_vertexAllocLink) {
        Vertex *vertex = m_vertexAllocLink;
        m_vertexAllocLink = vertex->_allocLink;
        delete vertex;
    }
    
    while (nullptr != m_faceAllocLink) {
        Face *face = m_faceAllocLink;
        m_faceAllocLink = face->_allocLink;
        delete face;
    }
    
    while (nullptr != m_halfedgeAllocLink) {
        Halfedge *halfedge = m_halfedgeAllocLink;
        m_halfedgeAllocLink = halfedge->_allocLink;
        delete halfedge;
    }
}

IsotropicHalfedgeMesh::IsotropicHalfedgeMesh(const std::vector<Vector3> &vertices,
    const std::vector<std::vector<size_t>> &faces)
{
    std::vector<Vertex *> halfedgeVertices;
    halfedgeVertices.reserve(vertices.size());
    int sourceIndex = 0;
    for (const auto &it: vertices) {
        Vertex *vertex = newVertex();
        vertex->position = it;
        vertex->sourceIndex = sourceIndex++;
        halfedgeVertices.push_back(vertex);
    }
    
    std::vector<Face *> halfedgeFaces;
    halfedgeFaces.reserve(faces.size());
    for (const auto &it: faces) {
        Face *face = newFace();
        halfedgeFaces.push_back(face);
    }

    std::unordered_map<uint64_t, Halfedge *> halfedgeMap;
    for (size_t faceIndex = 0; faceIndex < faces.size(); ++faceIndex) {
        const auto &indices = faces[faceIndex];
        if (3 != indices.size()) {
            dust3dDebug << "Found non-triangle, face count:" << indices.size();
            continue;
        }
        std::vector<Halfedge *> halfedges(indices.size());
        for (size_t i = 0; i < 3; ++i) {
            size_t j = (i + 1) % 3;
            const auto &first = indices[i];
            const auto &second = indices[j];
            
            Vertex *vertex = halfedgeVertices[first];
            ++vertex->initialFaces;
            
            Halfedge *halfedge = newHalfedge();
            halfedge->startVertex = vertex;
            halfedge->leftFace = halfedgeFaces[faceIndex];
            
            if (nullptr == halfedge->leftFace->halfedge) {
                halfedge->leftFace->halfedge = halfedge;
            }
            if (nullptr == vertex->firstHalfedge) {
                vertex->firstHalfedge = halfedge;
            }
            
            halfedges[i] = halfedge;
            
            auto insertResult = halfedgeMap.insert({makeHalfedgeKey(first, second), halfedge});
            if (!insertResult.second) {
                dust3dDebug << "Found repeated halfedge:" << first << "," << second;
            }
        }
        linkFaceHalfedges(halfedges);
    }
    for (auto &it: halfedgeMap) {
        auto halfedgeIt = halfedgeMap.find(swapHalfedgeKey(it.first));
        if (halfedgeIt == halfedgeMap.end())
            continue;
        it.second->oppositeHalfedge = halfedgeIt->second;
        halfedgeIt->second->oppositeHalfedge = it.second;
    }
    
    for (Vertex *vertex = m_firstVertex; nullptr != vertex; vertex = vertex->nextVertex) {
        vertex->_isBoundary = false;
        vertex->_valence = vertexValence(vertex, &vertex->_isBoundary);
        if (vertex->_isBoundary) {
            if (vertex->_valence == vertex->initialFaces + 1)
                continue;
        } else {
            if (vertex->_valence == vertex->initialFaces)
                continue;
        }
        vertex->featured = true;
    }
}

void IsotropicHalfedgeMesh::linkFaceHalfedges(std::vector<IsotropicHalfedgeMesh::Halfedge *> &halfedges)
{
    for (size_t i = 0; i < halfedges.size(); ++i) {
        size_t j = (i + 1) % halfedges.size();
        halfedges[i]->nextHalfedge = halfedges[j];
        halfedges[j]->previousHalfedge = halfedges[i];
    }
}

void IsotropicHalfedgeMesh::updateFaceHalfedgesLeftFace(std::vector<IsotropicHalfedgeMesh::Halfedge *> &halfedges,
    IsotropicHalfedgeMesh::Face *leftFace)
{
    for (auto &it: halfedges)
        it->leftFace = leftFace;
}

void IsotropicHalfedgeMesh::linkHalfedgePair(IsotropicHalfedgeMesh::Halfedge *first, IsotropicHalfedgeMesh::Halfedge *second)
{
    if (nullptr != first)
        first->oppositeHalfedge = second;
    if (nullptr != second)
        second->oppositeHalfedge = first;
}

double IsotropicHalfedgeMesh::averageEdgeLength()
{
    double totalLength = 0.0;
    size_t halfedgeCount = 0;
    for (Face *face = m_firstFace; nullptr != face; face = face->nextFace) {
        const auto &startHalfedge = face->halfedge;
        Halfedge *halfedge = startHalfedge;
        do {
            const auto &nextHalfedge = halfedge->nextHalfedge;
            totalLength += (halfedge->startVertex->position - nextHalfedge->startVertex->position).length();
            ++halfedgeCount;
        } while (halfedge != startHalfedge);
    }
    if (0 == halfedgeCount)
        return 0.0;
    return totalLength / halfedgeCount;
}

IsotropicHalfedgeMesh::Face *IsotropicHalfedgeMesh::newFace()
{
    Face *face = new Face;
    
    face->_allocLink = m_faceAllocLink;
    m_faceAllocLink = face;
    
    face->debugIndex = ++m_debugFaceIndex;
    if (nullptr != m_lastFace) {
        m_lastFace->nextFace = face;
        face->previousFace = m_lastFace;
    } else {
        m_firstFace = face;
    }
    m_lastFace = face;
    return face;
}

IsotropicHalfedgeMesh::Vertex *IsotropicHalfedgeMesh::newVertex()
{
    Vertex *vertex = new Vertex;
    
    vertex->_allocLink = m_vertexAllocLink;
    m_vertexAllocLink = vertex;
    
    vertex->debugIndex = ++m_debugVertexIndex;
    if (nullptr != m_lastVertex) {
        m_lastVertex->nextVertex = vertex;
        vertex->previousVertex = m_lastVertex;
    } else {
        m_firstVertex = vertex;
    }
    m_lastVertex = vertex;
    return vertex;
}

IsotropicHalfedgeMesh::Halfedge *IsotropicHalfedgeMesh::newHalfedge()
{
    Halfedge *halfedge = new Halfedge;
    
    halfedge->_allocLink = m_halfedgeAllocLink;
    m_halfedgeAllocLink = halfedge;
    
    halfedge->debugIndex = ++m_debugHalfedgeIndex;
    return halfedge;
}

IsotropicHalfedgeMesh::Face *IsotropicHalfedgeMesh::moveToNextFace(IsotropicHalfedgeMesh::Face *face)
{
    if (nullptr == face) {
        face = m_firstFace;
        if (nullptr == face)
            return nullptr;
        if (!face->removed)
            return face;
    }
    do {
        face = face->nextFace;
    } while (nullptr != face && face->removed);
    return face;
}

IsotropicHalfedgeMesh::Vertex *IsotropicHalfedgeMesh::moveToNextVertex(IsotropicHalfedgeMesh::Vertex *vertex)
{
    if (nullptr == vertex) {
        vertex = m_firstVertex;
        if (nullptr == vertex)
            return nullptr;
        if (!vertex->removed)
            return vertex;
    }
    do {
        vertex = vertex->nextVertex;
    } while (nullptr != vertex && vertex->removed);
    return vertex;
}

void IsotropicHalfedgeMesh::breakFace(IsotropicHalfedgeMesh::Face *leftOldFace,
    IsotropicHalfedgeMesh::Halfedge *halfedge,
    IsotropicHalfedgeMesh::Vertex *breakPointVertex,
    std::vector<IsotropicHalfedgeMesh::Halfedge *> &leftNewFaceHalfedges,
    std::vector<IsotropicHalfedgeMesh::Halfedge *> &leftOldFaceHalfedges)
{
    std::vector<Halfedge *> leftFaceHalfedges = {
        halfedge->previousHalfedge,
        halfedge,
        halfedge->nextHalfedge
    };
    
    Face *leftNewFace = newFace();
    leftNewFace->halfedge = leftFaceHalfedges[2];
    leftFaceHalfedges[2]->leftFace = leftNewFace;
    
    leftNewFaceHalfedges.push_back(newHalfedge());
    leftNewFaceHalfedges.push_back(newHalfedge());
    leftNewFaceHalfedges.push_back(leftFaceHalfedges[2]);
    linkFaceHalfedges(leftNewFaceHalfedges);
    updateFaceHalfedgesLeftFace(leftNewFaceHalfedges, leftNewFace);
    leftNewFaceHalfedges[0]->startVertex = leftFaceHalfedges[0]->startVertex;
    leftNewFaceHalfedges[1]->startVertex = breakPointVertex;
    
    leftOldFaceHalfedges.push_back(leftFaceHalfedges[0]);
    leftOldFaceHalfedges.push_back(halfedge);
    leftOldFaceHalfedges.push_back(newHalfedge());
    linkFaceHalfedges(leftOldFaceHalfedges);
    updateFaceHalfedgesLeftFace(leftOldFaceHalfedges, leftOldFace);
    leftOldFaceHalfedges[2]->startVertex = breakPointVertex;
    
    breakPointVertex->firstHalfedge = leftNewFaceHalfedges[1];
    
    linkHalfedgePair(leftNewFaceHalfedges[0], leftOldFaceHalfedges[2]);
    
    leftOldFace->halfedge = leftOldFaceHalfedges[0];
}

void IsotropicHalfedgeMesh::breakEdge(IsotropicHalfedgeMesh::Halfedge *halfedge)
{
    Face *leftOldFace = halfedge->leftFace;
    Halfedge *oppositeHalfedge = halfedge->oppositeHalfedge;
    Face *rightOldFace = nullptr;
    
    if (nullptr != oppositeHalfedge)
        rightOldFace = oppositeHalfedge->leftFace;
    
    Vertex *breakPointVertex = newVertex();
    breakPointVertex->position = (halfedge->startVertex->position +
        halfedge->nextHalfedge->startVertex->position) * 0.5;
        
    breakPointVertex->featured = halfedge->startVertex->featured &&
        halfedge->nextHalfedge->startVertex->featured;

    std::vector<Halfedge *> leftNewFaceHalfedges;
    std::vector<Halfedge *> leftOldFaceHalfedges;
    breakFace(leftOldFace, halfedge, breakPointVertex,
        leftNewFaceHalfedges, leftOldFaceHalfedges);
    
    if (nullptr != rightOldFace) {
        std::vector<Halfedge *> rightNewFaceHalfedges;
        std::vector<Halfedge *> rightOldFaceHalfedges;
        breakFace(rightOldFace, oppositeHalfedge, breakPointVertex,
            rightNewFaceHalfedges, rightOldFaceHalfedges);
        linkHalfedgePair(leftOldFaceHalfedges[1], rightNewFaceHalfedges[1]);
        linkHalfedgePair(leftNewFaceHalfedges[1], rightOldFaceHalfedges[1]);
    }
}

void IsotropicHalfedgeMesh::collectVerticesAroundVertex(Vertex *vertex,
    std::set<Vertex *> *vertices)
{
    const auto &startHalfedge = vertex->firstHalfedge;
    if (nullptr == startHalfedge)
        return;
    
    Halfedge *loopHalfedge = startHalfedge;
    do {
        vertices->insert(loopHalfedge->nextHalfedge->startVertex);
        if (nullptr == loopHalfedge->oppositeHalfedge) {
            loopHalfedge = startHalfedge;
            do {
                vertices->insert(loopHalfedge->previousHalfedge->startVertex);
                loopHalfedge = loopHalfedge->previousHalfedge->oppositeHalfedge;
                if (nullptr == loopHalfedge)
                    break;
            } while (loopHalfedge != startHalfedge);
            break;
        }
        loopHalfedge = loopHalfedge->oppositeHalfedge->nextHalfedge;
    } while (loopHalfedge != startHalfedge);
}

size_t IsotropicHalfedgeMesh::vertexValence(Vertex *vertex, bool *isBoundary)
{
    const auto &startHalfedge = vertex->firstHalfedge;
    if (nullptr == startHalfedge)
        return 0;
    
    size_t valence = 0;
    
    Halfedge *loopHalfedge = startHalfedge;
    do {
        ++valence;
        if (nullptr == loopHalfedge->oppositeHalfedge) {
            if (nullptr != isBoundary)
                *isBoundary = true;
            loopHalfedge = startHalfedge;
            do {
                ++valence;
                loopHalfedge = loopHalfedge->previousHalfedge->oppositeHalfedge;
                if (nullptr == loopHalfedge)
                    break;
            } while (loopHalfedge != startHalfedge);
            break;
        }
        loopHalfedge = loopHalfedge->oppositeHalfedge->nextHalfedge;
    } while (loopHalfedge != startHalfedge);
    
    return valence;
}

bool IsotropicHalfedgeMesh::testLengthSquaredAroundVertex(Vertex *vertex, 
    const Vector3 &target, 
    double maxEdgeLengthSquared)
{
    const auto &startHalfedge = vertex->firstHalfedge;
    if (nullptr == startHalfedge)
        return false;
    
    Halfedge *loopHalfedge = startHalfedge;
    do {
        if ((loopHalfedge->nextHalfedge->startVertex->position - 
                target).lengthSquared() > maxEdgeLengthSquared) {
            return true;            
        }
        if (nullptr == loopHalfedge->oppositeHalfedge) {
            loopHalfedge = startHalfedge;
            do {
                if ((loopHalfedge->previousHalfedge->startVertex->position - 
                        target).lengthSquared() > maxEdgeLengthSquared) {
                    return true;            
                }
                loopHalfedge = loopHalfedge->previousHalfedge->oppositeHalfedge;
                if (nullptr == loopHalfedge)
                    break;
            } while (loopHalfedge != startHalfedge);
            break;
        }
        loopHalfedge = loopHalfedge->oppositeHalfedge->nextHalfedge;
    } while (loopHalfedge != startHalfedge);
    
    return false;
}

void IsotropicHalfedgeMesh::pointerVertexToNewVertex(Vertex *vertex, Vertex *replacement)
{
    const auto &startHalfedge = vertex->firstHalfedge;
    if (nullptr == startHalfedge)
        return;
    
    Halfedge *loopHalfedge = startHalfedge;
    do {
        loopHalfedge->startVertex = replacement;
        if (nullptr == loopHalfedge->oppositeHalfedge) {
            loopHalfedge = startHalfedge;
            do {
                loopHalfedge->startVertex = replacement;
                loopHalfedge = loopHalfedge->previousHalfedge->oppositeHalfedge;
                if (nullptr == loopHalfedge)
                    break;
            } while (loopHalfedge != startHalfedge);
            break;
        }
        loopHalfedge = loopHalfedge->oppositeHalfedge->nextHalfedge;
    } while (loopHalfedge != startHalfedge);
}

void IsotropicHalfedgeMesh::relaxVertex(Vertex *vertex)
{
    if (vertex->_isBoundary || vertex->_valence <= 0)
        return;
    
    const auto &startHalfedge = vertex->firstHalfedge;
    if (nullptr == startHalfedge)
        return;
    
    Vector3 position;
    
    Halfedge *loopHalfedge = startHalfedge;
    do {
        position += loopHalfedge->oppositeHalfedge->startVertex->position;
        loopHalfedge = loopHalfedge->oppositeHalfedge->nextHalfedge;
    } while (loopHalfedge != startHalfedge);
    
    position /= vertex->_valence;
    
    Vector3 projectedPosition = Vector3::projectPointOnLine(vertex->position, position, position + vertex->_normal);
    if (projectedPosition.containsNan() || projectedPosition.containsInf()) {
        vertex->position = position;
        return;
    }
    
    vertex->position = projectedPosition;
}

bool IsotropicHalfedgeMesh::flipEdge(Halfedge *halfedge)
{
    Vertex *topVertex = halfedge->previousHalfedge->startVertex;
    Vertex *bottomVertex = halfedge->oppositeHalfedge->previousHalfedge->startVertex;
    
    Vertex *leftVertex = halfedge->startVertex;
    Vertex *rightVertex = halfedge->oppositeHalfedge->startVertex;

    bool isLeftBoundary = false;
    int leftValence = (int)vertexValence(leftVertex, &isLeftBoundary);
    if (leftValence <= 3)
        return false;
    
    bool isRightBoundary = false;
    int rightValence = (int)vertexValence(rightVertex, &isRightBoundary);
    if (rightValence <= 3)
        return false;
    
    bool isTopBoundary = false;
    int topValence = (int)vertexValence(topVertex, &isTopBoundary);
    
    bool isBottomBoundary = false;
    int bottomValence = (int)vertexValence(bottomVertex, &isBottomBoundary);
    
    auto deviation = [](int valence, bool isBoundary) {
        return std::pow(valence - (isBoundary ? 4 : 6), 2);
    };
    
    int oldDeviation = deviation(topValence, isTopBoundary) +
        deviation(bottomValence, isBottomBoundary) +
        deviation(leftValence, isLeftBoundary) +
        deviation(rightValence, isRightBoundary);
    int newDeviation = deviation(topValence + 1, isTopBoundary) +
        deviation(bottomValence + 1, isBottomBoundary) +
        deviation(leftValence - 1, isLeftBoundary) +
        deviation(rightValence - 1, isRightBoundary);
    
    if (newDeviation >= oldDeviation)
        return false;
    
    Halfedge *opposite = halfedge->oppositeHalfedge;
    
    Face *topFace = halfedge->leftFace;
    Face *bottomFace = opposite->leftFace;
    
    if (leftVertex->firstHalfedge == halfedge)
        leftVertex->firstHalfedge = opposite->nextHalfedge;
    
    if (rightVertex->firstHalfedge == opposite)
        rightVertex->firstHalfedge = halfedge->nextHalfedge;
    
    halfedge->nextHalfedge->leftFace = bottomFace;
    opposite->nextHalfedge->leftFace = topFace;
    
    halfedge->startVertex = bottomVertex;
    opposite->startVertex = topVertex;
    
    topFace->halfedge = halfedge;
    bottomFace->halfedge = opposite;

    std::vector<Halfedge *> newLeftHalfedges = {
        halfedge->previousHalfedge,
        opposite->nextHalfedge,
        halfedge
    };
    std::vector<Halfedge *> newRightHalfedges = {
        opposite->previousHalfedge,
        halfedge->nextHalfedge,
        opposite,
    };
    
    linkFaceHalfedges(newLeftHalfedges);
    linkFaceHalfedges(newRightHalfedges);

    return true;
}

bool IsotropicHalfedgeMesh::collapseEdge(Halfedge *halfedge, double maxEdgeLengthSquared)
{   
    Halfedge *opposite = halfedge->oppositeHalfedge;
    
    Vertex *topVertex = halfedge->previousHalfedge->startVertex;
    Vertex *bottomVertex = opposite->previousHalfedge->startVertex;
    
    if (topVertex == bottomVertex)
        return false;
    
    if (topVertex->featured)
        return false;
    
    if (bottomVertex->featured)
        return false;
    
    Vector3 collapseTo = (halfedge->startVertex->position +
        halfedge->nextHalfedge->startVertex->position) * 0.5;
    
    if (testLengthSquaredAroundVertex(halfedge->startVertex, collapseTo, maxEdgeLengthSquared))
        return false;
    if (testLengthSquaredAroundVertex(halfedge->nextHalfedge->startVertex, collapseTo, maxEdgeLengthSquared))
        return false;
    
    std::set<Vertex *> neighborVertices;
    std::set<Vertex *> otherNeighborVertices;
    collectVerticesAroundVertex(halfedge->startVertex, &neighborVertices);
    collectVerticesAroundVertex(halfedge->nextHalfedge->startVertex, &otherNeighborVertices);
    std::vector<Vertex *> sharedNeighborVertices;
    std::set_intersection(neighborVertices.begin(), neighborVertices.end(),
        otherNeighborVertices.begin(), otherNeighborVertices.end(),
        std::back_inserter(sharedNeighborVertices));
    if (sharedNeighborVertices.size() > 2) {
        return false;
    }
    
    Vertex *leftVertex = halfedge->startVertex;
    Vertex *rightVertex = opposite->startVertex;
    Face *topFace = halfedge->leftFace;
    Face *bottomFace = opposite->leftFace;

    pointerVertexToNewVertex(leftVertex, rightVertex);
    
    if (topFace == rightVertex->firstHalfedge->leftFace || 
            bottomFace == rightVertex->firstHalfedge->leftFace) {
        rightVertex->firstHalfedge = opposite->previousHalfedge->oppositeHalfedge;
    }
    if (topFace == topVertex->firstHalfedge->leftFace || 
            bottomFace == topVertex->firstHalfedge->leftFace) {
        topVertex->firstHalfedge = halfedge->nextHalfedge->oppositeHalfedge;
    }
    if (bottomFace == bottomVertex->firstHalfedge->leftFace || 
            bottomFace == bottomVertex->firstHalfedge->leftFace) {
        bottomVertex->firstHalfedge = opposite->nextHalfedge->oppositeHalfedge;
    }
    
    linkHalfedgePair(halfedge->previousHalfedge->oppositeHalfedge,
        halfedge->nextHalfedge->oppositeHalfedge);
    linkHalfedgePair(opposite->previousHalfedge->oppositeHalfedge,
        opposite->nextHalfedge->oppositeHalfedge);
    
    rightVertex->position = collapseTo;
    
    leftVertex->removed = true;
    topFace->removed = true;
    bottomFace->removed = true;

    return true;
}

void IsotropicHalfedgeMesh::updateVertexValences()
{
    for (Vertex *vertex = m_firstVertex; nullptr != vertex; vertex = vertex->nextVertex) {
        if (vertex->removed)
            continue;
        vertex->_isBoundary = false;
        vertex->_valence = vertexValence(vertex, &vertex->_isBoundary);
    }
}

void IsotropicHalfedgeMesh::updateTriangleNormals()
{
    for (Face *face = m_firstFace; nullptr != face; face = face->nextFace) {
        if (face->removed)
            continue;
        auto &startHalfedge = face->halfedge;
        face->_normal = Vector3::normal(startHalfedge->previousHalfedge->startVertex->position,
            startHalfedge->startVertex->position,
            startHalfedge->nextHalfedge->startVertex->position);
    }
}

void IsotropicHalfedgeMesh::updateVertexNormals()
{
    for (Vertex *vertex = m_firstVertex; nullptr != vertex; vertex = vertex->nextVertex) {
        if (vertex->removed)
            continue;
        vertex->_normal = Vector3();
    }
    
    for (Face *face = m_firstFace; nullptr != face; face = face->nextFace) {
        if (face->removed)
            continue;
        auto &startHalfedge = face->halfedge;
        Vector3 faceNormal = face->_normal *
            Vector3::area(startHalfedge->previousHalfedge->startVertex->position,
                startHalfedge->startVertex->position,
                startHalfedge->nextHalfedge->startVertex->position);
        startHalfedge->previousHalfedge->startVertex->_normal += faceNormal;
        startHalfedge->startVertex->_normal += faceNormal;
        startHalfedge->nextHalfedge->startVertex->_normal += faceNormal;
    }
    
    for (Vertex *vertex = m_firstVertex; nullptr != vertex; vertex = vertex->nextVertex) {
        if (vertex->removed)
            continue;
        vertex->_normal.normalize();
    }
}

void IsotropicHalfedgeMesh::featureHalfedge(Halfedge *halfedge, double radians)
{
    if (-1 != halfedge->featureState)
        return;
    
    auto &opposite = halfedge->oppositeHalfedge;
    if (nullptr == opposite) {
        halfedge->startVertex->featured = true;
        halfedge->featureState = 1;
        return;
    }
    
    if (Vector3::angle(halfedge->leftFace->_normal, 
            opposite->leftFace->_normal) >= radians) {
        halfedge->featureState = opposite->featureState = 1;
        halfedge->startVertex->featured = opposite->startVertex->featured = true;
        return;
    }
    
    halfedge->featureState = opposite->featureState = 0;
}

void IsotropicHalfedgeMesh::featureBoundaries()
{
    for (Face *face = m_firstFace; nullptr != face; face = face->nextFace) {
        if (face->removed)
            continue;
        auto &startHalfedge = face->halfedge;
        if (nullptr == startHalfedge->previousHalfedge->oppositeHalfedge) {
            startHalfedge->previousHalfedge->startVertex->featured = true;
        }
        if (nullptr == startHalfedge->oppositeHalfedge) {
            startHalfedge->startVertex->featured = true;
        }
        if (nullptr == startHalfedge->nextHalfedge->oppositeHalfedge) {
            startHalfedge->nextHalfedge->startVertex->featured = true;
        }
    }
}

void IsotropicHalfedgeMesh::featureEdges(double radians)
{
    for (Face *face = m_firstFace; nullptr != face; face = face->nextFace) {
        if (face->removed)
            continue;
        auto &startHalfedge = face->halfedge;
        featureHalfedge(startHalfedge->previousHalfedge, radians);
        featureHalfedge(startHalfedge, radians);
        featureHalfedge(startHalfedge->nextHalfedge, radians);
    }
}

}
