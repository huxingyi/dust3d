#include "rig_skeleton_mesh_generator.h"
#include "theme.h"
#include <cmath>
#include <dust3d/mesh/rope_mesh.h>

RigSkeletonMeshGenerator::RigSkeletonMeshGenerator()
    : m_startRadius(0.05)
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
    segment.toRadius = m_startRadius / 5.0; // End radius is 1/5 of start radius
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
        m_resultQuads->push_back({ fromFaces[j], fromFaces[i], toFaces[i], toFaces[j] });
    }
    std::reverse(toFaces.begin(), toFaces.end());
    m_resultQuads->push_back(toFaces);

    size_t endVertexIndex = m_resultVertices->size();
    m_boneVertexRanges[boneName] = std::make_pair(startVertexIndex, endVertexIndex);
}

void RigSkeletonMeshGenerator::buildRing(const BoneNode& bone)
{
    dust3d::Vector3 from(bone.posX, bone.posY, bone.posZ);
    dust3d::Vector3 to(bone.endX, bone.endY, bone.endZ);
    dust3d::Vector3 center = (from + to) * 0.5;
    dust3d::Vector3 axis = to - from;
    double axisLength = axis.length();
    if (axisLength < 1e-6)
        return;

    axis = axis / axisLength;
    dust3d::Vector3 arbitraryBasis = std::abs(axis.x()) < 0.9 ? dust3d::Vector3(1, 0, 0) : dust3d::Vector3(0, 1, 0);
    dust3d::Vector3 u = dust3d::Vector3::crossProduct(axis, arbitraryBasis).normalized();
    dust3d::Vector3 v = dust3d::Vector3::crossProduct(axis, u).normalized();

    double ringRadius = std::max(0.01, static_cast<double>(bone.capsuleRadius) * 1.25);
    size_t segmentCount = 32;
    std::vector<dust3d::Vector3> ringPositions;
    ringPositions.reserve(segmentCount);
    for (size_t i = 0; i < segmentCount; ++i) {
        double theta = 2.0 * M_PI * static_cast<double>(i) / static_cast<double>(segmentCount);
        double cosTheta = std::cos(theta);
        double sinTheta = std::sin(theta);
        ringPositions.emplace_back(center + u * (ringRadius * cosTheta) + v * (ringRadius * sinTheta));
    }

    dust3d::RopeMesh::BuildParameters ropeParams;
    ropeParams.defaultRadius = 0.001;
    ropeParams.sectionSegments = 3;
    dust3d::RopeMesh ropeMesh(ropeParams);
    ropeMesh.addRope(ringPositions, true);

    size_t ringStart = m_resultVertices->size();
    for (const auto& vertex : ropeMesh.resultVertices()) {
        m_resultVertices->push_back(vertex);
    }
    size_t ringEnd = m_resultVertices->size();
    m_ringVertexRanges.emplace_back(ringStart, ringEnd);

    for (const auto& tri : ropeMesh.resultTriangles()) {
        m_resultFaces->push_back({ ringStart + tri[0], ringStart + tri[1], ringStart + tri[2] });
    }
}

RigSkeletonMeshGenerator::BoundingBox RigSkeletonMeshGenerator::calculateBoundingBox() const
{
    BoundingBox bbox;

    if (m_resultVertices->empty()) {
        bbox.minPoint = dust3d::Vector3(0, 0, 0);
        bbox.maxPoint = dust3d::Vector3(0, 0, 0);
        return bbox;
    }

    // Initialize with first vertex
    bbox.minPoint = (*m_resultVertices)[0];
    bbox.maxPoint = (*m_resultVertices)[0];

    // Find min and max for each axis
    for (const auto& vertex : *m_resultVertices) {
        if (vertex.x() < bbox.minPoint.x())
            bbox.minPoint.setX(vertex.x());
        if (vertex.y() < bbox.minPoint.y())
            bbox.minPoint.setY(vertex.y());
        if (vertex.z() < bbox.minPoint.z())
            bbox.minPoint.setZ(vertex.z());

        if (vertex.x() > bbox.maxPoint.x())
            bbox.maxPoint.setX(vertex.x());
        if (vertex.y() > bbox.maxPoint.y())
            bbox.maxPoint.setY(vertex.y());
        if (vertex.z() > bbox.maxPoint.z())
            bbox.maxPoint.setZ(vertex.z());
    }

    return bbox;
}

void RigSkeletonMeshGenerator::normalizeMeshSize()
{
    if (m_resultVertices->empty()) {
        return;
    }

    // Calculate bounding box
    BoundingBox bbox = calculateBoundingBox();

    // Calculate the size of the rig in each dimension
    double sizeX = bbox.maxPoint.x() - bbox.minPoint.x();
    double sizeY = bbox.maxPoint.y() - bbox.minPoint.y();
    double sizeZ = bbox.maxPoint.z() - bbox.minPoint.z();

    // Find the largest dimension
    double maxSize = std::max({ sizeX, sizeY, sizeZ });

    // Target size - normalize all rigs to have their largest dimension be 1.5 units
    const double TARGET_SIZE = 1.5;

    if (maxSize > 0.001) { // Avoid division by very small numbers
        double scaleFactor = TARGET_SIZE / maxSize;

        // Calculate the center of the bounding box
        dust3d::Vector3 center = (bbox.minPoint + bbox.maxPoint) * 0.5;

        // Scale all vertices around the center
        for (auto& vertex : *m_resultVertices) {
            // Translate to origin
            vertex = vertex - center;
            // Scale
            vertex = vertex * scaleFactor;
        }
    }
}

void RigSkeletonMeshGenerator::generateMesh(const RigStructure& rigStructure, const QString& selectedBoneName)
{
    // Clear previous results
    m_resultVertices->clear();
    m_resultQuads->clear();
    m_resultFaces->clear();
    m_boneVertexRanges.clear();
    m_ringVertexRanges.clear();
    m_vertexProperties->clear();

    // Build mesh for each bone
    for (const auto& bone : rigStructure.bones) {
        BoneSegment segment = boneToBoneSegment(bone);
        buildBone(segment, bone.name);
        buildRing(bone);
    }

    if (m_normalizeRequired) {
        // Normalize the mesh size to ensure all rig types appear at similar scale
        normalizeMeshSize();
    }

    // Convert quads to faces (split each quad into two triangles)
    m_resultFaces->reserve(m_resultQuads->size() * 2 + m_resultFaces->size());
    for (const auto& quad : *m_resultQuads) {
        if (quad.size() == 4) {
            // Split quad into two triangles
            m_resultFaces->push_back({ quad[0], quad[1], quad[2] });
            m_resultFaces->push_back({ quad[2], quad[3], quad[0] });
        } else if (quad.size() == 3) {
            // Already a triangle
            m_resultFaces->push_back(quad);
        }
    }

    // Generate per-vertex color/alpha properties for bones and ring
    m_vertexProperties->resize(m_resultVertices->size());

    auto selectedBoneIt = m_boneVertexRanges.find(selectedBoneName);
    size_t selectedStartIdx = (selectedBoneIt != m_boneVertexRanges.end()) ? selectedBoneIt->second.first : static_cast<size_t>(-1);
    size_t selectedEndIdx = (selectedBoneIt != m_boneVertexRanges.end()) ? selectedBoneIt->second.second : static_cast<size_t>(-1);

    dust3d::Color defaultColor(Theme::green.redF(), Theme::green.greenF(), Theme::green.blueF(), 1.0);
    dust3d::Color highlightColor(Theme::red.redF(), Theme::red.greenF(), Theme::red.blueF(), 1.0);
    dust3d::Color ringColor(Theme::green.redF(), Theme::green.greenF(), Theme::green.blueF(), 1.0);

    for (size_t i = 0; i < m_resultVertices->size(); ++i) {
        (*m_vertexProperties)[i] = std::make_tuple(defaultColor, 0.0f, 1.0f);
    }

    if (!selectedBoneName.isEmpty() && selectedBoneIt != m_boneVertexRanges.end()) {
        for (size_t i = selectedStartIdx; i < selectedEndIdx && i < m_resultVertices->size(); ++i) {
            (*m_vertexProperties)[i] = std::make_tuple(highlightColor, 0.0f, 1.0f);
        }
    }

    for (const auto& ringRange : m_ringVertexRanges) {
        for (size_t i = ringRange.first; i < ringRange.second && i < m_resultVertices->size(); ++i) {
            (*m_vertexProperties)[i] = std::make_tuple(ringColor, 0.0f, 1.0f);
        }
    }
}

void RigSkeletonMeshGenerator::setNormalizeRequired(bool required)
{
    m_normalizeRequired = required;
}