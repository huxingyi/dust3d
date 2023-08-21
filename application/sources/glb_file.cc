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

bool GlbFileWriter::m_enableComment = false;

GlbFileWriter::GlbFileWriter(dust3d::Object& object,
    const QString& filename,
    QImage* textureImage,
    QImage* normalImage,
    QImage* ormImage)
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

    m_json["asset"]["version"] = "2.0";
    m_json["asset"]["generator"] = APP_NAME " " APP_HUMAN_VER;
    m_json["scenes"][0]["nodes"] = { 0 };

    m_json["nodes"][0]["mesh"] = 0;

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
