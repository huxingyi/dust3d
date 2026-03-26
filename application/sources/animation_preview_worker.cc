#include "animation_preview_worker.h"
#include "theme.h"
#include <dust3d/base/vector3.h>
#include <dust3d/rig/rig_generator.h>
#include <QDebug>
#include <cstring>

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
    if (!dust3d::AnimationGenerator::generate(baseRig, inverseBindMatrices, animationClip, "walk", m_frameCount, m_durationSeconds,
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
        meshGenerator.setNormalizeRequired(false);
        meshGenerator.setStartRadius(0.02);
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
            dust3d::Color(Theme::blue.redF(), Theme::blue.greenF(), Theme::blue.blueF()),
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

            frameMesh = std::make_unique<ModelMesh>(skinnedObject);
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
            m_previewMeshes.push_back(std::move(combinedMesh));
        } else if (showSkeleton) {
            m_previewMeshes.push_back(std::move(skeletonMesh));
        } else if (showSkinned) {
            m_previewMeshes.push_back(std::move(*frameMesh));
        }
    }

    qDebug() << "Animation preview: generated" << m_previewMeshes.size() << "frames";

    emit finished();
}
