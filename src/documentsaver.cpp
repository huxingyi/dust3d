#include <QXmlStreamWriter>
#include <set>
#include <QGuiApplication>
#include <QtCore/qbuffer.h>
#include "documentsaver.h"
#include "imageforever.h"
#include "ds3file.h"
#include "snapshotxml.h"
#include "variablesxml.h"
#include "fileforever.h"
#include "objectxml.h"

DocumentSaver::DocumentSaver(const QString *filename, 
        Snapshot *snapshot,
        Object *object,
        DocumentSaver::Textures *textures,
        QByteArray *turnaroundPngByteArray,
        QString *script,
        std::map<QString, std::map<QString, QString>> *variables) :
    m_filename(filename),
    m_snapshot(snapshot),
    m_object(object),
    m_textures(textures),
    m_turnaroundPngByteArray(turnaroundPngByteArray),
    m_script(script),
    m_variables(variables)
{
}

DocumentSaver::~DocumentSaver()
{
    delete m_snapshot;
    delete m_object;
    delete m_textures;
    delete m_turnaroundPngByteArray;
    delete m_script;
    delete m_variables;
}

void DocumentSaver::process()
{
    save(m_filename,
        m_snapshot,
        m_object,
        m_textures,
        m_turnaroundPngByteArray,
        m_script,
        m_variables);
    emit finished();
}

void DocumentSaver::collectUsedResourceIds(const Snapshot *snapshot,
    std::set<QUuid> &imageIds,
    std::set<QUuid> &fileIds)
{
    for (const auto &material: snapshot->materials) {
        for (auto &layer: material.second) {
            for (auto &mapItem: layer.second) {
                auto findImageIdString = mapItem.find("linkData");
                if (findImageIdString == mapItem.end())
                    continue;
                QUuid imageId = QUuid(findImageIdString->second);
                imageIds.insert(imageId);
            }
        }
    }
    
    for (const auto &part: snapshot->parts) {
        auto findImageIdString = part.second.find("deformMapImageId");
        if (findImageIdString == part.second.end())
            continue;
        QUuid imageId = QUuid(findImageIdString->second);
        imageIds.insert(imageId);
    }
    
    for (const auto &part: snapshot->parts) {
        auto fillMeshLinkedIdString = part.second.find("fillMesh");
        if (fillMeshLinkedIdString == part.second.end())
            continue;
        QUuid fileId = QUuid(fillMeshLinkedIdString->second);
        if (fileId.isNull())
            continue;
        fileIds.insert(fileId);
        const QByteArray *byteArray = FileForever::getContent(fileId);
        if (nullptr == byteArray)
            continue;
        QXmlStreamReader stream(*byteArray);
        Snapshot fileSnapshot;
        loadSkeletonFromXmlStream(&fileSnapshot, stream, SNAPSHOT_ITEM_CANVAS | SNAPSHOT_ITEM_COMPONENT);
        collectUsedResourceIds(&fileSnapshot, imageIds, fileIds);
    }
}

bool DocumentSaver::save(const QString *filename, 
        Snapshot *snapshot,
        const Object *object,
        Textures *textures,
        const QByteArray *turnaroundPngByteArray,
        const QString *script,
        const std::map<QString, std::map<QString, QString>> *variables)
{
    Ds3FileWriter ds3Writer;
    
    {
        QByteArray modelXml;
        QXmlStreamWriter stream(&modelXml);
        saveSkeletonToXmlStream(snapshot, &stream);
        if (modelXml.size() > 0)
            ds3Writer.add("model.xml", "model", &modelXml);
    }
    
    if (nullptr != object) {
        QByteArray objectXml;
        QXmlStreamWriter stream(&objectXml);
        saveObjectToXmlStream(object, &stream);
        if (objectXml.size() > 0)
            ds3Writer.add("object.xml", "object", &objectXml);
    }
    
    if (nullptr != object && nullptr != textures) {
        if (nullptr != textures->textureImage && !textures->textureImage->isNull()) {
            if (nullptr == textures->textureImageByteArray) {
                textures->textureImageByteArray = new QByteArray;
                QBuffer pngBuffer(textures->textureImageByteArray);
                pngBuffer.open(QIODevice::WriteOnly);
                textures->textureImage->save(&pngBuffer, "PNG");
            }
            if (textures->textureImageByteArray->size() > 0)
                ds3Writer.add("object_color.png", "asset", textures->textureImageByteArray);
        }
        if (nullptr != textures->textureNormalImage && !textures->textureNormalImage->isNull()) {
            if (nullptr == textures->textureNormalImageByteArray) {
                textures->textureNormalImageByteArray = new QByteArray;
                QBuffer pngBuffer(textures->textureNormalImageByteArray);
                pngBuffer.open(QIODevice::WriteOnly);
                textures->textureNormalImage->save(&pngBuffer, "PNG");
            }
            if (textures->textureNormalImageByteArray->size() > 0)
                ds3Writer.add("object_normal.png", "asset", textures->textureNormalImageByteArray);
        }
        if (nullptr != textures->textureMetalnessImage && !textures->textureMetalnessImage->isNull()) {
            if (nullptr == textures->textureMetalnessImageByteArray) {
                textures->textureMetalnessImageByteArray = new QByteArray;
                QBuffer pngBuffer(textures->textureMetalnessImageByteArray);
                pngBuffer.open(QIODevice::WriteOnly);
                textures->textureMetalnessImage->save(&pngBuffer, "PNG");
            }
            if (textures->textureMetalnessImageByteArray->size() > 0)
                ds3Writer.add("object_metallic.png", "asset", textures->textureMetalnessImageByteArray);
        }
        if (nullptr != textures->textureRoughnessImage && !textures->textureRoughnessImage->isNull()) {
            if (nullptr == textures->textureRoughnessImageByteArray) {
                textures->textureRoughnessImageByteArray = new QByteArray;
                QBuffer pngBuffer(textures->textureRoughnessImageByteArray);
                pngBuffer.open(QIODevice::WriteOnly);
                textures->textureRoughnessImage->save(&pngBuffer, "PNG");
            }
            if (textures->textureRoughnessImageByteArray->size() > 0)
                ds3Writer.add("object_roughness.png", "asset", textures->textureRoughnessImageByteArray);
        }
        if (nullptr != textures->textureAmbientOcclusionImage && !textures->textureAmbientOcclusionImage->isNull()) {
            if (nullptr == textures->textureAmbientOcclusionImageByteArray) {
                textures->textureAmbientOcclusionImageByteArray = new QByteArray;
                QBuffer pngBuffer(textures->textureAmbientOcclusionImageByteArray);
                pngBuffer.open(QIODevice::WriteOnly);
                textures->textureAmbientOcclusionImage->save(&pngBuffer, "PNG");
            }
            if (textures->textureAmbientOcclusionImageByteArray->size() > 0)
                ds3Writer.add("object_ao.png", "asset", textures->textureAmbientOcclusionImageByteArray);
        }
    }
    
    if (nullptr != turnaroundPngByteArray && turnaroundPngByteArray->size() > 0)
        ds3Writer.add("canvas.png", "asset", turnaroundPngByteArray);
    
    if (nullptr != script && !script->isEmpty()) {
        auto scriptByteArray = script->toUtf8();
        ds3Writer.add("model.js", "script", &scriptByteArray);
    }
    
    if (nullptr != variables && !variables->empty()) {
        QByteArray variablesXml;
        QXmlStreamWriter variablesXmlStream(&variablesXml);
        saveVariablesToXmlStream(*variables, &variablesXmlStream);
        if (variablesXml.size() > 0)
            ds3Writer.add("variables.xml", "variable", &variablesXml);
    }
    
    std::set<QUuid> imageIds;
    std::set<QUuid> fileIds;
    collectUsedResourceIds(snapshot, imageIds, fileIds);

    for (const auto &imageId: imageIds) {
        const QByteArray *pngByteArray = ImageForever::getPngByteArray(imageId);
        if (nullptr == pngByteArray)
            continue;
        if (pngByteArray->size() > 0)
            ds3Writer.add("images/" + imageId.toString() + ".png", "asset", pngByteArray);
    }
    
    for (const auto &fileId: fileIds) {
        const QByteArray *byteArray = FileForever::getContent(fileId);
        if (nullptr == byteArray)
            continue;
        QString suffix = ".bin";
        const QString *name = FileForever::getName(fileId);
        if (nullptr != name) {
            int suffixBegin = name->lastIndexOf(".");
            if (-1 != suffixBegin)
                suffix = name->mid(suffixBegin);
        }
        if (byteArray->size() > 0)
            ds3Writer.add("files/" + fileId.toString() + suffix, "asset", byteArray);
    }
    
    return ds3Writer.save(*filename);
}
