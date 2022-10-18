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

#ifndef DUST3D_MESH_HOLE_WRAPPER_H_
#define DUST3D_MESH_HOLE_WRAPPER_H_

#include <deque>
#include <dust3d/base/vector3.h>
#include <map>
#include <vector>

namespace dust3d {

class HoleWrapper {
public:
    void setVertices(const std::vector<Vector3>* vertices);
    void wrap(const std::vector<std::pair<std::vector<size_t>, Vector3>>& edgeLoops);
    const std::vector<std::vector<size_t>>& newlyGeneratedFaces();
    bool finished();
    void getFailedEdgeLoops(std::vector<size_t>& failedEdgeLoops);

private:
    struct WrapItemKey {
        size_t p1;
        size_t p2;

        bool operator<(const WrapItemKey& right) const
        {
            if (p1 < right.p1)
                return true;
            if (p1 > right.p1)
                return false;
            if (p2 < right.p2)
                return true;
            if (p2 > right.p2)
                return false;
            return false;
        }
    };

    struct WrapItem {
        Vector3 baseNormal;
        size_t p1;
        size_t p2;
        size_t p3;
        bool processed;
    };

    struct Face3 {
        size_t p1;
        size_t p2;
        size_t p3;
        Vector3 normal;
        size_t index;
    };

    struct Face4 {
        size_t p1;
        size_t p2;
        size_t p3;
        size_t p4;
    };

    struct SourceVertex {
        Vector3 position;
        size_t sourcePlane;
        size_t index;
        size_t tag;
    };

    std::vector<WrapItem> m_items;
    std::map<WrapItemKey, size_t> m_itemsMap;
    std::deque<size_t> m_itemsList;
    const std::vector<Vector3>* m_positions;
    std::vector<size_t> m_candidates;
    std::vector<SourceVertex> m_sourceVertices;
    std::vector<Face3> m_generatedFaces;
    std::map<WrapItemKey, std::pair<size_t, bool>> m_generatedFaceEdgesMap;
    std::map<size_t, std::vector<size_t>> m_generatedVertexEdgesMap;
    bool m_finalizeFinished = false;
    std::vector<std::vector<size_t>> m_newlyGeneratedfaces;

    void addCandidateVertices(const std::vector<size_t>& vertices, const Vector3& planeNormal, size_t planeId);
    size_t addSourceVertex(const Vector3& position, size_t sourcePlane, size_t tag);
    void addStartup(size_t p1, size_t p2, const Vector3& baseNormal);
    Vector3 calculateFaceVector(size_t p1, size_t p2, const Vector3& baseNormal);
    void addItem(size_t p1, size_t p2, const Vector3& baseNormal);
    std::pair<size_t, bool> findItem(size_t p1, size_t p2);
    bool isEdgeGenerated(size_t p1, size_t p2);
    float angleOfBaseFaceAndPoint(size_t itemIndex, size_t vertexIndex);
    std::pair<size_t, bool> findBestVertexOnTheLeft(size_t itemIndex);
    std::pair<size_t, bool> peekItem();
    bool isEdgeClosed(size_t p1, size_t p2);
    bool isVertexClosed(size_t vertexIndex);
    void generate();
    size_t anotherVertexIndexOfFace3(const Face3& f, size_t p1, size_t p2);
    std::pair<size_t, bool> findPairFace3(const Face3& f, std::map<size_t, bool>& usedIds, std::vector<Face4>& q);
    void finalize();
    bool almostEqual(const Vector3& v1, const Vector3& v2);
};

}

#endif
