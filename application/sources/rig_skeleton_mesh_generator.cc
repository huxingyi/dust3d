#include "rig_skeleton_mesh_generator.h"
#include <cmath>

RigSkeletonMeshGenerator::RigSkeletonMeshGenerator()
    : m_startRadius(0.1)
    , m_resultVertices(new std::vector<dust3d::Vector3>)
    , m_resultQuads(new std::vector<std::vector<size_t>>)
    , m_resultFaces(new std::vector<std::vector<size_t>>)
    , m_vertexProperties(new std::vector<std::tuple<dust3d::Color, float, float>>)
{
}

RigSkeletonMeshGenerator::~RigSkeletonMeshGenerator()
{
    // unique_ptr will automatically clean up
}

void RigSkeletonMeshGenerator::setStartRadius(double radius)
{
    m_startRadius = radius;
}

double RigSkeletonMeshGenerator::getStartRadius() const
{
    return m_startRadius;
}

const std::vector<dust3d::Vector3>& RigSkeletonMeshGenerator::getVertices() const
{
    return *m_resultVertices;
}

const std::vector<std::vector<size_t>>& RigSkeletonMeshGenerator::getFaces() const
{
    return *m_resultFaces;
}

const std::vector<std::tuple<dust3d::Color, float, float>>* RigSkeletonMeshGenerator::getVertexProperties() const
{
    return m_vertexProperties->empty() ? nullptr : m_vertexProperties.get();
}

RigSkeletonMeshGenerator::BoneSegment RigSkeletonMeshGenerator::boneToBoneSegment(const BoneNode& bone) const
{
    BoneSegment segment;
    segment.fromPosition = dust3d::Vector3(bone.posX, bone.posY, bone.posZ);
    segment.toPosition = dust3d::Vector3(bone.endX, bone.endY, bone.endZ);
    segment.fromRadius = m_startRadius;
    segment.toRadius = m_startRadius / 3.0;  // End radius is 1/3 of start radius
    return segment;
}

std::vector<size_t> RigSkeletonMeshGenerator::buildFace(const dust3d::Vector3& origin,
    const dust3d::Vector3& faceNormal,
    const dust3d::Vector3& startDirection,
    double radius)
{
    std::vector<size_t> face;
    face.push_back(m_resultVertices->size() + 0);
    face.push_back(m_resultVertices->size() + 1);
    face.push_back(m_resultVertices->size() + 2);
    face.push_back(m_resultVertices->size() + 3);

    auto upDirection = dust3d::Vector3::crossProduct(startDirection, faceNormal);

    m_resultVertices->push_back(origin + startDirection * radius);
    m_resultVertices->push_back(origin - upDirection * radius);
    m_resultVertices->push_back(origin - startDirection * radius);
    m_resultVertices->push_back(origin + upDirection * radius);

    return face;
}

dust3d::Vector3 RigSkeletonMeshGenerator::calculateStartDirection(const dust3d::Vector3& direction)
{
    const std::vector<dust3d::Vector3> axisList = {
        dust3d::Vector3(1, 0, 0),
        dust3d::Vector3(0, 1, 0),
        dust3d::Vector3(0, 0, 1),
    };
    double maxDot = -1;
    size_t nearAxisIndex = 0;
    bool reversed = false;
    for (size_t i = 0; i < axisList.size(); ++i) {
        const auto axis = axisList[i];
        auto dot = dust3d::Vector3::dotProduct(axis, direction);
        auto positiveDot = std::abs(dot);
        if (positiveDot >= maxDot) {
            reversed = dot < 0;
            maxDot = positiveDot;
            nearAxisIndex = i;
        }
    }
    const auto& chosenAxis = axisList[(nearAxisIndex + 1) % 3];
    auto startDirection = dust3d::Vector3::crossProduct(direction, chosenAxis).normalized();
    return reversed ? -startDirection : startDirection;
}

void RigSkeletonMeshGenerator::buildBone(const BoneSegment& bone, const QString& boneName)
{
    size_t startVertexIndex = m_resultVertices->size();
    
    dust3d::Vector3 fromFaceNormal = (bone.toPosition - bone.fromPosition).normalized();
    dust3d::Vector3 startDirection = calculateStartDirection(-fromFaceNormal);
    std::vector<size_t> fromFaces = buildFace(bone.fromPosition,
        -fromFaceNormal, startDirection, bone.fromRadius);
    std::vector<size_t> toFaces = buildFace(bone.toPosition,
        -fromFaceNormal, startDirection, bone.toRadius);

    m_resultQuads->push_back(fromFaces);
    for (size_t i = 0; i < fromFaces.size(); ++i) {
        size_t j = (i + 1) % fromFaces.size();
        m_resultQuads->push_back({fromFaces[j], fromFaces[i], toFaces[i], toFaces[j]});
    }
    std::reverse(toFaces.begin(), toFaces.end());
    m_resultQuads->push_back(toFaces);
    
    size_t endVertexIndex = m_resultVertices->size();
    m_boneVertexRanges[boneName] = std::make_pair(startVertexIndex, endVertexIndex);
}

void RigSkeletonMeshGenerator::generateMesh(const RigStructure& rigStructure, const QString& selectedBoneName)
{
    // Clear previous results
    m_resultVertices->clear();
    m_resultQuads->clear();
    m_resultFaces->clear();
    m_boneVertexRanges.clear();
    m_vertexProperties->clear();

    // Build mesh for each bone
    for (const auto& bone : rigStructure.bones) {
        BoneSegment segment = boneToBoneSegment(bone);
        buildBone(segment, bone.name);
    }

    // Convert quads to faces (split each quad into two triangles)
    m_resultFaces->reserve(m_resultQuads->size() * 2);
    for (const auto& quad : *m_resultQuads) {
        if (quad.size() == 4) {
            // Split quad into two triangles
            m_resultFaces->push_back({quad[0], quad[1], quad[2]});
            m_resultFaces->push_back({quad[2], quad[3], quad[0]});
        } else if (quad.size() == 3) {
            // Already a triangle
            m_resultFaces->push_back(quad);
        }
    }
    
    // Generate per-vertex properties if a bone is selected
    if (!selectedBoneName.isEmpty()) {
        m_vertexProperties->resize(m_resultVertices->size());
        
        auto selectedBoneIt = m_boneVertexRanges.find(selectedBoneName);
        size_t selectedStartIdx = (selectedBoneIt != m_boneVertexRanges.end()) ? selectedBoneIt->second.first : -1;
        size_t selectedEndIdx = (selectedBoneIt != m_boneVertexRanges.end()) ? selectedBoneIt->second.second : -1;
        
        // Default color for non-selected bones (light gray)
        dust3d::Color defaultColor(0.8f, 0.8f, 0.8f);
        // Highlight color for selected bone (bright orange/yellow)
        dust3d::Color highlightColor(1.0f, 0.85f, 0.0f);
        
        for (size_t i = 0; i < m_resultVertices->size(); ++i) {
            bool isSelected = (selectedStartIdx != (size_t)-1 && i >= selectedStartIdx && i < selectedEndIdx);
            dust3d::Color vertexColor = isSelected ? highlightColor : defaultColor;
            (*m_vertexProperties)[i] = std::make_tuple(vertexColor, 0.0f, 1.0f);
        }
    }
}
