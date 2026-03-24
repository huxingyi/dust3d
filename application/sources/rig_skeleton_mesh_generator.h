#ifndef DUST3D_APPLICATION_RIG_SKELETON_MESH_GENERATOR_H_
#define DUST3D_APPLICATION_RIG_SKELETON_MESH_GENERATOR_H_

#include "bone_structure.h"
#include <dust3d/base/color.h>
#include <dust3d/base/vector3.h>
#include <map>
#include <memory>
#include <vector>

class RigSkeletonMeshGenerator {
public:
    RigSkeletonMeshGenerator();
    ~RigSkeletonMeshGenerator();

    // Generate mesh from rig structure
    void generateMesh(const RigStructure& rigStructure, const QString& selectedBoneName = "");

    // Get the generated mesh data
    const std::vector<dust3d::Vector3>& getVertices() const;
    const std::vector<std::vector<size_t>>& getFaces() const;

    // Get per-vertex color properties for highlighting selected bone
    const std::vector<std::tuple<dust3d::Color, float, float>>* getVertexProperties() const;

    // Set the start radius for bones (default 0.1)
    void setStartRadius(double radius);
    double getStartRadius() const;

    void setNormalizeRequired(bool required);

private:
    struct BoneSegment {
        dust3d::Vector3 fromPosition;
        dust3d::Vector3 toPosition;
        double fromRadius;
        double toRadius;
    };

    // Start radius - the radius at the bone start
    // End radius will be 1/3 of start radius
    double m_startRadius = 0.0;

    bool m_normalizeRequired = true;

    std::unique_ptr<std::vector<dust3d::Vector3>> m_resultVertices;
    std::unique_ptr<std::vector<std::vector<size_t>>> m_resultQuads;
    std::unique_ptr<std::vector<std::vector<size_t>>> m_resultFaces;
    std::unique_ptr<std::vector<std::tuple<dust3d::Color, float, float>>> m_vertexProperties;
    std::map<QString, std::pair<size_t, size_t>> m_boneVertexRanges; // bone name -> (start index, end index)

    // Helper methods following the BlockMesh pattern
    std::vector<size_t> buildFace(const dust3d::Vector3& origin,
        const dust3d::Vector3& faceNormal,
        const dust3d::Vector3& startDirection,
        double radius);

    dust3d::Vector3 calculateStartDirection(const dust3d::Vector3& direction);

    void buildBone(const BoneSegment& bone, const QString& boneName);

    // Convert BoneNode to BoneSegment
    BoneSegment boneToBoneSegment(const BoneNode& bone) const;

    // Calculate bounding box of all vertices and normalize size
    struct BoundingBox {
        dust3d::Vector3 minPoint;
        dust3d::Vector3 maxPoint;
    };

    BoundingBox calculateBoundingBox() const;
    void normalizeMeshSize();
};

#endif
