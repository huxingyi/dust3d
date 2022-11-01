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

#include <dust3d/base/debug.h>
#include <dust3d/mesh/mesh_recombiner.h>
#include <dust3d/mesh/mesh_state.h>

namespace dust3d {

MeshState::MeshState(const std::vector<Vector3>& vertices, const std::vector<std::vector<size_t>>& faces)
{
    mesh = std::make_unique<MeshCombiner::Mesh>(vertices, faces);
}

MeshState::MeshState(const MeshState& other)
{
    if (nullptr != other.mesh)
        mesh = std::make_unique<MeshCombiner::Mesh>(*other.mesh);
    seamTriangleUvs = other.seamTriangleUvs;
}

void MeshState::fetch(std::vector<Vector3>& vertices, std::vector<std::vector<size_t>>& faces) const
{
    if (mesh)
        mesh->fetch(vertices, faces);
}

bool MeshState::isNull() const
{
    if (nullptr == mesh)
        return true;
    return mesh->isNull();
}

std::unique_ptr<MeshState> MeshState::combine(const MeshState& first, const MeshState& second,
    MeshCombiner::Method method)
{
    if (first.mesh->isNull() || second.mesh->isNull())
        return nullptr;
    auto newMeshState = std::make_unique<MeshState>();
    std::vector<std::pair<MeshCombiner::Source, size_t>> combinedVerticesSources;
    auto newMesh = std::unique_ptr<MeshCombiner::Mesh>(MeshCombiner::combine(*first.mesh,
        *second.mesh,
        method,
        &combinedVerticesSources));
    if (nullptr == newMesh)
        return nullptr;
    if (!newMesh->isNull()) {
        MeshRecombiner recombiner;
        std::vector<Vector3> combinedVertices;
        std::vector<std::vector<size_t>> combinedFaces;
        newMesh->fetch(combinedVertices, combinedFaces);
        recombiner.setVertices(&combinedVertices, &combinedVerticesSources);
        recombiner.setFaces(&combinedFaces);
        if (recombiner.recombine()) {
            if (MeshState::isWatertight(recombiner.regeneratedFaces())) {
                auto reMesh = std::make_unique<MeshCombiner::Mesh>(recombiner.regeneratedVertices(), recombiner.regeneratedFaces());
                if (!reMesh->isNull()) {
                    for (const auto& uvSeams : recombiner.generatedBridgingTriangleUvs()) {
                        std::map<std::array<PositionKey, 3>, std::array<Vector2, 3>> uvs;
                        for (const auto& it : uvSeams) {
                            uvs.insert(std::make_pair(std::array<PositionKey, 3> {
                                                          PositionKey(it.first[0]),
                                                          PositionKey(it.first[1]),
                                                          PositionKey(it.first[2]) },
                                it.second));
                        }
                        if (uvs.empty())
                            continue;
                        newMeshState->seamTriangleUvs.push_back(uvs);
                    }
                    newMesh = std::move(reMesh);
                }
            }
        }
    }
    if (newMesh->isNull()) {
        return nullptr;
    }
    newMeshState->mesh = std::move(newMesh);
    for (const auto& it : first.seamTriangleUvs)
        newMeshState->seamTriangleUvs.push_back(it);
    for (const auto& it : second.seamTriangleUvs)
        newMeshState->seamTriangleUvs.push_back(it);
    return newMeshState;
}

bool MeshState::isWatertight(const std::vector<std::vector<size_t>>& faces)
{
    std::set<std::pair<size_t, size_t>> halfEdges;
    for (const auto& face : faces) {
        for (size_t i = 0; i < face.size(); ++i) {
            size_t j = (i + 1) % face.size();
            auto insertResult = halfEdges.insert({ face[i], face[j] });
            if (!insertResult.second)
                return false;
        }
    }
    for (const auto& it : halfEdges) {
        if (halfEdges.find({ it.second, it.first }) == halfEdges.end())
            return false;
    }
    return true;
}

}
