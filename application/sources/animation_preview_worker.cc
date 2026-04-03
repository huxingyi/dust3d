#include "animation_preview_worker.h"
#include "theme.h"
#include <QDebug>
#include <algorithm>
#include <cstring>
#include <dust3d/base/vector3.h>
#include <dust3d/rig/rig_generator.h>

void AnimationPreviewWorker::process()
{
    m_previewMeshes.clear();

    dust3d::RigStructure baseRig = m_rigStructure.toRigStructure();

    dust3d::RigGenerator rigGenerator;
    std::map<std::string, dust3d::Matrix4x4> inverseBindMatrices;
    if (!rigGenerator.computeBoneInverseBindMatrices(baseRig, inverseBindMatrices)) {
        qWarning() << "Animation preview: failed to compute inverse bind matrices:" << QString::fromStdString(rigGenerator.getErrorMessage());
        emit finished();
        return;
    }

    dust3d::RigAnimationClip animationClip;
    if (!dust3d::AnimationGenerator::generate(baseRig, inverseBindMatrices, animationClip, m_animationType,
            m_animationParameters)) {
        qWarning() << "Animation preview: generate failed (only fly rig supported)";
        emit finished();
        return;
    }

    // Generate a mesh for every frame
    for (const auto& frame : animationClip.frames) {
        RigStructure poseRig = m_rigStructure;

        for (auto& boneNode : poseRig.bones) {
            auto iter = frame.boneWorldTransforms.find(boneNode.name.toStdString());
            if (iter == frame.boneWorldTransforms.end())
                continue;

            const dust3d::Matrix4x4& boneTransform = iter->second;

            float boneLength = 1.0f;
            for (const auto& sourceBone : m_rigStructure.bones) {
                if (sourceBone.name == boneNode.name) {
                    dust3d::Vector3 v0(sourceBone.posX, sourceBone.posY, sourceBone.posZ);
                    dust3d::Vector3 v1(sourceBone.endX, sourceBone.endY, sourceBone.endZ);
                    float d = (v1 - v0).length();
                    if (d > 1e-6f)
                        boneLength = d;
                    break;
                }
            }

            dust3d::Vector3 worldHead = boneTransform.transformPoint(dust3d::Vector3(0, 0, 0));
            dust3d::Vector3 worldTail = boneTransform.transformPoint(dust3d::Vector3(0, 0, boneLength));

            boneNode.posX = worldHead.x();
            boneNode.posY = worldHead.y();
            boneNode.posZ = worldHead.z();
            boneNode.endX = worldTail.x();
            boneNode.endY = worldTail.y();
            boneNode.endZ = worldTail.z();
        }

        RigSkeletonMeshGenerator meshGenerator;
        meshGenerator.setStartRadius(0.02);
        meshGenerator.setNormalizeRequired(false);
        meshGenerator.generateMesh(poseRig, "");

        const auto& vertices = meshGenerator.getVertices();
        const auto& faces = meshGenerator.getFaces();
        const auto* vertexProperties = meshGenerator.getVertexProperties();

        if (vertices.empty() || faces.empty()) {
            continue;
        }

        std::vector<std::vector<dust3d::Vector3>> triangleVertexNormals;
        triangleVertexNormals.reserve(faces.size());
        for (const auto& face : faces) {
            if (face.size() < 3)
                continue;
            dust3d::Vector3 normal = dust3d::Vector3::normal(vertices[face[0]], vertices[face[1]], vertices[face[2]]);
            triangleVertexNormals.push_back({ normal, normal, normal });
        }

        ModelMesh skeletonMesh(vertices, faces, triangleVertexNormals,
            dust3d::Color(Theme::green.redF(), Theme::green.greenF(), Theme::green.blueF()),
            0.0f, 1.0f, vertexProperties);

        std::unique_ptr<ModelMesh> frameMesh;
        if (m_rigObject && !m_rigObject->vertices.empty() && !frame.boneSkinMatrices.empty()) {
            dust3d::Object skinnedObject(*m_rigObject);

            for (size_t i = 0; i < skinnedObject.vertices.size(); ++i) {
                const dust3d::Vector3& origin = m_rigObject->vertices[i];
                dust3d::Vector3 transformed(0.0f, 0.0f, 0.0f);
                float totalWeight = 0.0f;

                if (i < m_rigObject->vertexBone1.size()) {
                    const auto& b1 = m_rigObject->vertexBone1[i];
                    if (!b1.first.empty()) {
                        auto it = frame.boneSkinMatrices.find(b1.first);
                        if (it != frame.boneSkinMatrices.end()) {
                            transformed += it->second.transformPoint(origin) * b1.second;
                            totalWeight += b1.second;
                        }
                    }
                }

                if (i < m_rigObject->vertexBone2.size()) {
                    const auto& b2 = m_rigObject->vertexBone2[i];
                    if (!b2.first.empty()) {
                        auto it = frame.boneSkinMatrices.find(b2.first);
                        if (it != frame.boneSkinMatrices.end()) {
                            transformed += it->second.transformPoint(origin) * b2.second;
                            totalWeight += b2.second;
                        }
                    }
                }

                if (totalWeight > 1e-6f) {
                    transformed /= totalWeight;
                    skinnedObject.vertices[i] = transformed;
                } else {
                    skinnedObject.vertices[i] = origin;
                }
            }

            // Create vertex properties for bone weight visualization
            std::vector<std::tuple<dust3d::Color, float /*metalness*/, float /*roughness*/>> vertexProperties;
            if (!m_selectedBoneName.isEmpty()) {
                // Generate bone weight colors for the selected bone
                std::string selectedBoneStd = m_selectedBoneName.toStdString();
                vertexProperties.reserve(skinnedObject.vertices.size());

                for (size_t i = 0; i < skinnedObject.vertices.size(); ++i) {
                    float weight = 0.0f;

                    // Check primary bone weight
                    if (i < m_rigObject->vertexBone1.size()) {
                        const auto& bone1 = m_rigObject->vertexBone1[i];
                        if (bone1.first == selectedBoneStd) {
                            weight += bone1.second;
                        }
                    }

                    // Check secondary bone weight
                    if (i < m_rigObject->vertexBone2.size()) {
                        const auto& bone2 = m_rigObject->vertexBone2[i];
                        if (bone2.first == selectedBoneStd) {
                            weight += bone2.second;
                        }
                    }

                    dust3d::Color boneWeightColor = calculateBoneWeightColor(weight);
                    vertexProperties.push_back(std::make_tuple(boneWeightColor, 0.0f, 1.0f));
                }

                // Create mesh with bone weight colors
                std::vector<std::vector<dust3d::Vector3>> triangleVertexNormals;
                triangleVertexNormals.reserve(skinnedObject.triangles.size());
                for (const auto& triangle : skinnedObject.triangles) {
                    if (triangle.size() >= 3) {
                        dust3d::Vector3 normal = dust3d::Vector3::normal(
                            skinnedObject.vertices[triangle[0]],
                            skinnedObject.vertices[triangle[1]],
                            skinnedObject.vertices[triangle[2]]);
                        triangleVertexNormals.push_back({ normal, normal, normal });
                    }
                }

                frameMesh = std::make_unique<ModelMesh>(
                    skinnedObject.vertices,
                    skinnedObject.triangles,
                    triangleVertexNormals,
                    dust3d::Color::createWhite(),
                    0.0f,
                    1.0f,
                    &vertexProperties);
            } else {
                // No bone selected, use default mesh
                frameMesh = std::make_unique<ModelMesh>(skinnedObject);
            }
        }

        // Decide what should be visible according to the hide options.
        bool showSkeleton = !m_hideBones && skeletonMesh.triangleVertexCount() > 0;
        bool showSkinned = !m_hideParts && frameMesh && frameMesh->triangleVertexCount() > 0;

        if (!showSkeleton && !showSkinned) {
            // Both hidden: do not push any mesh for this frame.
            continue;
        }

        if (showSkeleton && showSkinned) {
            int skeletonCount = skeletonMesh.triangleVertexCount();
            int skinnedCount = frameMesh->triangleVertexCount();
            int totalCount = skeletonCount + skinnedCount;

            ModelOpenGLVertex* combinedVertices = new ModelOpenGLVertex[totalCount];
            std::memcpy(combinedVertices, skeletonMesh.triangleVertices(), skeletonCount * sizeof(ModelOpenGLVertex));
            std::memcpy(combinedVertices + skeletonCount, frameMesh->triangleVertices(), skinnedCount * sizeof(ModelOpenGLVertex));

            ModelMesh combinedMesh(combinedVertices, totalCount);
            combinedMesh.setSkeletonVertexCount(skeletonCount);
            m_previewMeshes.push_back(std::move(combinedMesh));
        } else if (showSkeleton) {
            skeletonMesh.setSkeletonVertexCount(skeletonMesh.triangleVertexCount());
            m_previewMeshes.push_back(std::move(skeletonMesh));
        } else if (showSkinned) {
            frameMesh->setSkeletonVertexCount(0);
            m_previewMeshes.push_back(std::move(*frameMesh));
        }
    }

    qDebug() << "Animation preview: generated" << m_previewMeshes.size() << "frames";

    emit finished();
}

dust3d::Color AnimationPreviewWorker::calculateBoneWeightColor(float weight)
{
    // Clamp weight to [0, 1] range
    weight = std::max(0.0f, std::min(1.0f, weight));

    // Interpolate between blue (weight = 0) and red (weight = 1)
    // Blue: (0, 0, 1), Red: (1, 0, 0)
    float red = weight;
    float green = 0.0f;
    float blue = 1.0f - weight;

    return dust3d::Color(red, green, blue);
}
