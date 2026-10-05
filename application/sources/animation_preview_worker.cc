#include "animation_preview_worker.h"
#include "theme.h"
#include <QDebug>
#include <algorithm>
#include <array>
#include <cstring>
#include <dust3d/animation/sound_event_detector.h>
#include <dust3d/animation/sound_generator.h>
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

    // Store animation metadata from animation clip
    m_movementSpeed = animationClip.movementSpeed;
    m_movementDirectionX = animationClip.movementDirectionX;
    m_movementDirectionZ = animationClip.movementDirectionZ;
    m_durationSeconds = animationClip.durationSeconds;
    if (animationClip.frames.size() > 1)
        m_frameInterval = animationClip.frames[1].time - animationClip.frames[0].time;

    // Generate procedural sound from animation contact events
    m_soundData = dust3d::AnimationSoundData();
    if (m_soundEnabled) {
        auto soundEvents = dust3d::SoundEventDetector::detect(animationClip, m_animationType, m_animationParameters);
        if (!soundEvents.empty()) {
            m_soundData = dust3d::SoundGenerator::generate(
                soundEvents, animationClip.durationSeconds, m_surfaceMaterial);
        }
    }

    // Generate a mesh for every frame
    for (const auto& frame : animationClip.frames) {
        // The terminal loop key is required by exporters but duplicates frame zero.
        if (animationClip.loop && frame.time >= animationClip.durationSeconds)
            continue;
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
        meshGenerator.generateMesh(poseRig, m_selectedBoneName);

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
        // No UV for the bones: they keep their own color when the model is textured
        for (int i = 0; i < skeletonMesh.triangleVertexCount(); ++i) {
            skeletonMesh.triangleVertices()[i].texU = -1.0f;
            skeletonMesh.triangleVertices()[i].texV = -1.0f;
        }

        std::unique_ptr<ModelMesh> frameMesh;
        if (m_rigObject && !m_rigObject->vertices.empty() && !frame.boneSkinMatrices.empty()) {
            dust3d::Object skinnedObject(*m_rigObject);

            // The bones of every vertex, resolved to the skin matrices of this frame
            using VertexSkin = std::array<std::pair<const dust3d::Matrix4x4*, float>, 4>;
            const std::array<const std::vector<std::pair<std::string, float>>*, 4> vertexBones = {
                &m_rigObject->vertexBone1, &m_rigObject->vertexBone2,
                &m_rigObject->vertexBone3, &m_rigObject->vertexBone4
            };
            std::vector<VertexSkin> vertexSkins(skinnedObject.vertices.size());
            for (size_t i = 0; i < vertexSkins.size(); ++i) {
                for (size_t k = 0; k < vertexBones.size(); ++k) {
                    vertexSkins[i][k] = { nullptr, 0.0f };
                    if (i >= vertexBones[k]->size())
                        continue;
                    const auto& bone = (*vertexBones[k])[i];
                    if (bone.first.empty())
                        continue;
                    auto it = frame.boneSkinMatrices.find(bone.first);
                    if (it != frame.boneSkinMatrices.end())
                        vertexSkins[i][k] = { &it->second, bone.second };
                }
            }

            for (size_t i = 0; i < skinnedObject.vertices.size(); ++i) {
                const dust3d::Vector3& origin = m_rigObject->vertices[i];
                dust3d::Vector3 transformed(0.0f, 0.0f, 0.0f);
                float totalWeight = 0.0f;
                for (const auto& skin : vertexSkins[i]) {
                    if (nullptr == skin.first)
                        continue;
                    transformed += skin.first->transformPoint(origin) * skin.second;
                    totalWeight += skin.second;
                }
                if (totalWeight > 1e-6f) {
                    transformed /= totalWeight;
                    skinnedObject.vertices[i] = transformed;
                } else {
                    skinnedObject.vertices[i] = origin;
                }
            }

            frameMesh = std::make_unique<ModelMesh>(skinnedObject);

            // Normals and tangents follow the bones as well, as they do in the exported model,
            // otherwise the lighting and the normal map stay those of the rest pose.
            // The length is kept: it carries the handedness of the tangent.
            auto skinDirection = [&](size_t vertexIndex, float& x, float& y, float& z) {
                dust3d::Vector3 direction(x, y, z);
                double length = direction.length();
                if (length < 1e-6)
                    return;
                dust3d::Vector3 transformed(0.0f, 0.0f, 0.0f);
                for (const auto& skin : vertexSkins[vertexIndex]) {
                    if (nullptr != skin.first)
                        transformed += skin.first->transformVector(direction) * skin.second;
                }
                double transformedLength = transformed.length();
                if (transformedLength < 1e-6)
                    return;
                transformed *= length / transformedLength;
                x = (float)transformed.x();
                y = (float)transformed.y();
                z = (float)transformed.z();
            };
            ModelOpenGLVertex* skinnedVertices = frameMesh->triangleVertices();
            for (size_t i = 0; i < skinnedObject.triangles.size(); ++i) {
                for (size_t j = 0; j < 3; ++j) {
                    ModelOpenGLVertex& v = skinnedVertices[i * 3 + j];
                    size_t vertexIndex = skinnedObject.triangles[i][j];
                    skinDirection(vertexIndex, v.normX, v.normY, v.normZ);
                    skinDirection(vertexIndex, v.tangentX, v.tangentY, v.tangentZ);
                }
            }
        }

        // Decide what should be visible according to the hide options.
        bool showSkeleton = !m_hideBones && skeletonMesh.triangleVertexCount() > 0;
        bool showSkinned = !m_hideParts && frameMesh && frameMesh->triangleVertexCount() > 0;

        if (!showSkeleton && !showSkinned) {
            // Both hidden: do not push any mesh for this frame.
            continue;
        }

        m_frameTimes.push_back(frame.time);
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

    if (m_textureImage && m_textureImage->width() > 0 && m_textureImage->height() > 0) {
        int texW = m_textureImage->width();
        int texH = m_textureImage->height();
        for (auto& frame : m_previewMeshes) {
            int skeletonCount = frame.skeletonVertexCount();
            int totalCount = frame.triangleVertexCount();
            ModelOpenGLVertex* vertices = frame.triangleVertices();
            for (int i = skeletonCount; i < totalCount; ++i) {
                ModelOpenGLVertex& v = vertices[i];
                float u = std::max(0.0f, std::min(1.0f, v.texU));
                float vc = std::max(0.0f, std::min(1.0f, v.texV));
                int px = std::min((int)(u * texW), texW - 1);
                int py = std::min((int)(vc * texH), texH - 1);
                QRgb pixel = m_textureImage->pixel(px, py);
                v.colorR = qRed(pixel) / 255.0f;
                v.colorG = qGreen(pixel) / 255.0f;
                v.colorB = qBlue(pixel) / 255.0f;
            }
        }
    }

    if (!m_selectedBoneName.isEmpty() && m_rigObject) {
        std::string selectedBoneStd = m_selectedBoneName.toStdString();
        std::vector<dust3d::Color> vertexWeightColors(m_rigObject->vertices.size());
        for (size_t i = 0; i < m_rigObject->vertices.size(); ++i) {
            float weight = 0.0f;
            if (i < m_rigObject->vertexBone1.size() && m_rigObject->vertexBone1[i].first == selectedBoneStd)
                weight += m_rigObject->vertexBone1[i].second;
            if (i < m_rigObject->vertexBone2.size() && m_rigObject->vertexBone2[i].first == selectedBoneStd)
                weight += m_rigObject->vertexBone2[i].second;
            if (i < m_rigObject->vertexBone3.size() && m_rigObject->vertexBone3[i].first == selectedBoneStd)
                weight += m_rigObject->vertexBone3[i].second;
            if (i < m_rigObject->vertexBone4.size() && m_rigObject->vertexBone4[i].first == selectedBoneStd)
                weight += m_rigObject->vertexBone4[i].second;
            vertexWeightColors[i] = calculateBoneWeightColor(weight);
        }
        for (auto& frame : m_previewMeshes) {
            int skeletonCount = frame.skeletonVertexCount();
            int totalCount = frame.triangleVertexCount();
            if (skeletonCount >= totalCount)
                continue;
            ModelOpenGLVertex* vertices = frame.triangleVertices();
            int destIndex = skeletonCount;
            for (size_t ti = 0; ti < m_rigObject->triangles.size() && destIndex < totalCount; ++ti) {
                const auto& tri = m_rigObject->triangles[ti];
                for (size_t j = 0; j < 3 && j < tri.size() && destIndex < totalCount; ++j) {
                    const dust3d::Color& c = vertexWeightColors[tri[j]];
                    vertices[destIndex].colorR = c.r();
                    vertices[destIndex].colorG = c.g();
                    vertices[destIndex].colorB = c.b();
                    ++destIndex;
                }
            }
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
