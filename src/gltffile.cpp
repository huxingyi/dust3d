#include <QFile>
#include <QQuaternion>
#include <QByteArray>
#include <QDataStream>
#include <QFileInfo>
#include <QDir>
#include "gltffile.h"
#include "version.h"
#include "dust3dutil.h"
#include "jointnodetree.h"
#include "meshloader.h"

// Play with glTF online:
// https://gltf-viewer.donmccurdy.com/

// https://github.com/KhronosGroup/glTF-Tutorials/blob/master/gltfTutorial/gltfTutorial_020_Skins.md
// https://github.com/KhronosGroup/glTF-Tutorials/blob/master/gltfTutorial/gltfTutorial_004_ScenesNodes.md#global-transforms-of-nodes

// http://quaternions.online/
// https://en.m.wikipedia.org/wiki/Rotation_formalisms_in_three_dimensions?wprov=sfla1

bool GltfFileWriter::m_enableComment = true;

GltfFileWriter::GltfFileWriter(MeshResultContext &resultContext,
        const std::vector<AutoRiggerBone> *resultRigBones,
        const std::map<int, AutoRiggerVertexWeights> *resultRigWeights,
        const QString &filename) :
    m_filename(filename),
    m_outputNormal(false),
    m_outputAnimation(true),
    m_outputUv(true),
    m_testOutputAsWhole(false)
{
    QFileInfo nameInfo(filename);
    QString textureFilenameWithoutPath = nameInfo.completeBaseName() + ".png";
    m_textureFilename = nameInfo.path() + QDir::separator() + textureFilenameWithoutPath;
    
    QByteArray binaries;
    QDataStream stream(&binaries, QIODevice::WriteOnly);
    stream.setFloatingPointPrecision(QDataStream::SinglePrecision);
    stream.setByteOrder(QDataStream::LittleEndian);
    
    auto alignBinaries = [&binaries, &stream] {
        while (0 != binaries.size() % 4) {
            stream << (quint8)0;
        }
    };
    
    int bufferViewIndex = 0;
    int bufferViewFromOffset;
    
    JointNodeTree jointNodeTree(resultRigBones);
    const auto &boneNodes = jointNodeTree.nodes();
    
    m_json["asset"]["version"] = "2.0";
    m_json["asset"]["generator"] = APP_NAME " " APP_HUMAN_VER;
    m_json["scenes"][0]["nodes"] = {0};
    
    if (resultRigBones && resultRigWeights && !resultRigBones->empty()) {

        constexpr int skeletonNodeStartIndex = 2;
        
        m_json["nodes"][0]["children"] = {
            1,
            skeletonNodeStartIndex
        };
        
        m_json["nodes"][1]["mesh"] = 0;
        m_json["nodes"][1]["skin"] = 0;
        
        m_json["skins"][0]["joints"] = {};
        for (size_t i = 0; i < boneNodes.size(); i++) {
            m_json["skins"][0]["joints"] += skeletonNodeStartIndex + i;
            
            m_json["nodes"][skeletonNodeStartIndex + i]["name"] = boneNodes[i].name.toUtf8().constData();
            m_json["nodes"][skeletonNodeStartIndex + i]["translation"] = {
                boneNodes[i].translation.x(),
                boneNodes[i].translation.y(),
                boneNodes[i].translation.z()
            };
            m_json["nodes"][skeletonNodeStartIndex + i]["rotation"] = {
                boneNodes[i].rotation.x(),
                boneNodes[i].rotation.y(),
                boneNodes[i].rotation.z(),
                boneNodes[i].rotation.scalar()
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
        bufferViewFromOffset = (int)binaries.size();
        m_json["bufferViews"][bufferViewIndex]["buffer"] = 0;
        m_json["bufferViews"][bufferViewIndex]["byteOffset"] = bufferViewFromOffset;
        for (auto i = 0u; i < boneNodes.size(); i++) {
            const float *floatArray = boneNodes[i].inverseBindMatrix.constData();
            for (auto j = 0u; j < 16; j++) {
                stream << (float)floatArray[j];
            }
        }
        m_json["bufferViews"][bufferViewIndex]["byteLength"] = binaries.size() - bufferViewFromOffset;
        Q_ASSERT((int)boneNodes.size() * 16 * sizeof(float) == binaries.size() - bufferViewFromOffset);
        alignBinaries();
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

    m_json["textures"][0]["sampler"] = 0;
    m_json["textures"][0]["source"] = 0;
    
    m_json["images"][0]["uri"] = textureFilenameWithoutPath.toUtf8().constData();
    
    m_json["samplers"][0]["magFilter"] = 9729;
    m_json["samplers"][0]["minFilter"] = 9987;
    m_json["samplers"][0]["wrapS"] = 33648;
    m_json["samplers"][0]["wrapT"] = 33648;
    
    const std::map<QUuid, ResultPart> *parts = &resultContext.parts();
    
    std::map<QUuid, ResultPart> testParts;
    if (m_testOutputAsWhole) {
        testParts[0].vertices = resultContext.vertices;
        testParts[0].triangles = resultContext.triangles;
        
        m_outputNormal = false;
        m_outputUv = false;
        
        parts = &testParts;
    }

    int primitiveIndex = 0;
    for (const auto &part: *parts) {
        
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
        m_json["materials"][primitiveIndex]["pbrMetallicRoughness"]["baseColorTexture"]["index"] = 0;
        m_json["materials"][primitiveIndex]["pbrMetallicRoughness"]["metallicFactor"] = MeshLoader::m_defaultMetalness;
        m_json["materials"][primitiveIndex]["pbrMetallicRoughness"]["roughnessFactor"] = MeshLoader::m_defaultRoughness;
        
        primitiveIndex++;

        bufferViewFromOffset = (int)binaries.size();
        for (const auto &it: part.second.triangles) {
            stream << (quint16)it.indicies[0] << (quint16)it.indicies[1] << (quint16)it.indicies[2];
        }
        m_json["bufferViews"][bufferViewIndex]["buffer"] = 0;
        m_json["bufferViews"][bufferViewIndex]["byteOffset"] = bufferViewFromOffset;
        m_json["bufferViews"][bufferViewIndex]["byteLength"] = (int)part.second.triangles.size() * 3 * sizeof(quint16);
        m_json["bufferViews"][bufferViewIndex]["target"] = 34963;
        Q_ASSERT((int)part.second.triangles.size() * 3 * sizeof(quint16) == binaries.size() - bufferViewFromOffset);
        alignBinaries();
        if (m_enableComment)
            m_json["accessors"][bufferViewIndex]["__comment"] = QString("/accessors/%1: triangle indicies").arg(QString::number(bufferViewIndex)).toUtf8().constData();
        m_json["accessors"][bufferViewIndex]["bufferView"] = bufferViewIndex;
        m_json["accessors"][bufferViewIndex]["byteOffset"] = 0;
        m_json["accessors"][bufferViewIndex]["componentType"] = 5123;
        m_json["accessors"][bufferViewIndex]["count"] = part.second.triangles.size() * 3;
        m_json["accessors"][bufferViewIndex]["type"] = "SCALAR";
        bufferViewIndex++;
        
        bufferViewFromOffset = (int)binaries.size();
        m_json["bufferViews"][bufferViewIndex]["buffer"] = 0;
        m_json["bufferViews"][bufferViewIndex]["byteOffset"] = bufferViewFromOffset;
        float minX = 100;
        float maxX = -100;
        float minY = 100;
        float maxY = -100;
        float minZ = 100;
        float maxZ = -100;
        for (const auto &it: part.second.vertices) {
            if (it.position.x() < minX)
                minX = it.position.x();
            if (it.position.x() > maxX)
                maxX = it.position.x();
            if (it.position.y() < minY)
                minY = it.position.y();
            if (it.position.y() > maxY)
                maxY = it.position.y();
            if (it.position.z() < minZ)
                minZ = it.position.z();
            if (it.position.z() > maxZ)
                maxZ = it.position.z();
            stream << (float)it.position.x() << (float)it.position.y() << (float)it.position.z();
        }
        Q_ASSERT((int)part.second.vertices.size() * 3 * sizeof(float) == binaries.size() - bufferViewFromOffset);
        m_json["bufferViews"][bufferViewIndex]["byteLength"] =  part.second.vertices.size() * 3 * sizeof(float);
        m_json["bufferViews"][bufferViewIndex]["target"] = 34962;
        alignBinaries();
        if (m_enableComment)
            m_json["accessors"][bufferViewIndex]["__comment"] = QString("/accessors/%1: xyz").arg(QString::number(bufferViewIndex)).toUtf8().constData();
        m_json["accessors"][bufferViewIndex]["bufferView"] = bufferViewIndex;
        m_json["accessors"][bufferViewIndex]["byteOffset"] = 0;
        m_json["accessors"][bufferViewIndex]["componentType"] = 5126;
        m_json["accessors"][bufferViewIndex]["count"] =  part.second.vertices.size();
        m_json["accessors"][bufferViewIndex]["type"] = "VEC3";
        m_json["accessors"][bufferViewIndex]["max"] = {maxX, maxY, maxZ};
        m_json["accessors"][bufferViewIndex]["min"] = {minX, minY, minZ};
        bufferViewIndex++;
        
        /*
        if (m_outputNormal) {
            bufferViewFromOffset = (int)binaries.size();
            m_json["bufferViews"][bufferViewIndex]["buffer"] = 0;
            m_json["bufferViews"][bufferViewIndex]["byteOffset"] = bufferViewFromOffset;
            QStringList normalList;
            for (const auto &it: part.second.interpolatedVertexNormals) {
                stream << (float)it.x() << (float)it.y() << (float)it.z();
                if (m_enableComment && m_outputNormal)
                    normalList.append(QString("<%1,%2,%3>").arg(QString::number(it.x())).arg(QString::number(it.y())).arg(QString::number(it.z())));
            }
            Q_ASSERT((int)part.second.interpolatedVertexNormals.size() * 3 * sizeof(float) == binaries.size() - bufferViewFromOffset);
            m_json["bufferViews"][bufferViewIndex]["byteLength"] =  part.second.vertices.size() * 3 * sizeof(float);
            m_json["bufferViews"][bufferViewIndex]["target"] = 34962;
            alignBinaries();
            if (m_enableComment)
                m_json["accessors"][bufferViewIndex]["__comment"] = QString("/accessors/%1: normal %2").arg(QString::number(bufferViewIndex)).arg(normalList.join(" ")).toUtf8().constData();
            m_json["accessors"][bufferViewIndex]["bufferView"] = bufferViewIndex;
            m_json["accessors"][bufferViewIndex]["byteOffset"] = 0;
            m_json["accessors"][bufferViewIndex]["componentType"] = 5126;
            m_json["accessors"][bufferViewIndex]["count"] =  part.second.vertices.size();
            m_json["accessors"][bufferViewIndex]["type"] = "VEC3";
            bufferViewIndex++;
        }*/
        
        if (m_outputUv) {
            bufferViewFromOffset = (int)binaries.size();
            m_json["bufferViews"][bufferViewIndex]["buffer"] = 0;
            m_json["bufferViews"][bufferViewIndex]["byteOffset"] = bufferViewFromOffset;
            for (const auto &it: part.second.vertexUvs) {
                stream << (float)it.uv[0] << (float)it.uv[1];
            }
            m_json["bufferViews"][bufferViewIndex]["byteLength"] = binaries.size() - bufferViewFromOffset;
            alignBinaries();
            if (m_enableComment)
                m_json["accessors"][bufferViewIndex]["__comment"] = QString("/accessors/%1: uv").arg(QString::number(bufferViewIndex)).toUtf8().constData();
            m_json["accessors"][bufferViewIndex]["bufferView"] = bufferViewIndex;
            m_json["accessors"][bufferViewIndex]["byteOffset"] = 0;
            m_json["accessors"][bufferViewIndex]["componentType"] = 5126;
            m_json["accessors"][bufferViewIndex]["count"] =  part.second.vertexUvs.size();
            m_json["accessors"][bufferViewIndex]["type"] = "VEC2";
            bufferViewIndex++;
        }
        
        if (resultRigWeights && !resultRigWeights->empty()) {
            bufferViewFromOffset = (int)binaries.size();
            m_json["bufferViews"][bufferViewIndex]["buffer"] = 0;
            m_json["bufferViews"][bufferViewIndex]["byteOffset"] = bufferViewFromOffset;
            QStringList boneList;
            int weightItIndex = 0;
            for (const auto &oldIndex: part.second.verticesOldIndicies) {
                auto i = 0u;
                if (m_enableComment)
                    boneList.append(QString("%1:<").arg(QString::number(weightItIndex)));
                auto findWeight = resultRigWeights->find(oldIndex);
                if (findWeight != resultRigWeights->end()) {
                    for (; i < MAX_WEIGHT_NUM; i++) {
                        quint16 nodeIndex = (quint16)findWeight->second.boneIndicies[i];
                        stream << (quint16)nodeIndex;
                        if (m_enableComment)
                            boneList.append(QString("%1").arg(nodeIndex));
                    }
                }
                for (; i < MAX_WEIGHT_NUM; i++) {
                    stream << (quint16)0;
                    if (m_enableComment)
                        boneList.append(QString("%1").arg(0));
                }
                if (m_enableComment)
                    boneList.append(QString(">"));
                weightItIndex++;
            }
            m_json["bufferViews"][bufferViewIndex]["byteLength"] = binaries.size() - bufferViewFromOffset;
            alignBinaries();
            if (m_enableComment)
                m_json["accessors"][bufferViewIndex]["__comment"] = QString("/accessors/%1: bone indicies %2").arg(QString::number(bufferViewIndex)).arg(boneList.join(" ")).toUtf8().constData();
            m_json["accessors"][bufferViewIndex]["bufferView"] = bufferViewIndex;
            m_json["accessors"][bufferViewIndex]["byteOffset"] = 0;
            m_json["accessors"][bufferViewIndex]["componentType"] = 5123;
            m_json["accessors"][bufferViewIndex]["count"] =  part.second.verticesOldIndicies.size();
            m_json["accessors"][bufferViewIndex]["type"] = "VEC4";
            bufferViewIndex++;
            
            bufferViewFromOffset = (int)binaries.size();
            m_json["bufferViews"][bufferViewIndex]["buffer"] = 0;
            m_json["bufferViews"][bufferViewIndex]["byteOffset"] = bufferViewFromOffset;
            QStringList weightList;
            weightItIndex = 0;
            for (const auto &oldIndex: part.second.verticesOldIndicies) {
                auto i = 0u;
                if (m_enableComment)
                    weightList.append(QString("%1:<").arg(QString::number(weightItIndex)));
                auto findWeight = resultRigWeights->find(oldIndex);
                if (findWeight != resultRigWeights->end()) {
                    for (; i < MAX_WEIGHT_NUM; i++) {
                        float weight = (float)findWeight->second.boneWeights[i];
                        stream << (float)weight;
                        if (m_enableComment)
                            weightList.append(QString("%1").arg(QString::number((float)weight)));
                    }
                }
                for (; i < MAX_WEIGHT_NUM; i++) {
                    stream << (float)0.0;
                    if (m_enableComment)
                        weightList.append(QString("%1").arg(QString::number(0.0)));
                }
                if (m_enableComment)
                    weightList.append(QString(">"));
                weightItIndex++;
            }
            m_json["bufferViews"][bufferViewIndex]["byteLength"] = binaries.size() - bufferViewFromOffset;
            alignBinaries();
            if (m_enableComment)
                m_json["accessors"][bufferViewIndex]["__comment"] = QString("/accessors/%1: bone weights %2").arg(QString::number(bufferViewIndex)).arg(weightList.join(" ")).toUtf8().constData();
            m_json["accessors"][bufferViewIndex]["bufferView"] = bufferViewIndex;
            m_json["accessors"][bufferViewIndex]["byteOffset"] = 0;
            m_json["accessors"][bufferViewIndex]["componentType"] = 5126;
            m_json["accessors"][bufferViewIndex]["count"] =  part.second.verticesOldIndicies.size();
            m_json["accessors"][bufferViewIndex]["type"] = "VEC4";
            bufferViewIndex++;
        }
    }
    
    m_json["buffers"][0]["uri"] = QString("data:application/octet-stream;base64," + binaries.toBase64()).toUtf8().constData();
    m_json["buffers"][0]["byteLength"] = binaries.size();
}

const QString &GltfFileWriter::textureFilenameInGltf()
{
    return m_textureFilename;
}

bool GltfFileWriter::save()
{
    QFile file(m_filename);
    if (!file.open(QIODevice::WriteOnly)) {
        return false;
    }
    file.write(QString::fromStdString(m_json.dump(4)).toUtf8());
    return true;
}
