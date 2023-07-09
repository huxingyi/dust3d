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

#ifndef DUST3D_MESH_MESH_GENERATOR_H_
#define DUST3D_MESH_MESH_GENERATOR_H_

#include <dust3d/base/combine_mode.h>
#include <dust3d/base/object.h>
#include <dust3d/base/position_key.h>
#include <dust3d/base/snapshot.h>
#include <dust3d/base/uuid.h>
#include <dust3d/mesh/mesh_combiner.h>
#include <dust3d/mesh/mesh_node.h>
#include <dust3d/mesh/mesh_state.h>
#include <set>
#include <tuple>
#include <unordered_map>
#include <unordered_set>

namespace dust3d {

class MeshGenerator {
public:
    static double m_minimalRadius;

    struct GeneratedPart {
        std::vector<Vector3> vertices;
        std::map<PositionKey, Uuid> positionToNodeIdMap;
        std::map<Uuid, ObjectNode> nodeMap;
        std::vector<std::vector<size_t>> faces;
        std::map<std::array<PositionKey, 3>, std::array<Vector2, 3>> triangleUvs;
        Color color = Color(1.0, 1.0, 1.0);
        float metalness = 0.0;
        float roughness = 1.0;
        bool isSuccessful = false;
        bool joined = true;
        void reset()
        {
            vertices.clear();
            faces.clear();
            triangleUvs.clear();
            positionToNodeIdMap.clear();
            nodeMap.clear();
            color = Color(1.0, 1.0, 1.0);
            metalness = 0.0;
            roughness = 1.0;
            isSuccessful = false;
            joined = true;
        }
    };

    struct GeneratedComponent {
        std::unique_ptr<MeshState> mesh;
        std::set<std::pair<PositionKey, PositionKey>> sharedQuadEdges;
        std::unordered_map<Uuid, std::map<std::array<PositionKey, 3>, std::array<Vector2, 3>>> componentTriangleUvs;
        std::set<std::array<PositionKey, 3>> brokenTriangles;
        std::set<PositionKey> noneSeamVertices;
        std::map<PositionKey, Uuid> positionToNodeIdMap;
        std::map<Uuid, ObjectNode> nodeMap;
        void reset()
        {
            mesh.reset();
            sharedQuadEdges.clear();
            componentTriangleUvs.clear();
            brokenTriangles.clear();
            noneSeamVertices.clear();
            positionToNodeIdMap.clear();
            nodeMap.clear();
        }
    };

    struct GeneratedCacheContext {
        std::map<std::string, GeneratedComponent> components;
        std::map<std::string, GeneratedPart> parts;
        std::map<std::string, std::string> partMirrorIdMap;
        std::map<std::string, std::unique_ptr<MeshState>> cachedCombination;
    };

    struct ComponentPreview {
        std::vector<Vector3> vertices;
        std::vector<std::vector<size_t>> triangles;
        std::map<std::array<PositionKey, 3>, std::array<Vector2, 3>> triangleUvs;
        Color color = Color(1.0, 1.0, 1.0);
        float metalness = 0.0;
        float roughness = 1.0;
        std::vector<std::tuple<dust3d::Color, float /*metalness*/, float /*roughness*/>> vertexProperties;
        std::vector<Vector2> cutFaceTemplate;
    };

    MeshGenerator(Snapshot* snapshot);
    ~MeshGenerator();
    bool isSuccessful();
    const std::set<Uuid>& generatedPreviewComponentIds();
    const std::map<Uuid, ComponentPreview>& generatedComponentPreviews();
    Object* takeObject();
    Snapshot* takeSnapshot();
    virtual void generate();
    void setGeneratedCacheContext(GeneratedCacheContext* cacheContext);
    void setSmoothShadingThresholdAngleDegrees(float degrees);
    void setDefaultPartColor(const Color& color);
    void setId(uint64_t id);
    uint64_t id();

protected:
    std::set<Uuid> m_generatedPreviewComponentIds;
    std::map<Uuid, ComponentPreview> m_generatedComponentPreviews;
    Object* m_object = nullptr;

private:
    Color m_defaultPartColor = Color::createWhite();
    Snapshot* m_snapshot = nullptr;
    GeneratedCacheContext* m_cacheContext = nullptr;
    std::set<std::string> m_dirtyComponentIds;
    std::set<std::string> m_dirtyPartIds;
    float m_mainProfileMiddleX = 0;
    float m_sideProfileMiddleX = 0;
    float m_mainProfileMiddleY = 0;
    std::map<std::string, std::set<std::string>> m_partNodeIds;
    std::map<std::string, std::set<std::string>> m_partEdgeIds;
    bool m_isSuccessful = false;
    bool m_cacheEnabled = false;
    float m_smoothShadingThresholdAngleDegrees = 60;
    uint64_t m_id = 0;

    void collectParts();
    void collectIncombinableMesh(const MeshState* mesh, const GeneratedComponent& componentCache);
    bool checkIsComponentDirty(const std::string& componentIdString);
    bool checkIsPartDirty(const std::string& partIdString);
    bool checkIsPartDependencyDirty(const std::string& partIdString);
    void checkDirtyFlags();
    std::unique_ptr<MeshState> combinePartMesh(const std::string& partIdString,
        const std::string& componentIdString,
        Color color,
        float smoothCutoffDegrees,
        bool* hasError);
    std::unique_ptr<MeshState> combineComponentMesh(const std::string& componentIdString, CombineMode* combineMode);
    void collectSharedQuadEdges(const std::vector<Vector3>& vertices, const std::vector<std::vector<size_t>>& faces,
        std::set<std::pair<PositionKey, PositionKey>>* sharedQuadEdges);
    const std::map<std::string, std::string>* findComponent(const std::string& componentIdString);
    CombineMode componentCombineMode(const std::map<std::string, std::string>* component);
    std::unique_ptr<MeshState> combineComponentChildGroupMesh(const std::vector<std::string>& componentIdStrings,
        GeneratedComponent& componentCache,
        std::set<std::array<PositionKey, 3>>* brokenTriangles);
    std::unique_ptr<MeshState> combineMultipleMeshes(std::vector<std::tuple<std::unique_ptr<MeshState>, CombineMode, std::string>>&& multipleMeshes,
        std::set<std::array<PositionKey, 3>>* brokenTriangles);
    std::unique_ptr<MeshState> combineStitchingMesh(const std::string& componentIdString,
        const std::vector<std::string>& partIdStrings,
        const std::vector<std::string>& componentIdStrings,
        bool frontClosed,
        bool backClosed,
        bool sideClosed,
        size_t targetSegments,
        Color color,
        float smoothCutoffDegrees,
        GeneratedComponent& componentCache);
    void collectUncombinedComponent(const std::string& componentIdString);
    void collectBrokenTriangles(const std::string& componentIdString);
    void cutFaceStringToCutTemplate(const std::string& cutFaceString, std::vector<Vector2>& cutTemplate);
    void postprocessObject(Object* object);
    void preprocessMirror();
    std::string reverseUuid(const std::string& uuidString);
    void recoverQuads(const std::vector<Vector3>& vertices, const std::vector<std::vector<size_t>>& triangles, const std::set<std::pair<PositionKey, PositionKey>>& sharedQuadEdges, std::vector<std::vector<size_t>>& triangleAndQuads);
    void addComponentPreview(const Uuid& componentId, ComponentPreview&& preview);
    bool fetchPartOrderedNodes(const std::string& partIdString, std::vector<MeshNode>* meshNodes, bool* isCircle);

    static void chamferFace(std::vector<Vector2>* face);
    static void subdivideFace(std::vector<Vector2>* face);
    static void flattenLinks(const std::unordered_map<size_t, size_t>& links,
        std::vector<size_t>* array,
        bool* isCircle);
};

}

#endif
