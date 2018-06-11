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

// Play with glTF online:
// https://gltf-viewer.donmccurdy.com/

// https://github.com/KhronosGroup/glTF-Tutorials/blob/master/gltfTutorial/gltfTutorial_020_Skins.md
// https://github.com/KhronosGroup/glTF-Tutorials/blob/master/gltfTutorial/gltfTutorial_004_ScenesNodes.md#global-transforms-of-nodes

// http://quaternions.online/
// https://en.m.wikipedia.org/wiki/Rotation_formalisms_in_three_dimensions?wprov=sfla1

bool GLTFFileWriter::m_enableComment = false;

GLTFFileWriter::GLTFFileWriter(MeshResultContext &resultContext, const QString &filename) :
    m_filename(filename),
    m_outputNormal(true)
{
    const BmeshNode *rootNode = resultContext.centerBmeshNode();
    if (!rootNode) {
        qDebug() << "Cannot export gltf because lack of root node";
        return;
    }
    
    QFileInfo nameInfo(filename);
    QString textureFilenameWithoutPath = nameInfo.completeBaseName() + ".png";
    m_textureFilename = nameInfo.path() + QDir::separator() + textureFilenameWithoutPath;
    
    JointNodeTree jointNodeTree(resultContext);
    const std::vector<JointInfo> &tracedJoints = jointNodeTree.joints();
    
    m_json["asset"]["version"] = "2.0";
    m_json["asset"]["generator"] = APP_NAME " " APP_HUMAN_VER;
    m_json["scenes"][0]["nodes"] = {0};
    
    m_json["nodes"][0]["mesh"] = 0;
    m_json["nodes"][0]["skin"] = 0;
    m_json["nodes"][0]["children"] = {1};
    
    int skeletonNodeStartIndex = 1;

    for (auto i = 0u; i < tracedJoints.size(); i++) {
        m_json["nodes"][skeletonNodeStartIndex + i]["translation"] = {tracedJoints[i].translation.x(),
            tracedJoints[i].translation.y(),
            tracedJoints[i].translation.z()
        };
        m_json["nodes"][skeletonNodeStartIndex + i]["rotation"] = {tracedJoints[i].rotation.x(),
            tracedJoints[i].rotation.y(),
            tracedJoints[i].rotation.z(),
            tracedJoints[i].rotation.scalar()
        };
        if (tracedJoints[i].children.empty())
            continue;
        m_json["nodes"][skeletonNodeStartIndex + i]["children"] = {};
        for (const auto &it: tracedJoints[i].children) {
            m_json["nodes"][skeletonNodeStartIndex + i]["children"] += skeletonNodeStartIndex + it;
        }
    }
    
    m_json["skins"][0]["inverseBindMatrices"] = 0;
    m_json["skins"][0]["skeleton"] = skeletonNodeStartIndex;
    m_json["skins"][0]["joints"] = {};
    for (auto i = 0u; i < tracedJoints.size(); i++) {
        m_json["skins"][0]["joints"] += skeletonNodeStartIndex + i;
    }
    
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
    
    if (m_enableComment)
        m_json["accessors"][bufferViewIndex]["__comment"] = QString("/accessors/%1: mat").arg(QString::number(bufferViewIndex)).toUtf8().constData();
    m_json["accessors"][bufferViewIndex]["bufferView"] = bufferViewIndex;
    m_json["accessors"][bufferViewIndex]["byteOffset"] = 0;
    m_json["accessors"][bufferViewIndex]["componentType"] = 5126;
    m_json["accessors"][bufferViewIndex]["count"] = tracedJoints.size();
    m_json["accessors"][bufferViewIndex]["type"] = "MAT4";
    m_json["bufferViews"][bufferViewIndex]["buffer"] = 0;
    m_json["bufferViews"][bufferViewIndex]["byteOffset"] = (int)binaries.size();
    int bufferViews0FromOffset = (int)binaries.size();
    for (auto i = 0u; i < tracedJoints.size(); i++) {
        const float *floatArray = tracedJoints[i].inverseBindMatrix.constData();
        for (auto j = 0u; j < 16; j++) {
            stream << (float)floatArray[j];
        }
    }
    m_json["bufferViews"][bufferViewIndex]["byteLength"] = binaries.size() - bufferViews0FromOffset;
    alignBinaries();
    bufferViewIndex++;
    
    m_json["textures"][0]["sampler"] = 0;
    m_json["textures"][0]["source"] = 0;
    
    m_json["images"][0]["uri"] = textureFilenameWithoutPath.toUtf8().constData();
    
    m_json["samplers"][0]["magFilter"] = 9729;
    m_json["samplers"][0]["minFilter"] = 9987;
    m_json["samplers"][0]["wrapS"] = 33648;
    m_json["samplers"][0]["wrapT"] = 33648;

    int primitiveIndex = 0;
    for (const auto &part: resultContext.parts()) {
        int bufferViewFromOffset;
        
        m_json["meshes"][0]["primitives"][primitiveIndex]["indices"] = bufferViewIndex;
        m_json["meshes"][0]["primitives"][primitiveIndex]["material"] = primitiveIndex;
        int attributeIndex = 0;
        m_json["meshes"][0]["primitives"][primitiveIndex]["attributes"]["POSITION"] = bufferViewIndex + (++attributeIndex);
        if (m_outputNormal)
            m_json["meshes"][0]["primitives"][primitiveIndex]["attributes"]["NORMAL"] = bufferViewIndex + (++attributeIndex);
        m_json["meshes"][0]["primitives"][primitiveIndex]["attributes"]["JOINTS_0"] = bufferViewIndex + (++attributeIndex);
        m_json["meshes"][0]["primitives"][primitiveIndex]["attributes"]["WEIGHTS_0"] = bufferViewIndex + (++attributeIndex);
        m_json["meshes"][0]["primitives"][primitiveIndex]["attributes"]["TEXCOORD_0"] = bufferViewIndex + (++attributeIndex);
        /*
        m_json["materials"][primitiveIndex]["pbrMetallicRoughness"]["baseColorFactor"] = {
            part.second.color.redF(),
            part.second.color.greenF(),
            part.second.color.blueF(),
            1.0
        };*/
        m_json["materials"][primitiveIndex]["pbrMetallicRoughness"]["baseColorTexture"]["index"] = 0;
        m_json["materials"][primitiveIndex]["pbrMetallicRoughness"]["metallicFactor"] = 0.0;
        m_json["materials"][primitiveIndex]["pbrMetallicRoughness"]["roughnessFactor"] = 1.0;
        
        primitiveIndex++;

        bufferViewFromOffset = (int)binaries.size();
        for (const auto &it: part.second.triangles) {
            stream << (quint16)it.indicies[0] << (quint16)it.indicies[1] << (quint16)it.indicies[2];
        }
        m_json["bufferViews"][bufferViewIndex]["buffer"] = 0;
        m_json["bufferViews"][bufferViewIndex]["byteOffset"] = bufferViewFromOffset;
        m_json["bufferViews"][bufferViewIndex]["byteLength"] = (int)part.second.triangles.size() * 3 * sizeof(quint16);
        m_json["bufferViews"][bufferViewIndex]["target"] = 34963;
        Q_ASSERT(part.second.triangles.size() * 3 * sizeof(quint16) == binaries.size() - bufferViewFromOffset);
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
        Q_ASSERT( part.second.vertices.size() * 3 * sizeof(float) == binaries.size() - bufferViewFromOffset);
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
        
        if (m_outputNormal) {
            bufferViewFromOffset = (int)binaries.size();
            m_json["bufferViews"][bufferViewIndex]["buffer"] = 0;
            m_json["bufferViews"][bufferViewIndex]["byteOffset"] = bufferViewFromOffset;
            QStringList normalList;
            for (const auto &it: part.second.interpolatedVertexNormals) {
                stream << (float)it.x() << (float)it.y() << (float)it.z();
                if (m_outputNormal)
                    normalList.append(QString("<%1,%2,%3>").arg(QString::number(it.x())).arg(QString::number(it.y())).arg(QString::number(it.z())));
            }
            Q_ASSERT( part.second.interpolatedVertexNormals.size() * 3 * sizeof(float) == binaries.size() - bufferViewFromOffset);
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
        }
        
        bufferViewFromOffset = (int)binaries.size();
        m_json["bufferViews"][bufferViewIndex]["buffer"] = 0;
        m_json["bufferViews"][bufferViewIndex]["byteOffset"] = bufferViewFromOffset;
        for (const auto &it: part.second.weights) {
            auto i = 0u;
            for (; i < it.size() && i < MAX_WEIGHT_NUM; i++) {
                stream << (quint16)jointNodeTree.nodeToJointIndex(it[i].sourceNode.first, it[i].sourceNode.second);
            }
            for (; i < MAX_WEIGHT_NUM; i++) {
                stream << (quint16)0;
            }
        }
        m_json["bufferViews"][bufferViewIndex]["byteLength"] = binaries.size() - bufferViewFromOffset;
        alignBinaries();
        if (m_enableComment)
            m_json["accessors"][bufferViewIndex]["__comment"] = QString("/accessors/%1: bone indicies").arg(QString::number(bufferViewIndex)).toUtf8().constData();
        m_json["accessors"][bufferViewIndex]["bufferView"] = bufferViewIndex;
        m_json["accessors"][bufferViewIndex]["byteOffset"] = 0;
        m_json["accessors"][bufferViewIndex]["componentType"] = 5123;
        m_json["accessors"][bufferViewIndex]["count"] =  part.second.weights.size();
        m_json["accessors"][bufferViewIndex]["type"] = "VEC4";
        bufferViewIndex++;
        
        bufferViewFromOffset = (int)binaries.size();
        m_json["bufferViews"][bufferViewIndex]["buffer"] = 0;
        m_json["bufferViews"][bufferViewIndex]["byteOffset"] = bufferViewFromOffset;
        for (const auto &it: part.second.weights) {
            auto i = 0u;
            for (; i < it.size() && i < MAX_WEIGHT_NUM; i++) {
                stream << (float)it[i].weight;
            }
            for (; i < MAX_WEIGHT_NUM; i++) {
                stream << (float)0.0;
            }
        }
        m_json["bufferViews"][bufferViewIndex]["byteLength"] = binaries.size() - bufferViewFromOffset;
        alignBinaries();
        if (m_enableComment)
            m_json["accessors"][bufferViewIndex]["__comment"] = QString("/accessors/%1: bone weights").arg(QString::number(bufferViewIndex)).toUtf8().constData();
        m_json["accessors"][bufferViewIndex]["bufferView"] = bufferViewIndex;
        m_json["accessors"][bufferViewIndex]["byteOffset"] = 0;
        m_json["accessors"][bufferViewIndex]["componentType"] = 5126;
        m_json["accessors"][bufferViewIndex]["count"] =  part.second.weights.size();
        m_json["accessors"][bufferViewIndex]["type"] = "VEC4";
        bufferViewIndex++;
        
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
    
    m_json["buffers"][0]["uri"] = QString("data:application/octet-stream;base64," + binaries.toBase64()).toUtf8().constData();
    m_json["buffers"][0]["byteLength"] = binaries.size();
}

const QString &GLTFFileWriter::textureFilenameInGltf()
{
    return m_textureFilename;
}

bool GLTFFileWriter::save()
{
    QFile file(m_filename);
    if (!file.open(QIODevice::WriteOnly)) {
        return false;
    }
    file.write(QString::fromStdString(m_json.dump(4)).toUtf8());
    return true;
}
