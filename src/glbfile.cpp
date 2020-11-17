#include <QFile>
#include <QQuaternion>
#include <QByteArray>
#include <QDataStream>
#include <QFileInfo>
#include <QDir>
#include <QtCore/qbuffer.h>
#include "glbfile.h"
#include "version.h"
#include "util.h"
#include "jointnodetree.h"
#include "model.h"

// Play with glTF online:
// https://gltf-viewer.donmccurdy.com/

// https://github.com/KhronosGroup/glTF-Tutorials/blob/master/gltfTutorial/gltfTutorial_020_Skins.md
// https://github.com/KhronosGroup/glTF-Tutorials/blob/master/gltfTutorial/gltfTutorial_004_ScenesNodes.md#global-transforms-of-nodes

// http://quaternions.online/
// https://en.m.wikipedia.org/wiki/Rotation_formalisms_in_three_dimensions?wprov=sfla1

bool GlbFileWriter::m_enableComment = false;

GlbFileWriter::GlbFileWriter(Object &object,
        const std::vector<RiggerBone> *resultRigBones,
        const std::map<int, RiggerVertexWeights> *resultRigWeights,
        const QString &filename,
        bool textureHasTransparencySettings,
        QImage *textureImage,
        QImage *normalImage,
        QImage *ormImage,
        const std::vector<std::pair<QString, std::vector<std::pair<float, JointNodeTree>>>> *motions) :
    m_filename(filename),
    m_outputNormal(true),
    m_outputAnimation(true),
    m_outputUv(true)
{
    const std::vector<std::vector<QVector3D>> *triangleVertexNormals = object.triangleVertexNormals();
    if (m_outputNormal) {
        m_outputNormal = nullptr != triangleVertexNormals;
    }
    
    const std::vector<std::vector<QVector2D>> *triangleVertexUvs = object.triangleVertexUvs();
    if (m_outputUv) {
        m_outputUv = nullptr != triangleVertexUvs;
    }
    
    if (m_outputAnimation) {
        m_outputAnimation = nullptr != motions && !motions->empty();
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
    
    JointNodeTree jointNodeTree(resultRigBones);
    const auto &boneNodes = jointNodeTree.nodes();
    
    m_json["asset"]["version"] = "2.0";
    m_json["asset"]["generator"] = APP_NAME " " APP_HUMAN_VER;
    m_json["scenes"][0]["nodes"] = {0};
    
    constexpr int skeletonNodeStartIndex = 2;
    
    if (resultRigBones && resultRigWeights && !resultRigBones->empty()) {
        m_json["nodes"][0]["children"] = {
            1,
            skeletonNodeStartIndex
        };
        
        m_json["nodes"][1]["mesh"] = 0;
        m_json["nodes"][1]["skin"] = 0;
        
        m_json["skins"][0]["joints"] = {};
        const QQuaternion noneRotation;
        for (size_t i = 0; i < boneNodes.size(); i++) {
            const auto &bone = (*resultRigBones)[i];
            m_json["skins"][0]["joints"] += skeletonNodeStartIndex + i;
            
            m_json["nodes"][skeletonNodeStartIndex + i]["name"] = boneNodes[i].name.toUtf8().constData();
            m_json["nodes"][skeletonNodeStartIndex + i]["translation"] = {
                boneNodes[i].bindTranslation.x(),
                boneNodes[i].bindTranslation.y(),
                boneNodes[i].bindTranslation.z()
            };
            m_json["nodes"][skeletonNodeStartIndex + i]["rotation"] = {
                noneRotation.x(),
                noneRotation.y(),
                noneRotation.z(),
                noneRotation.scalar()
            };
            
            if (!boneNodes[i].children.empty()) {
                m_json["nodes"][skeletonNodeStartIndex + i]["children"] = {};
                for (const auto &it: boneNodes[i].children) {
                    m_json["nodes"][skeletonNodeStartIndex + i]["children"] += skeletonNodeStartIndex + it;
                }
            }
        }
        
        m_json["skins"][0]["skeleton"] = skeletonNodeStartIndex;
        m_json["skins"][0]["inverseBindMatrices"] = bufferViewIndex;
        bufferViewFromOffset = (int)m_binByteArray.size();
        m_json["bufferViews"][bufferViewIndex]["buffer"] = 0;
        m_json["bufferViews"][bufferViewIndex]["byteOffset"] = bufferViewFromOffset;
        for (auto i = 0u; i < boneNodes.size(); i++) {
            const float *floatArray = boneNodes[i].inverseBindMatrix.constData();
            for (auto j = 0u; j < 16; j++) {
                binStream << (float)floatArray[j];
            }
        }
        m_json["bufferViews"][bufferViewIndex]["byteLength"] = m_binByteArray.size() - bufferViewFromOffset;
        Q_ASSERT((int)boneNodes.size() * 16 * sizeof(float) == m_binByteArray.size() - bufferViewFromOffset);
        alignBin();
        if (m_enableComment)
            m_json["accessors"][bufferViewIndex]["__comment"] = QString("/accessors/%1: mat").arg(QString::number(bufferViewIndex)).toUtf8().constData();
        m_json["accessors"][bufferViewIndex]["bufferView"] = bufferViewIndex;
        m_json["accessors"][bufferViewIndex]["byteOffset"] = 0;
        m_json["accessors"][bufferViewIndex]["componentType"] = 5126;
        m_json["accessors"][bufferViewIndex]["count"] = boneNodes.size();
        m_json["accessors"][bufferViewIndex]["type"] = "MAT4";
        bufferViewIndex++;
    } else {
        m_json["nodes"][0]["mesh"] = 0;
    }

    std::vector<QVector3D> triangleVertexPositions;
    std::vector<size_t> triangleVertexOldIndices;
    for (const auto &triangleIndices: object.triangles) {
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
        if (resultRigWeights && !resultRigWeights->empty()) {
            m_json["meshes"][0]["primitives"][primitiveIndex]["attributes"]["JOINTS_0"] = bufferViewIndex + (++attributeIndex);
            m_json["meshes"][0]["primitives"][primitiveIndex]["attributes"]["WEIGHTS_0"] = bufferViewIndex + (++attributeIndex);
        }
        int textureIndex = 0;
        m_json["materials"][primitiveIndex]["pbrMetallicRoughness"]["baseColorTexture"]["index"] = textureIndex++;
        m_json["materials"][primitiveIndex]["pbrMetallicRoughness"]["metallicFactor"] = Model::m_defaultMetalness;
        m_json["materials"][primitiveIndex]["pbrMetallicRoughness"]["roughnessFactor"] = Model::m_defaultRoughness;
        if (textureHasTransparencySettings)
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
        for (const auto &position: triangleVertexPositions) {
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
        m_json["bufferViews"][bufferViewIndex]["byteLength"] =  triangleVertexPositions.size() * 3 * sizeof(float);
        m_json["bufferViews"][bufferViewIndex]["target"] = 34962;
        alignBin();
        if (m_enableComment)
            m_json["accessors"][bufferViewIndex]["__comment"] = QString("/accessors/%1: xyz").arg(QString::number(bufferViewIndex)).toUtf8().constData();
        m_json["accessors"][bufferViewIndex]["bufferView"] = bufferViewIndex;
        m_json["accessors"][bufferViewIndex]["byteOffset"] = 0;
        m_json["accessors"][bufferViewIndex]["componentType"] = 5126;
        m_json["accessors"][bufferViewIndex]["count"] =  triangleVertexPositions.size();
        m_json["accessors"][bufferViewIndex]["type"] = "VEC3";
        m_json["accessors"][bufferViewIndex]["max"] = {maxX, maxY, maxZ};
        m_json["accessors"][bufferViewIndex]["min"] = {minX, minY, minZ};
        bufferViewIndex++;
        
        if (m_outputNormal) {
            bufferViewFromOffset = (int)m_binByteArray.size();
            m_json["bufferViews"][bufferViewIndex]["buffer"] = 0;
            m_json["bufferViews"][bufferViewIndex]["byteOffset"] = bufferViewFromOffset;
            QStringList normalList;
            for (const auto &normals: (*triangleVertexNormals)) {
                for (const auto &it: normals) {
                    binStream << (float)it.x() << (float)it.y() << (float)it.z();
                    if (m_enableComment && m_outputNormal)
                        normalList.append(QString("<%1,%2,%3>").arg(QString::number(it.x())).arg(QString::number(it.y())).arg(QString::number(it.z())));
                }
            }
            Q_ASSERT((int)triangleVertexNormals->size() * 3 * 3 * sizeof(float) == m_binByteArray.size() - bufferViewFromOffset);
            m_json["bufferViews"][bufferViewIndex]["byteLength"] =  triangleVertexNormals->size() * 3 * 3 * sizeof(float);
            m_json["bufferViews"][bufferViewIndex]["target"] = 34962;
            alignBin();
            if (m_enableComment)
                m_json["accessors"][bufferViewIndex]["__comment"] = QString("/accessors/%1: normal %2").arg(QString::number(bufferViewIndex)).arg(normalList.join(" ")).toUtf8().constData();
            m_json["accessors"][bufferViewIndex]["bufferView"] = bufferViewIndex;
            m_json["accessors"][bufferViewIndex]["byteOffset"] = 0;
            m_json["accessors"][bufferViewIndex]["componentType"] = 5126;
            m_json["accessors"][bufferViewIndex]["count"] =  triangleVertexNormals->size() * 3;
            m_json["accessors"][bufferViewIndex]["type"] = "VEC3";
            bufferViewIndex++;
        }
        
        if (m_outputUv) {
            bufferViewFromOffset = (int)m_binByteArray.size();
            m_json["bufferViews"][bufferViewIndex]["buffer"] = 0;
            m_json["bufferViews"][bufferViewIndex]["byteOffset"] = bufferViewFromOffset;
            for (const auto &uvs: (*triangleVertexUvs)) {
                for (const auto &it: uvs)
                    binStream << (float)it.x() << (float)it.y();
            }
            m_json["bufferViews"][bufferViewIndex]["byteLength"] = m_binByteArray.size() - bufferViewFromOffset;
            alignBin();
            if (m_enableComment)
                m_json["accessors"][bufferViewIndex]["__comment"] = QString("/accessors/%1: uv").arg(QString::number(bufferViewIndex)).toUtf8().constData();
            m_json["accessors"][bufferViewIndex]["bufferView"] = bufferViewIndex;
            m_json["accessors"][bufferViewIndex]["byteOffset"] = 0;
            m_json["accessors"][bufferViewIndex]["componentType"] = 5126;
            m_json["accessors"][bufferViewIndex]["count"] =  triangleVertexUvs->size() * 3;
            m_json["accessors"][bufferViewIndex]["type"] = "VEC2";
            bufferViewIndex++;
        }
        
        if (resultRigWeights && !resultRigWeights->empty()) {
            bufferViewFromOffset = (int)m_binByteArray.size();
            m_json["bufferViews"][bufferViewIndex]["buffer"] = 0;
            m_json["bufferViews"][bufferViewIndex]["byteOffset"] = bufferViewFromOffset;
            QStringList boneList;
            int weightItIndex = 0;
            for (const auto &oldIndex: triangleVertexOldIndices) {
                auto i = 0u;
                if (m_enableComment)
                    boneList.append(QString("%1:<").arg(QString::number(weightItIndex)));
                auto findWeight = resultRigWeights->find(oldIndex);
                if (findWeight != resultRigWeights->end()) {
                    for (; i < MAX_WEIGHT_NUM; i++) {
                        quint16 nodeIndex = (quint16)findWeight->second.boneIndices[i];
                        binStream << (quint16)nodeIndex;
                        if (m_enableComment)
                            boneList.append(QString("%1").arg(nodeIndex));
                    }
                }
                for (; i < MAX_WEIGHT_NUM; i++) {
                    binStream << (quint16)0;
                    if (m_enableComment)
                        boneList.append(QString("%1").arg(0));
                }
                if (m_enableComment)
                    boneList.append(QString(">"));
                weightItIndex++;
            }
            m_json["bufferViews"][bufferViewIndex]["byteLength"] = m_binByteArray.size() - bufferViewFromOffset;
            alignBin();
            if (m_enableComment)
                m_json["accessors"][bufferViewIndex]["__comment"] = QString("/accessors/%1: bone indices %2").arg(QString::number(bufferViewIndex)).arg(boneList.join(" ")).toUtf8().constData();
            m_json["accessors"][bufferViewIndex]["bufferView"] = bufferViewIndex;
            m_json["accessors"][bufferViewIndex]["byteOffset"] = 0;
            m_json["accessors"][bufferViewIndex]["componentType"] = 5123;
            m_json["accessors"][bufferViewIndex]["count"] =  triangleVertexOldIndices.size();
            m_json["accessors"][bufferViewIndex]["type"] = "VEC4";
            bufferViewIndex++;
            
            bufferViewFromOffset = (int)m_binByteArray.size();
            m_json["bufferViews"][bufferViewIndex]["buffer"] = 0;
            m_json["bufferViews"][bufferViewIndex]["byteOffset"] = bufferViewFromOffset;
            QStringList weightList;
            weightItIndex = 0;
            for (const auto &oldIndex: triangleVertexOldIndices) {
                auto i = 0u;
                if (m_enableComment)
                    weightList.append(QString("%1:<").arg(QString::number(weightItIndex)));
                auto findWeight = resultRigWeights->find(oldIndex);
                if (findWeight != resultRigWeights->end()) {
                    for (; i < MAX_WEIGHT_NUM; i++) {
                        float weight = (float)findWeight->second.boneWeights[i];
                        binStream << (float)weight;
                        if (m_enableComment)
                            weightList.append(QString("%1").arg(QString::number((float)weight)));
                    }
                }
                for (; i < MAX_WEIGHT_NUM; i++) {
                    binStream << (float)0.0;
                    if (m_enableComment)
                        weightList.append(QString("%1").arg(QString::number(0.0)));
                }
                if (m_enableComment)
                    weightList.append(QString(">"));
                weightItIndex++;
            }
            m_json["bufferViews"][bufferViewIndex]["byteLength"] = m_binByteArray.size() - bufferViewFromOffset;
            alignBin();
            if (m_enableComment)
                m_json["accessors"][bufferViewIndex]["__comment"] = QString("/accessors/%1: bone weights %2").arg(QString::number(bufferViewIndex)).arg(weightList.join(" ")).toUtf8().constData();
            m_json["accessors"][bufferViewIndex]["bufferView"] = bufferViewIndex;
            m_json["accessors"][bufferViewIndex]["byteOffset"] = 0;
            m_json["accessors"][bufferViewIndex]["componentType"] = 5126;
            m_json["accessors"][bufferViewIndex]["count"] = triangleVertexOldIndices.size();
            m_json["accessors"][bufferViewIndex]["type"] = "VEC4";
            bufferViewIndex++;
        }
    }
    
    if (m_outputAnimation) {
        for (int animationIndex = 0; animationIndex < (int)motions->size(); ++animationIndex) {
            const auto &motion = (*motions)[animationIndex];
            
            m_json["animations"][animationIndex]["name"] = motion.first.toUtf8().constData();
            
            int input = bufferViewIndex;
            bufferViewFromOffset = (int)m_binByteArray.size();
            m_json["bufferViews"][bufferViewIndex]["buffer"] = 0;
            m_json["bufferViews"][bufferViewIndex]["byteOffset"] = bufferViewFromOffset;
            float minTime = 1000000.0;
            float maxTime = 0.0;
            QStringList timeList;
            float timePoint = 0;
            for (const auto &keyframe: motion.second) {
                binStream << (float)timePoint;
                if (timePoint < minTime)
                    minTime = timePoint;
                if (timePoint > maxTime)
                    maxTime = timePoint;
                if (m_enableComment)
                    timeList.append(QString::number(timePoint));
                timePoint += keyframe.first;
            }
            m_json["bufferViews"][bufferViewIndex]["byteLength"] = m_binByteArray.size() - bufferViewFromOffset;
            alignBin();
            if (m_enableComment)
                m_json["accessors"][bufferViewIndex]["__comment"] = QString("/accessors/%1: times %2").arg(QString::number(bufferViewIndex)).arg(timeList.join(" ")).toUtf8().constData();
            m_json["accessors"][bufferViewIndex]["bufferView"] = bufferViewIndex;
            m_json["accessors"][bufferViewIndex]["byteOffset"] = 0;
            m_json["accessors"][bufferViewIndex]["componentType"] = 5126;
            m_json["accessors"][bufferViewIndex]["count"] =  motion.second.size();
            m_json["accessors"][bufferViewIndex]["type"] = "SCALAR";
            m_json["accessors"][bufferViewIndex]["max"][0] = maxTime;
            m_json["accessors"][bufferViewIndex]["min"][0] = minTime;
            bufferViewIndex++;
            
            std::set<int> rotatedJoints;
            std::set<int> translatedJoints;
            
            for (const auto &keyframe: motion.second) {
                for (int i = 0; i < (int)keyframe.second.nodes().size() && i < (int)boneNodes.size(); ++i) {
                    const auto &src = boneNodes[i];
                    const auto &dest = keyframe.second.nodes()[i];
                    if (!qFuzzyCompare(src.rotation, dest.rotation))
                        rotatedJoints.insert(i);
                    if (!qFuzzyCompare(src.translation, dest.translation))
                        translatedJoints.insert(i);
                }
            }
            
            int sampler = 0;
            int channel = 0;
            
            for (const auto &jointIndex: rotatedJoints) {
                int output = bufferViewIndex;
                bufferViewFromOffset = (int)m_binByteArray.size();
                m_json["bufferViews"][bufferViewIndex]["buffer"] = 0;
                m_json["bufferViews"][bufferViewIndex]["byteOffset"] = bufferViewFromOffset;
                QStringList rotationList;
                for (int frame = 0; frame < (int)motion.second.size(); frame++) {
                    const auto &keyframe = motion.second[frame];
                    const auto &rotation = keyframe.second.nodes()[jointIndex].rotation;
                    float x = rotation.x();
                    float y = rotation.y();
                    float z = rotation.z();
                    float w = rotation.scalar();
                    binStream << (float)x << (float)y << (float)z << (float)w;
                    if (m_enableComment)
                        rotationList.append(QString("%1:<%2,%3,%4,%5>").arg(QString::number(frame)).arg(QString::number(x)).arg(QString::number(y)).arg(QString::number(z)).arg(QString::number(w)));
                }
                m_json["bufferViews"][bufferViewIndex]["byteLength"] = m_binByteArray.size() - bufferViewFromOffset;
                alignBin();
                if (m_enableComment)
                    m_json["accessors"][bufferViewIndex]["__comment"] = QString("/accessors/%1: rotation %2").arg(QString::number(bufferViewIndex)).arg(rotationList.join(" ")).toUtf8().constData();
                m_json["accessors"][bufferViewIndex]["bufferView"] = bufferViewIndex;
                m_json["accessors"][bufferViewIndex]["byteOffset"] = 0;
                m_json["accessors"][bufferViewIndex]["componentType"] = 5126;
                m_json["accessors"][bufferViewIndex]["count"] =  motion.second.size();
                m_json["accessors"][bufferViewIndex]["type"] = "VEC4";
                bufferViewIndex++;

                m_json["animations"][animationIndex]["samplers"][sampler]["input"] = input;
                m_json["animations"][animationIndex]["samplers"][sampler]["interpolation"] = "LINEAR";
                m_json["animations"][animationIndex]["samplers"][sampler]["output"] = output;
                
                m_json["animations"][animationIndex]["channels"][channel]["sampler"] = sampler;
                m_json["animations"][animationIndex]["channels"][channel]["target"]["node"] = skeletonNodeStartIndex + jointIndex;
                m_json["animations"][animationIndex]["channels"][channel]["target"]["path"] = "rotation";
                
                sampler++;
                channel++;
                
                for (const auto &jointIndex: translatedJoints) {
                    int output = bufferViewIndex;
                    bufferViewFromOffset = (int)m_binByteArray.size();
                    m_json["bufferViews"][bufferViewIndex]["buffer"] = 0;
                    m_json["bufferViews"][bufferViewIndex]["byteOffset"] = bufferViewFromOffset;
                    for (int frame = 0; frame < (int)motion.second.size(); frame++) {
                        const auto &keyframe = motion.second[frame];
                        const auto &translation = keyframe.second.nodes()[jointIndex].translation;
                        binStream << (float)translation.x() << (float)translation.y() << (float)translation.z();
                    }
                    m_json["bufferViews"][bufferViewIndex]["byteLength"] = m_binByteArray.size() - bufferViewFromOffset;
                    alignBin();
                    if (m_enableComment)
                        m_json["accessors"][bufferViewIndex]["__comment"] = QString("/accessors/%1: translation").arg(QString::number(bufferViewIndex)).toUtf8().constData();
                    m_json["accessors"][bufferViewIndex]["bufferView"] = bufferViewIndex;
                    m_json["accessors"][bufferViewIndex]["byteOffset"] = 0;
                    m_json["accessors"][bufferViewIndex]["componentType"] = 5126;
                    m_json["accessors"][bufferViewIndex]["count"] =  motion.second.size();
                    m_json["accessors"][bufferViewIndex]["type"] = "VEC3";
                    bufferViewIndex++;

                    m_json["animations"][animationIndex]["samplers"][sampler]["input"] = input;
                    m_json["animations"][animationIndex]["samplers"][sampler]["interpolation"] = "LINEAR";
                    m_json["animations"][animationIndex]["samplers"][sampler]["output"] = output;
                    
                    m_json["animations"][animationIndex]["channels"][channel]["sampler"] = sampler;
                    m_json["animations"][animationIndex]["channels"][channel]["target"]["node"] = skeletonNodeStartIndex + jointIndex;
                    m_json["animations"][animationIndex]["channels"][channel]["target"]["path"] = "translation";
                    
                    sampler++;
                    channel++;
                }
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

bool GlbFileWriter::save()
{
    QFile file(m_filename);
    if (!file.open(QIODevice::WriteOnly)) {
        return false;
    }
    QDataStream output(&file);
    output.setFloatingPointPrecision(QDataStream::SinglePrecision);
    output.setByteOrder(QDataStream::LittleEndian);
    
    uint32_t headerSize = 12;
    uint32_t chunk0DescriptionSize = 8;
    uint32_t chunk1DescriptionSize = 8;
    uint32_t fileSize = headerSize +
        chunk0DescriptionSize + m_jsonByteArray.size() +
        chunk1DescriptionSize + m_binByteArray.size();
    
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
