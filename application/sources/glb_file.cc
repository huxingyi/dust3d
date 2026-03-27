#include "glb_file.h"
#include "model_mesh.h"
#include "version.h"
#include <QByteArray>
#include <QDataStream>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QQuaternion>
#include <QtCore/qbuffer.h>
#include <cmath>

bool GlbFileWriter::m_enableComment = false;

GlbFileWriter::GlbFileWriter(dust3d::Object& object,
    const QString& filename,
    QImage* textureImage,
    QImage* normalImage,
    QImage* ormImage,
    const RigStructure* rigStructure,
    const std::map<std::string, dust3d::Matrix4x4>* inverseBindMatrices,
    const std::vector<dust3d::RigAnimationClip>* animationClips)
    : m_filename(filename)
{
    const std::vector<std::vector<dust3d::Vector3>>* triangleVertexNormals = object.triangleVertexNormals();
    if (m_outputNormal) {
        m_outputNormal = nullptr != triangleVertexNormals;
    }

    const std::vector<std::vector<dust3d::Vector2>>* triangleVertexUvs = object.triangleVertexUvs();
    if (m_outputUv) {
        m_outputUv = nullptr != triangleVertexUvs;
    }

    QDataStream binStream(&m_binByteArray, QIODevice::WriteOnly);
    binStream.setFloatingPointPrecision(QDataStream::SinglePrecision);
    binStream.setByteOrder(QDataStream::LittleEndian);

    auto alignBin = [this, &binStream] {
        while (0 != this->m_binByteArray.size() % 4) {
            binStream << (quint8)0;
        }
    };

    QDataStream jsonStream(&m_jsonByteArray, QIODevice::WriteOnly);
    jsonStream.setFloatingPointPrecision(QDataStream::SinglePrecision);
    jsonStream.setByteOrder(QDataStream::LittleEndian);

    auto alignJson = [this, &jsonStream] {
        while (0 != this->m_jsonByteArray.size() % 4) {
            jsonStream << (quint8)' ';
        }
    };

    int bufferViewIndex = 0;
    int bufferViewFromOffset;

    bool hasRig = nullptr != rigStructure && nullptr != inverseBindMatrices && !rigStructure->bones.empty();
    bool hasAnimation = hasRig && nullptr != animationClips && !animationClips->empty();
    bool hasVertexBoneBindings = hasRig && !object.vertexBone1.empty() && object.vertexBone1.size() == object.vertices.size();

    constexpr int skeletonNodeStartIndex = 2;
    std::map<std::string, size_t> boneNameToIndex;
    if (hasRig) {
        for (size_t i = 0; i < rigStructure->bones.size(); ++i)
            boneNameToIndex[rigStructure->bones[i].name.toStdString()] = i;
    }

    auto matrixToTranslationAndRotation = [](const dust3d::Matrix4x4& mat,
        float& tx, float& ty, float& tz,
        float& qx, float& qy, float& qz, float& qw) {
        const double* m = mat.constData();
        tx = (float)m[12];
        ty = (float)m[13];
        tz = (float)m[14];
        double trace = m[0] + m[5] + m[10];
        double dw, dx, dy, dz;
        if (trace > 0.0) {
            double s = 0.5 / std::sqrt(trace + 1.0);
            dw = 0.25 / s;
            dx = (m[6] - m[9]) * s;
            dy = (m[8] - m[2]) * s;
            dz = (m[1] - m[4]) * s;
        } else if (m[0] > m[5] && m[0] > m[10]) {
            double s = 2.0 * std::sqrt(1.0 + m[0] - m[5] - m[10]);
            dw = (m[6] - m[9]) / s;
            dx = 0.25 * s;
            dy = (m[4] + m[1]) / s;
            dz = (m[8] + m[2]) / s;
        } else if (m[5] > m[10]) {
            double s = 2.0 * std::sqrt(1.0 + m[5] - m[0] - m[10]);
            dw = (m[8] - m[2]) / s;
            dx = (m[4] + m[1]) / s;
            dy = 0.25 * s;
            dz = (m[9] + m[6]) / s;
        } else {
            double s = 2.0 * std::sqrt(1.0 + m[10] - m[0] - m[5]);
            dw = (m[1] - m[4]) / s;
            dx = (m[8] + m[2]) / s;
            dy = (m[9] + m[6]) / s;
            dz = 0.25 * s;
        }
        qx = (float)dx;
        qy = (float)dy;
        qz = (float)dz;
        qw = (float)dw;
    };

    auto computeLocalTransform = [](const std::string& parentName,
        const dust3d::Matrix4x4& childWorldTransform,
        const std::map<std::string, dust3d::Matrix4x4>& worldTransforms) -> dust3d::Matrix4x4 {
        if (parentName.empty())
            return childWorldTransform;
        auto parentIt = worldTransforms.find(parentName);
        if (parentIt == worldTransforms.end())
            return childWorldTransform;
        dust3d::Matrix4x4 inv = parentIt->second.inverted();
        inv *= childWorldTransform;
        return inv;
    };

    m_json["asset"]["version"] = "2.0";
    m_json["asset"]["generator"] = APP_NAME " " APP_HUMAN_VER;
    m_json["scenes"][0]["nodes"] = { 0 };

    if (hasRig) {
        m_json["nodes"][0]["children"] = { 1, skeletonNodeStartIndex };
        m_json["nodes"][1]["mesh"] = 0;
        m_json["nodes"][1]["skin"] = 0;
    } else {
        m_json["nodes"][0]["mesh"] = 0;
    }

    std::vector<dust3d::Vector3> triangleVertexPositions;
    std::vector<size_t> triangleVertexOldIndices;
    for (const auto& triangleIndices : object.triangles) {
        for (size_t j = 0; j < 3; ++j) {
            triangleVertexOldIndices.push_back(triangleIndices[j]);
            triangleVertexPositions.push_back(object.vertices[triangleIndices[j]]);
        }
    }

    int primitiveIndex = 0;
    if (!triangleVertexPositions.empty()) {

        m_json["meshes"][0]["primitives"][primitiveIndex]["indices"] = bufferViewIndex;
        m_json["meshes"][0]["primitives"][primitiveIndex]["material"] = primitiveIndex;
        int attributeIndex = 0;
        m_json["meshes"][0]["primitives"][primitiveIndex]["attributes"]["POSITION"] = bufferViewIndex + (++attributeIndex);
        if (m_outputNormal)
            m_json["meshes"][0]["primitives"][primitiveIndex]["attributes"]["NORMAL"] = bufferViewIndex + (++attributeIndex);
        if (m_outputUv)
            m_json["meshes"][0]["primitives"][primitiveIndex]["attributes"]["TEXCOORD_0"] = bufferViewIndex + (++attributeIndex);
        if (hasVertexBoneBindings) {
            m_json["meshes"][0]["primitives"][primitiveIndex]["attributes"]["JOINTS_0"] = bufferViewIndex + (++attributeIndex);
            m_json["meshes"][0]["primitives"][primitiveIndex]["attributes"]["WEIGHTS_0"] = bufferViewIndex + (++attributeIndex);
        }
        int textureIndex = 0;
        m_json["materials"][primitiveIndex]["pbrMetallicRoughness"]["baseColorTexture"]["index"] = textureIndex++;
        m_json["materials"][primitiveIndex]["pbrMetallicRoughness"]["metallicFactor"] = ModelMesh::m_defaultMetalness;
        m_json["materials"][primitiveIndex]["pbrMetallicRoughness"]["roughnessFactor"] = ModelMesh::m_defaultRoughness;
        if (object.alphaEnabled)
            m_json["materials"][primitiveIndex]["alphaMode"] = "BLEND";
        if (normalImage) {
            m_json["materials"][primitiveIndex]["normalTexture"]["index"] = textureIndex++;
        }
        if (ormImage) {
            m_json["materials"][primitiveIndex]["occlusionTexture"]["index"] = textureIndex;
            m_json["materials"][primitiveIndex]["pbrMetallicRoughness"]["metallicRoughnessTexture"]["index"] = textureIndex;
            m_json["materials"][primitiveIndex]["pbrMetallicRoughness"]["metallicFactor"] = 1.0;
            m_json["materials"][primitiveIndex]["pbrMetallicRoughness"]["roughnessFactor"] = 1.0;
            textureIndex++;
        }

        primitiveIndex++;

        bufferViewFromOffset = (int)m_binByteArray.size();
        for (size_t index = 0; index < triangleVertexPositions.size(); index += 3) {
            binStream << (quint16)index << (quint16)(index + 1) << (quint16)(index + 2);
        }
        m_json["bufferViews"][bufferViewIndex]["buffer"] = 0;
        m_json["bufferViews"][bufferViewIndex]["byteOffset"] = bufferViewFromOffset;
        m_json["bufferViews"][bufferViewIndex]["byteLength"] = (int)triangleVertexPositions.size() * sizeof(quint16);
        m_json["bufferViews"][bufferViewIndex]["target"] = 34963;
        Q_ASSERT((int)triangleVertexPositions.size() * sizeof(quint16) == m_binByteArray.size() - bufferViewFromOffset);
        alignBin();
        if (m_enableComment)
            m_json["accessors"][bufferViewIndex]["__comment"] = QString("/accessors/%1: triangle indices").arg(QString::number(bufferViewIndex)).toUtf8().constData();
        m_json["accessors"][bufferViewIndex]["bufferView"] = bufferViewIndex;
        m_json["accessors"][bufferViewIndex]["byteOffset"] = 0;
        m_json["accessors"][bufferViewIndex]["componentType"] = 5123;
        m_json["accessors"][bufferViewIndex]["count"] = triangleVertexPositions.size();
        m_json["accessors"][bufferViewIndex]["type"] = "SCALAR";
        bufferViewIndex++;

        bufferViewFromOffset = (int)m_binByteArray.size();
        m_json["bufferViews"][bufferViewIndex]["buffer"] = 0;
        m_json["bufferViews"][bufferViewIndex]["byteOffset"] = bufferViewFromOffset;
        float minX = 100;
        float maxX = -100;
        float minY = 100;
        float maxY = -100;
        float minZ = 100;
        float maxZ = -100;
        for (const auto& position : triangleVertexPositions) {
            if (position.x() < minX)
                minX = position.x();
            if (position.x() > maxX)
                maxX = position.x();
            if (position.y() < minY)
                minY = position.y();
            if (position.y() > maxY)
                maxY = position.y();
            if (position.z() < minZ)
                minZ = position.z();
            if (position.z() > maxZ)
                maxZ = position.z();
            binStream << (float)position.x() << (float)position.y() << (float)position.z();
        }
        Q_ASSERT((int)triangleVertexPositions.size() * 3 * sizeof(float) == m_binByteArray.size() - bufferViewFromOffset);
        m_json["bufferViews"][bufferViewIndex]["byteLength"] = triangleVertexPositions.size() * 3 * sizeof(float);
        m_json["bufferViews"][bufferViewIndex]["target"] = 34962;
        alignBin();
        if (m_enableComment)
            m_json["accessors"][bufferViewIndex]["__comment"] = QString("/accessors/%1: xyz").arg(QString::number(bufferViewIndex)).toUtf8().constData();
        m_json["accessors"][bufferViewIndex]["bufferView"] = bufferViewIndex;
        m_json["accessors"][bufferViewIndex]["byteOffset"] = 0;
        m_json["accessors"][bufferViewIndex]["componentType"] = 5126;
        m_json["accessors"][bufferViewIndex]["count"] = triangleVertexPositions.size();
        m_json["accessors"][bufferViewIndex]["type"] = "VEC3";
        m_json["accessors"][bufferViewIndex]["max"] = { maxX, maxY, maxZ };
        m_json["accessors"][bufferViewIndex]["min"] = { minX, minY, minZ };
        bufferViewIndex++;

        if (m_outputNormal) {
            bufferViewFromOffset = (int)m_binByteArray.size();
            m_json["bufferViews"][bufferViewIndex]["buffer"] = 0;
            m_json["bufferViews"][bufferViewIndex]["byteOffset"] = bufferViewFromOffset;
            QStringList normalList;
            for (const auto& normals : (*triangleVertexNormals)) {
                for (const auto& it : normals) {
                    binStream << (float)it.x() << (float)it.y() << (float)it.z();
                    if (m_enableComment && m_outputNormal)
                        normalList.append(QString("<%1,%2,%3>").arg(QString::number(it.x())).arg(QString::number(it.y())).arg(QString::number(it.z())));
                }
            }
            Q_ASSERT((int)triangleVertexNormals->size() * 3 * 3 * sizeof(float) == m_binByteArray.size() - bufferViewFromOffset);
            m_json["bufferViews"][bufferViewIndex]["byteLength"] = triangleVertexNormals->size() * 3 * 3 * sizeof(float);
            m_json["bufferViews"][bufferViewIndex]["target"] = 34962;
            alignBin();
            if (m_enableComment)
                m_json["accessors"][bufferViewIndex]["__comment"] = QString("/accessors/%1: normal %2").arg(QString::number(bufferViewIndex)).arg(normalList.join(" ")).toUtf8().constData();
            m_json["accessors"][bufferViewIndex]["bufferView"] = bufferViewIndex;
            m_json["accessors"][bufferViewIndex]["byteOffset"] = 0;
            m_json["accessors"][bufferViewIndex]["componentType"] = 5126;
            m_json["accessors"][bufferViewIndex]["count"] = triangleVertexNormals->size() * 3;
            m_json["accessors"][bufferViewIndex]["type"] = "VEC3";
            bufferViewIndex++;
        }

        if (m_outputUv) {
            bufferViewFromOffset = (int)m_binByteArray.size();
            m_json["bufferViews"][bufferViewIndex]["buffer"] = 0;
            m_json["bufferViews"][bufferViewIndex]["byteOffset"] = bufferViewFromOffset;
            for (const auto& uvs : (*triangleVertexUvs)) {
                for (const auto& it : uvs)
                    binStream << (float)it.x() << (float)it.y();
            }
            m_json["bufferViews"][bufferViewIndex]["byteLength"] = m_binByteArray.size() - bufferViewFromOffset;
            alignBin();
            if (m_enableComment)
                m_json["accessors"][bufferViewIndex]["__comment"] = QString("/accessors/%1: uv").arg(QString::number(bufferViewIndex)).toUtf8().constData();
            m_json["accessors"][bufferViewIndex]["bufferView"] = bufferViewIndex;
            m_json["accessors"][bufferViewIndex]["byteOffset"] = 0;
            m_json["accessors"][bufferViewIndex]["componentType"] = 5126;
            m_json["accessors"][bufferViewIndex]["count"] = triangleVertexUvs->size() * 3;
            m_json["accessors"][bufferViewIndex]["type"] = "VEC2";
            bufferViewIndex++;
        }

        if (hasVertexBoneBindings) {
            bufferViewFromOffset = (int)m_binByteArray.size();
            m_json["bufferViews"][bufferViewIndex]["buffer"] = 0;
            m_json["bufferViews"][bufferViewIndex]["byteOffset"] = bufferViewFromOffset;
            for (const auto& oldIndex : triangleVertexOldIndices) {
                quint16 j0 = 0, j1 = 0;
                if (oldIndex < object.vertexBone1.size() && !object.vertexBone1[oldIndex].first.empty()) {
                    auto it = boneNameToIndex.find(object.vertexBone1[oldIndex].first);
                    if (it != boneNameToIndex.end())
                        j0 = (quint16)it->second;
                }
                if (oldIndex < object.vertexBone2.size() && !object.vertexBone2[oldIndex].first.empty()) {
                    auto it = boneNameToIndex.find(object.vertexBone2[oldIndex].first);
                    if (it != boneNameToIndex.end())
                        j1 = (quint16)it->second;
                }
                binStream << j0 << j1 << (quint16)0 << (quint16)0;
            }
            m_json["bufferViews"][bufferViewIndex]["byteLength"] = m_binByteArray.size() - bufferViewFromOffset;
            alignBin();
            if (m_enableComment)
                m_json["accessors"][bufferViewIndex]["__comment"] = QString("/accessors/%1: bone joints").arg(QString::number(bufferViewIndex)).toUtf8().constData();
            m_json["accessors"][bufferViewIndex]["bufferView"] = bufferViewIndex;
            m_json["accessors"][bufferViewIndex]["byteOffset"] = 0;
            m_json["accessors"][bufferViewIndex]["componentType"] = 5123;
            m_json["accessors"][bufferViewIndex]["count"] = triangleVertexOldIndices.size();
            m_json["accessors"][bufferViewIndex]["type"] = "VEC4";
            bufferViewIndex++;

            bufferViewFromOffset = (int)m_binByteArray.size();
            m_json["bufferViews"][bufferViewIndex]["buffer"] = 0;
            m_json["bufferViews"][bufferViewIndex]["byteOffset"] = bufferViewFromOffset;
            for (const auto& oldIndex : triangleVertexOldIndices) {
                float w0 = 0.0f, w1 = 0.0f;
                if (oldIndex < object.vertexBone1.size() && !object.vertexBone1[oldIndex].first.empty())
                    w0 = object.vertexBone1[oldIndex].second;
                if (oldIndex < object.vertexBone2.size() && !object.vertexBone2[oldIndex].first.empty())
                    w1 = object.vertexBone2[oldIndex].second;
                binStream << w0 << w1 << (float)0.0f << (float)0.0f;
            }
            m_json["bufferViews"][bufferViewIndex]["byteLength"] = m_binByteArray.size() - bufferViewFromOffset;
            alignBin();
            if (m_enableComment)
                m_json["accessors"][bufferViewIndex]["__comment"] = QString("/accessors/%1: bone weights").arg(QString::number(bufferViewIndex)).toUtf8().constData();
            m_json["accessors"][bufferViewIndex]["bufferView"] = bufferViewIndex;
            m_json["accessors"][bufferViewIndex]["byteOffset"] = 0;
            m_json["accessors"][bufferViewIndex]["componentType"] = 5126;
            m_json["accessors"][bufferViewIndex]["count"] = triangleVertexOldIndices.size();
            m_json["accessors"][bufferViewIndex]["type"] = "VEC4";
            bufferViewIndex++;
        }
    }

    if (hasRig) {
        // Inverse bind matrices
        bufferViewFromOffset = (int)m_binByteArray.size();
        m_json["bufferViews"][bufferViewIndex]["buffer"] = 0;
        m_json["bufferViews"][bufferViewIndex]["byteOffset"] = bufferViewFromOffset;
        for (const auto& bone : rigStructure->bones) {
            std::string boneName = bone.name.toStdString();
            auto invIt = inverseBindMatrices->find(boneName);
            if (invIt != inverseBindMatrices->end()) {
                const double* d = invIt->second.constData();
                for (int j = 0; j < 16; ++j)
                    binStream << (float)d[j];
            } else {
                for (int j = 0; j < 16; ++j)
                    binStream << (float)(j % 5 == 0 ? 1.0 : 0.0);
            }
        }
        Q_ASSERT((int)rigStructure->bones.size() * 16 * sizeof(float) == m_binByteArray.size() - bufferViewFromOffset);
        m_json["bufferViews"][bufferViewIndex]["byteLength"] = m_binByteArray.size() - bufferViewFromOffset;
        alignBin();
        if (m_enableComment)
            m_json["accessors"][bufferViewIndex]["__comment"] = QString("/accessors/%1: inverse bind matrices").arg(QString::number(bufferViewIndex)).toUtf8().constData();
        m_json["accessors"][bufferViewIndex]["bufferView"] = bufferViewIndex;
        m_json["accessors"][bufferViewIndex]["byteOffset"] = 0;
        m_json["accessors"][bufferViewIndex]["componentType"] = 5126;
        m_json["accessors"][bufferViewIndex]["count"] = rigStructure->bones.size();
        m_json["accessors"][bufferViewIndex]["type"] = "MAT4";
        int inverseBindMatricesAccessorIdx = bufferViewIndex;
        bufferViewIndex++;

        // Skin
        m_json["skins"][0]["inverseBindMatrices"] = inverseBindMatricesAccessorIdx;
        m_json["skins"][0]["joints"] = nlohmann::json::array();
        for (size_t i = 0; i < rigStructure->bones.size(); ++i)
            m_json["skins"][0]["joints"].push_back(skeletonNodeStartIndex + (int)i);

        // Find root bone for skin.skeleton
        for (const auto& bone : rigStructure->bones) {
            if (bone.parent.isEmpty()) {
                auto it = boneNameToIndex.find(bone.name.toStdString());
                if (it != boneNameToIndex.end()) {
                    m_json["skins"][0]["skeleton"] = skeletonNodeStartIndex + (int)it->second;
                    break;
                }
            }
        }

        // Build rest pose world transforms from inverse bind matrices
        std::map<std::string, dust3d::Matrix4x4> restWorldTransforms;
        for (const auto& bone : rigStructure->bones) {
            std::string name = bone.name.toStdString();
            auto it = inverseBindMatrices->find(name);
            if (it != inverseBindMatrices->end())
                restWorldTransforms[name] = it->second.inverted();
        }

        // Build parent->children index map for node children assignment
        std::map<std::string, std::vector<size_t>> boneChildren;
        for (size_t i = 0; i < rigStructure->bones.size(); ++i) {
            std::string parentName = rigStructure->bones[i].parent.toStdString();
            if (!parentName.empty())
                boneChildren[parentName].push_back(i);
        }

        // Set up bone nodes with local TRS from rest pose
        for (size_t i = 0; i < rigStructure->bones.size(); ++i) {
            const auto& bone = rigStructure->bones[i];
            std::string boneName = bone.name.toStdString();
            std::string parentName = bone.parent.toStdString();
            int nodeIdx = skeletonNodeStartIndex + (int)i;

            m_json["nodes"][nodeIdx]["name"] = boneName;

            dust3d::Matrix4x4 worldTransform;
            auto worldIt = restWorldTransforms.find(boneName);
            if (worldIt != restWorldTransforms.end())
                worldTransform = worldIt->second;

            dust3d::Matrix4x4 localTransform = computeLocalTransform(parentName, worldTransform, restWorldTransforms);

            float tx, ty, tz, qx, qy, qz, qw;
            matrixToTranslationAndRotation(localTransform, tx, ty, tz, qx, qy, qz, qw);

            m_json["nodes"][nodeIdx]["translation"] = { tx, ty, tz };
            m_json["nodes"][nodeIdx]["rotation"] = { qx, qy, qz, qw };

            auto childrenIt = boneChildren.find(boneName);
            if (childrenIt != boneChildren.end() && !childrenIt->second.empty()) {
                m_json["nodes"][nodeIdx]["children"] = nlohmann::json::array();
                for (auto childIdx : childrenIt->second)
                    m_json["nodes"][nodeIdx]["children"].push_back(skeletonNodeStartIndex + (int)childIdx);
            }
        }
    }

    if (hasAnimation) {
        for (int animIdx = 0; animIdx < (int)animationClips->size(); ++animIdx) {
            const auto& clip = (*animationClips)[animIdx];
            m_json["animations"][animIdx]["name"] = clip.name;

            // Input: keyframe timestamps
            int inputAccessorIdx = bufferViewIndex;
            bufferViewFromOffset = (int)m_binByteArray.size();
            m_json["bufferViews"][bufferViewIndex]["buffer"] = 0;
            m_json["bufferViews"][bufferViewIndex]["byteOffset"] = bufferViewFromOffset;
            float minTime = clip.frames.empty() ? 0.0f : clip.frames[0].time;
            float maxTime = minTime;
            for (const auto& frame : clip.frames) {
                binStream << frame.time;
                if (frame.time < minTime)
                    minTime = frame.time;
                if (frame.time > maxTime)
                    maxTime = frame.time;
            }
            m_json["bufferViews"][bufferViewIndex]["byteLength"] = m_binByteArray.size() - bufferViewFromOffset;
            alignBin();
            m_json["accessors"][bufferViewIndex]["bufferView"] = bufferViewIndex;
            m_json["accessors"][bufferViewIndex]["byteOffset"] = 0;
            m_json["accessors"][bufferViewIndex]["componentType"] = 5126;
            m_json["accessors"][bufferViewIndex]["count"] = clip.frames.size();
            m_json["accessors"][bufferViewIndex]["type"] = "SCALAR";
            m_json["accessors"][bufferViewIndex]["max"][0] = maxTime;
            m_json["accessors"][bufferViewIndex]["min"][0] = minTime;
            bufferViewIndex++;

            int samplerIndex = 0;
            int channelIndex = 0;

            for (size_t boneIdx = 0; boneIdx < rigStructure->bones.size(); ++boneIdx) {
                const auto& bone = rigStructure->bones[boneIdx];
                std::string boneName = bone.name.toStdString();
                std::string parentName = bone.parent.toStdString();
                int nodeIdx = skeletonNodeStartIndex + (int)boneIdx;

                // Translation output
                bufferViewFromOffset = (int)m_binByteArray.size();
                m_json["bufferViews"][bufferViewIndex]["buffer"] = 0;
                m_json["bufferViews"][bufferViewIndex]["byteOffset"] = bufferViewFromOffset;
                for (const auto& frame : clip.frames) {
                    dust3d::Matrix4x4 worldTransform;
                    auto worldIt = frame.boneWorldTransforms.find(boneName);
                    if (worldIt != frame.boneWorldTransforms.end())
                        worldTransform = worldIt->second;
                    dust3d::Matrix4x4 localTransform = computeLocalTransform(parentName, worldTransform, frame.boneWorldTransforms);
                    float tx, ty, tz, qx, qy, qz, qw;
                    matrixToTranslationAndRotation(localTransform, tx, ty, tz, qx, qy, qz, qw);
                    binStream << tx << ty << tz;
                }
                m_json["bufferViews"][bufferViewIndex]["byteLength"] = m_binByteArray.size() - bufferViewFromOffset;
                alignBin();
                m_json["accessors"][bufferViewIndex]["bufferView"] = bufferViewIndex;
                m_json["accessors"][bufferViewIndex]["byteOffset"] = 0;
                m_json["accessors"][bufferViewIndex]["componentType"] = 5126;
                m_json["accessors"][bufferViewIndex]["count"] = clip.frames.size();
                m_json["accessors"][bufferViewIndex]["type"] = "VEC3";
                int translationAccessorIdx = bufferViewIndex;
                bufferViewIndex++;

                // Rotation output
                bufferViewFromOffset = (int)m_binByteArray.size();
                m_json["bufferViews"][bufferViewIndex]["buffer"] = 0;
                m_json["bufferViews"][bufferViewIndex]["byteOffset"] = bufferViewFromOffset;
                for (const auto& frame : clip.frames) {
                    dust3d::Matrix4x4 worldTransform;
                    auto worldIt = frame.boneWorldTransforms.find(boneName);
                    if (worldIt != frame.boneWorldTransforms.end())
                        worldTransform = worldIt->second;
                    dust3d::Matrix4x4 localTransform = computeLocalTransform(parentName, worldTransform, frame.boneWorldTransforms);
                    float tx, ty, tz, qx, qy, qz, qw;
                    matrixToTranslationAndRotation(localTransform, tx, ty, tz, qx, qy, qz, qw);
                    binStream << qx << qy << qz << qw;
                }
                m_json["bufferViews"][bufferViewIndex]["byteLength"] = m_binByteArray.size() - bufferViewFromOffset;
                alignBin();
                m_json["accessors"][bufferViewIndex]["bufferView"] = bufferViewIndex;
                m_json["accessors"][bufferViewIndex]["byteOffset"] = 0;
                m_json["accessors"][bufferViewIndex]["componentType"] = 5126;
                m_json["accessors"][bufferViewIndex]["count"] = clip.frames.size();
                m_json["accessors"][bufferViewIndex]["type"] = "VEC4";
                int rotationAccessorIdx = bufferViewIndex;
                bufferViewIndex++;

                m_json["animations"][animIdx]["samplers"][samplerIndex]["input"] = inputAccessorIdx;
                m_json["animations"][animIdx]["samplers"][samplerIndex]["output"] = translationAccessorIdx;
                m_json["animations"][animIdx]["samplers"][samplerIndex]["interpolation"] = "LINEAR";
                m_json["animations"][animIdx]["channels"][channelIndex]["sampler"] = samplerIndex;
                m_json["animations"][animIdx]["channels"][channelIndex]["target"]["node"] = nodeIdx;
                m_json["animations"][animIdx]["channels"][channelIndex]["target"]["path"] = "translation";
                ++samplerIndex;
                ++channelIndex;

                m_json["animations"][animIdx]["samplers"][samplerIndex]["input"] = inputAccessorIdx;
                m_json["animations"][animIdx]["samplers"][samplerIndex]["output"] = rotationAccessorIdx;
                m_json["animations"][animIdx]["samplers"][samplerIndex]["interpolation"] = "LINEAR";
                m_json["animations"][animIdx]["channels"][channelIndex]["sampler"] = samplerIndex;
                m_json["animations"][animIdx]["channels"][channelIndex]["target"]["node"] = nodeIdx;
                m_json["animations"][animIdx]["channels"][channelIndex]["target"]["path"] = "rotation";
                ++samplerIndex;
                ++channelIndex;
            }
        }
    }

    m_json["samplers"][0]["magFilter"] = 9729;
    m_json["samplers"][0]["minFilter"] = 9987;
    m_json["samplers"][0]["wrapS"] = 33648;
    m_json["samplers"][0]["wrapT"] = 33648;

    int imageIndex = 0;
    int textureIndex = 0;

    // Images should be put in the end of the buffer, because we are not using accessors
    if (nullptr != textureImage) {
        m_json["textures"][textureIndex]["sampler"] = 0;
        m_json["textures"][textureIndex]["source"] = imageIndex;

        bufferViewFromOffset = (int)m_binByteArray.size();
        m_json["bufferViews"][bufferViewIndex]["buffer"] = 0;
        m_json["bufferViews"][bufferViewIndex]["byteOffset"] = bufferViewFromOffset;
        QByteArray pngByteArray;
        QBuffer buffer(&pngByteArray);
        textureImage->save(&buffer, "PNG");
        binStream.writeRawData(pngByteArray.data(), pngByteArray.size());
        alignBin();
        m_json["bufferViews"][bufferViewIndex]["byteLength"] = m_binByteArray.size() - bufferViewFromOffset;
        m_json["images"][imageIndex]["bufferView"] = bufferViewIndex;
        m_json["images"][imageIndex]["mimeType"] = "image/png";
        bufferViewIndex++;

        imageIndex++;
        textureIndex++;
    }
    if (nullptr != normalImage) {
        m_json["textures"][textureIndex]["sampler"] = 0;
        m_json["textures"][textureIndex]["source"] = imageIndex;

        bufferViewFromOffset = (int)m_binByteArray.size();
        m_json["bufferViews"][bufferViewIndex]["buffer"] = 0;
        m_json["bufferViews"][bufferViewIndex]["byteOffset"] = bufferViewFromOffset;
        QByteArray pngByteArray;
        QBuffer buffer(&pngByteArray);
        normalImage->save(&buffer, "PNG");
        binStream.writeRawData(pngByteArray.data(), pngByteArray.size());
        alignBin();
        m_json["bufferViews"][bufferViewIndex]["byteLength"] = m_binByteArray.size() - bufferViewFromOffset;
        m_json["images"][imageIndex]["bufferView"] = bufferViewIndex;
        m_json["images"][imageIndex]["mimeType"] = "image/png";
        bufferViewIndex++;

        imageIndex++;
        textureIndex++;
    }
    if (nullptr != ormImage) {
        m_json["textures"][textureIndex]["sampler"] = 0;
        m_json["textures"][textureIndex]["source"] = imageIndex;

        bufferViewFromOffset = (int)m_binByteArray.size();
        m_json["bufferViews"][bufferViewIndex]["buffer"] = 0;
        m_json["bufferViews"][bufferViewIndex]["byteOffset"] = bufferViewFromOffset;
        QByteArray pngByteArray;
        QBuffer buffer(&pngByteArray);
        ormImage->save(&buffer, "PNG");
        binStream.writeRawData(pngByteArray.data(), pngByteArray.size());
        alignBin();
        m_json["bufferViews"][bufferViewIndex]["byteLength"] = m_binByteArray.size() - bufferViewFromOffset;
        m_json["images"][imageIndex]["bufferView"] = bufferViewIndex;
        m_json["images"][imageIndex]["mimeType"] = "image/png";
        bufferViewIndex++;

        imageIndex++;
        textureIndex++;
    }

    m_json["buffers"][0]["byteLength"] = m_binByteArray.size();

    auto jsonString = m_enableComment ? m_json.dump(4) : m_json.dump();
    jsonStream.writeRawData(jsonString.data(), jsonString.size());
    alignJson();
}

bool GlbFileWriter::save(QDataStream& output)
{
    output.setFloatingPointPrecision(QDataStream::SinglePrecision);
    output.setByteOrder(QDataStream::LittleEndian);

    uint32_t headerSize = 12;
    uint32_t chunk0DescriptionSize = 8;
    uint32_t chunk1DescriptionSize = 8;
    uint32_t fileSize = headerSize + chunk0DescriptionSize + m_jsonByteArray.size() + chunk1DescriptionSize + m_binByteArray.size();

    qDebug() << "Chunk 0 data size:" << m_jsonByteArray.size();
    qDebug() << "Chunk 1 data size:" << m_binByteArray.size();
    qDebug() << "File size:" << fileSize;

    //////////// Header ////////////

    // magic
    output << (uint32_t)0x46546C67;

    // version
    output << (uint32_t)0x00000002;

    // length
    output << (uint32_t)fileSize;

    //////////// Chunk 0 (Json) ////////////

    // length
    output << (uint32_t)m_jsonByteArray.size();

    // type
    output << (uint32_t)0x4E4F534A;

    // data
    output.writeRawData(m_jsonByteArray.data(), m_jsonByteArray.size());

    //////////// Chunk 1 (Binary Buffer) ///

    // length
    output << (uint32_t)m_binByteArray.size();

    // type
    output << (uint32_t)0x004E4942;

    // data
    output.writeRawData(m_binByteArray.data(), m_binByteArray.size());

    return true;
}

bool GlbFileWriter::save()
{
    QFile file(m_filename);
    if (!file.open(QIODevice::WriteOnly)) {
        return false;
    }
    QDataStream output(&file);
    return save(output);
}
