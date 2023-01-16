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

#ifndef DUST3D_BASE_OBJECT_H_
#define DUST3D_BASE_OBJECT_H_

#include <array>
#include <dust3d/base/color.h>
#include <dust3d/base/position_key.h>
#include <dust3d/base/rectangle.h>
#include <dust3d/base/uuid.h>
#include <dust3d/base/vector2.h>
#include <dust3d/base/vector3.h>
#include <map>
#include <set>
#include <unordered_map>
#include <vector>

namespace dust3d {

struct ObjectNode {
    //Uuid partId;
    //Uuid nodeId;
    Vector3 origin;
    //float radius = 0.0;
    Color color;
    float smoothCutoffDegrees = 0.0;
    //float colorSolubility = 0.0;
    //float metalness = 0.0;
    //float roughness = 1.0;
    //Uuid materialId;
    //bool countershaded = false;
    //Uuid mirrorFromPartId;
    //Uuid mirroredByPartId;
    //Vector3 direction;
    //bool joined = true;
};

class Object {
public:
    std::vector<Vector3> vertices;
    std::map<PositionKey, Uuid> positionToNodeIdMap;
    std::map<Uuid, ObjectNode> nodeMap;
    std::vector<std::vector<size_t>> triangleAndQuads;
    std::vector<std::vector<size_t>> triangles;
    std::unordered_map<Uuid, std::map<std::array<PositionKey, 3>, std::array<Vector2, 3>>> componentTriangleUvs;
    std::vector<std::map<std::array<PositionKey, 3>, std::array<Vector2, 3>>> seamTriangleUvs;
    std::vector<Vector3> triangleNormals;
    std::vector<Color> vertexColors;
    std::vector<float> vertexSmoothCutoffDegrees;
    bool alphaEnabled = false;
    uint64_t meshId = 0;

    const std::vector<std::pair<Uuid, Uuid>>* triangleSourceNodes() const
    {
        if (!m_hasTriangleSourceNodes)
            return nullptr;
        return &m_triangleSourceNodes;
    }
    void setTriangleSourceNodes(const std::vector<std::pair<Uuid, Uuid>>& sourceNodes)
    {
        m_triangleSourceNodes = sourceNodes;
        m_hasTriangleSourceNodes = true;
    }

    const std::vector<std::vector<Vector2>>* triangleVertexUvs() const
    {
        if (!m_hasTriangleVertexUvs)
            return nullptr;
        return &m_triangleVertexUvs;
    }
    void setTriangleVertexUvs(const std::vector<std::vector<Vector2>>& uvs)
    {
        m_triangleVertexUvs = uvs;
        m_hasTriangleVertexUvs = true;
    }

    const std::vector<std::vector<Vector3>>* triangleVertexNormals() const
    {
        if (!m_hasTriangleVertexNormals)
            return nullptr;
        return &m_triangleVertexNormals;
    }
    void setTriangleVertexNormals(const std::vector<std::vector<Vector3>>& normals)
    {
        m_triangleVertexNormals = normals;
        m_hasTriangleVertexNormals = true;
    }

    const std::vector<Vector3>* triangleTangents() const
    {
        if (!m_hasTriangleTangents)
            return nullptr;
        return &m_triangleTangents;
    }
    void setTriangleTangents(const std::vector<Vector3>& tangents)
    {
        m_triangleTangents = tangents;
        m_hasTriangleTangents = true;
    }

    const std::map<Uuid, std::vector<Rectangle>>* partUvRects() const
    {
        if (!m_hasPartUvRects)
            return nullptr;
        return &m_partUvRects;
    }
    void setPartUvRects(const std::map<Uuid, std::vector<Rectangle>>& uvRects)
    {
        m_partUvRects = uvRects;
        m_hasPartUvRects = true;
    }

    const std::vector<std::pair<std::pair<size_t, size_t>, std::pair<size_t, size_t>>>* triangleLinks() const
    {
        if (!m_hasTriangleLinks)
            return nullptr;
        return &m_triangleLinks;
    }
    void setTriangleLinks(const std::vector<std::pair<std::pair<size_t, size_t>, std::pair<size_t, size_t>>>& triangleLinks)
    {
        m_triangleLinks = triangleLinks;
        m_hasTriangleLinks = true;
    }

private:
    bool m_hasTriangleSourceNodes = false;
    std::vector<std::pair<Uuid, Uuid>> m_triangleSourceNodes;

    bool m_hasTriangleVertexUvs = false;
    std::vector<std::vector<Vector2>> m_triangleVertexUvs;

    bool m_hasTriangleVertexNormals = false;
    std::vector<std::vector<Vector3>> m_triangleVertexNormals;

    bool m_hasTriangleTangents = false;
    std::vector<Vector3> m_triangleTangents;

    bool m_hasPartUvRects = false;
    std::map<Uuid, std::vector<Rectangle>> m_partUvRects;

    bool m_hasTriangleLinks = false;
    std::vector<std::pair<std::pair<size_t, size_t>, std::pair<size_t, size_t>>> m_triangleLinks;
};

}

#endif
