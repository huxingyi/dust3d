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

#include <dust3d/base/position_key.h>
#include <dust3d/mesh/mesh_combiner.h>
#include <dust3d/mesh/solid_mesh_boolean_operation.h>
#include <dust3d/mesh/triangulate.h>

namespace dust3d {

MeshCombiner::Mesh::Mesh(const std::vector<Vector3>& vertices, const std::vector<std::vector<size_t>>& faces)
{
    m_vertices = std::make_unique<std::vector<Vector3>>(vertices);
    m_triangles = std::make_unique<std::vector<std::vector<size_t>>>();
    triangulate(vertices, faces, m_triangles.get());
    m_solidMesh = std::make_unique<SolidMesh>();
    m_solidMesh->setVertices(m_vertices.get());
    m_solidMesh->setTriangles(m_triangles.get());
    m_solidMesh->prepare();
}

MeshCombiner::Mesh::Mesh(const Mesh& other)
{
    m_vertices = std::make_unique<std::vector<Vector3>>();
    m_triangles = std::make_unique<std::vector<std::vector<size_t>>>();
    other.fetch(*m_vertices, *m_triangles);
    m_solidMesh = std::make_unique<SolidMesh>();
    m_solidMesh->setVertices(m_vertices.get());
    m_solidMesh->setTriangles(m_triangles.get());
    m_solidMesh->prepare();
}

MeshCombiner::Mesh::~Mesh()
{
}

void MeshCombiner::Mesh::fetch(std::vector<Vector3>& vertices, std::vector<std::vector<size_t>>& faces) const
{
    if (nullptr != m_vertices)
        vertices = *m_vertices;

    if (nullptr != m_triangles)
        faces = *m_triangles;
}

bool MeshCombiner::Mesh::isNull() const
{
    return nullptr == m_vertices || m_vertices->empty();
}

MeshCombiner::Mesh* MeshCombiner::combine(const Mesh& firstMesh, const Mesh& secondMesh, Method method,
    std::vector<std::pair<Source, size_t>>* combinedVerticesComeFrom)
{
    if (firstMesh.isNull() || secondMesh.isNull())
        return nullptr;

    SolidMeshBooleanOperation booleanOperation(firstMesh.m_solidMesh.get(), secondMesh.m_solidMesh.get());
    if (!booleanOperation.combine())
        return nullptr;

    std::map<PositionKey, std::pair<Source, size_t>> verticesSourceMap;

    auto addToSourceMap = [&](SolidMesh* solidMesh, Source source) {
        size_t vertexIndex = 0;
        const std::vector<Vector3>* vertices = solidMesh->vertices();
        if (nullptr == vertices)
            return;
        for (const auto& point : *vertices) {
            verticesSourceMap.insert({ { point.x(), point.y(), point.z() },
                { source, vertexIndex } });
            ++vertexIndex;
        }
    };
    if (nullptr != combinedVerticesComeFrom) {
        addToSourceMap(firstMesh.m_solidMesh.get(), Source::First);
        addToSourceMap(secondMesh.m_solidMesh.get(), Source::Second);
    }

    std::vector<std::vector<size_t>> resultTriangles;
    if (Method::Union == method) {
        booleanOperation.fetchUnion(resultTriangles);
    } else if (Method::Diff == method) {
        booleanOperation.fetchDiff(resultTriangles);
    } else {
        return nullptr;
    }

    const auto& resultVertices = booleanOperation.resultVertices();
    if (nullptr != combinedVerticesComeFrom) {
        combinedVerticesComeFrom->clear();
        for (const auto& point : resultVertices) {
            auto findSource = verticesSourceMap.find(PositionKey(point.x(), point.y(), point.z()));
            if (findSource == verticesSourceMap.end()) {
                combinedVerticesComeFrom->push_back({ Source::None, 0 });
            } else {
                combinedVerticesComeFrom->push_back(findSource->second);
            }
        }
    }

    return new Mesh(resultVertices, resultTriangles);
}

}
