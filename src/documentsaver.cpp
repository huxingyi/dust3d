#include <QXmlStreamWriter>
#include <set>
#include <QGuiApplication>
#include "documentsaver.h"
#include "imageforever.h"
#include "ds3file.h"
#include "snapshotxml.h"
#include "variablesxml.h"
#include "fileforever.h"

DocumentSaver::DocumentSaver(const QString *filename, 
        Snapshot *snapshot, 
        QByteArray *turnaroundPngByteArray,
        QString *script,
        std::map<QString, std::map<QString, QString>> *variables) :
    m_filename(filename),
    m_snapshot(snapshot),
    m_turnaroundPngByteArray(turnaroundPngByteArray),
    m_script(script),
    m_variables(variables)
{
}

DocumentSaver::~DocumentSaver()
{
    delete m_snapshot;
    delete m_turnaroundPngByteArray;
    delete m_script;
    delete m_variables;
}

void DocumentSaver::process()
{
    save(m_filename,
        m_snapshot,
        m_turnaroundPngByteArray,
        m_script,
        m_variables);
    this->moveToThread(QGuiApplication::instance()->thread());
    emit finished();
}

bool DocumentSaver::save(const QString *filename, 
        Snapshot *snapshot, 
        const QByteArray *turnaroundPngByteArray,
        const QString *script,
        const std::map<QString, std::map<QString, QString>> *variables)
{
    Ds3FileWriter ds3Writer;
    
    QByteArray modelXml;
    QXmlStreamWriter stream(&modelXml);
    saveSkeletonToXmlStream(snapshot, &stream);
    if (modelXml.size() > 0)
        ds3Writer.add("model.xml", "model", &modelXml);
    
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
    }
    
    for (auto &pose: snapshot->poses) {
        auto findCanvasImageId = pose.first.find("canvasImageId");
        if (findCanvasImageId != pose.first.end()) {
            QUuid imageId = QUuid(findCanvasImageId->second);
            imageIds.insert(imageId);
        }
    }
    
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