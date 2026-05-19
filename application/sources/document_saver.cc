#include "document_saver.h"
#include "glb_forever.h"
#include "image_forever.h"
#include <QGuiApplication>
#include <QtCore/qbuffer.h>
#include <dust3d/base/ds3_file.h>
#include <dust3d/base/snapshot_xml.h>
#include <set>

DocumentSaver::DocumentSaver(const QString* filename,
    dust3d::Snapshot* snapshot,
    QByteArray* turnaroundPngByteArray)
    : m_filename(filename)
    , m_snapshot(snapshot)
    , m_turnaroundPngByteArray(turnaroundPngByteArray)
{
}

DocumentSaver::~DocumentSaver()
{
    delete m_snapshot;
    delete m_turnaroundPngByteArray;
}

void DocumentSaver::process()
{
    save(m_filename,
        m_snapshot,
        m_turnaroundPngByteArray);
    emit finished();
}

void DocumentSaver::collectUsedResourceIds(const dust3d::Snapshot* snapshot,
    std::set<dust3d::Uuid>& imageIds,
    std::set<dust3d::Uuid>& glbIds)
{
    for (const auto& componentIt : snapshot->components) {
        auto findImageIdString = componentIt.second.find("colorImageId");
        if (findImageIdString == componentIt.second.end())
            continue;
        dust3d::Uuid imageId = dust3d::Uuid(findImageIdString->second);
        imageIds.insert(imageId);
    }
    for (const auto& partIt : snapshot->parts) {
        auto findGlbIdString = partIt.second.find("importedModelId");
        if (findGlbIdString == partIt.second.end())
            continue;
        dust3d::Uuid glbId = dust3d::Uuid(findGlbIdString->second);
        if (!glbId.isNull())
            glbIds.insert(glbId);
    }
}

bool DocumentSaver::save(dust3d::Ds3FileWriter& ds3Writer,
    dust3d::Snapshot* snapshot,
    const QByteArray* turnaroundPngByteArray)
{
    {
        std::string modelXml;
        saveSnapshotToXmlString(*snapshot, modelXml);
        if (modelXml.size() > 0) {
            ds3Writer.add("model.xml", "model", modelXml.c_str(), modelXml.size());
        }
    }

    if (nullptr != turnaroundPngByteArray && turnaroundPngByteArray->size() > 0)
        ds3Writer.add("canvas.png", "asset", turnaroundPngByteArray->data(), turnaroundPngByteArray->size());

    std::set<dust3d::Uuid> imageIds;
    std::set<dust3d::Uuid> glbIds;
    collectUsedResourceIds(snapshot, imageIds, glbIds);

    for (const auto& imageId : imageIds) {
        const QByteArray* pngByteArray = ImageForever::getPngByteArray(imageId);
        if (nullptr == pngByteArray)
            continue;
        if (pngByteArray->size() > 0)
            ds3Writer.add("images/" + imageId.toString() + ".png", "asset", pngByteArray->data(), pngByteArray->size());
    }

    for (const auto& glbId : glbIds) {
        const QByteArray* glbData = GlbForever::get(glbId);
        if (nullptr == glbData)
            continue;
        if (glbData->size() > 0)
            ds3Writer.add("models/" + glbId.toString() + ".glb", "asset", glbData->data(), glbData->size());
    }

    return true;
}

bool DocumentSaver::save(const QString* filename,
    dust3d::Snapshot* snapshot,
    const QByteArray* turnaroundPngByteArray)
{
    dust3d::Ds3FileWriter ds3Writer;

    save(ds3Writer, snapshot, turnaroundPngByteArray);

    return ds3Writer.save(filename->toUtf8().constData());
}

bool DocumentSaver::save(QByteArray& byteArray,
    dust3d::Snapshot* snapshot,
    const QByteArray* turnaroundPngByteArray)
{
    dust3d::Ds3FileWriter ds3Writer;

    save(ds3Writer, snapshot, turnaroundPngByteArray);

    std::vector<uint8_t> rawArray;
    ds3Writer.save(rawArray);

    byteArray.append((char*)rawArray.data(), rawArray.size());

    return true;
}
