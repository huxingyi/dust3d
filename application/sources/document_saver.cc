#include "document_saver.h"
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
    std::set<dust3d::Uuid>& imageIds)
{
    for (const auto& componentIt : snapshot->components) {
        auto findImageIdString = componentIt.second.find("colorImageId");
        if (findImageIdString == componentIt.second.end())
            continue;
        dust3d::Uuid imageId = dust3d::Uuid(findImageIdString->second);
        imageIds.insert(imageId);
    }
}

bool DocumentSaver::save(const QString* filename,
    dust3d::Snapshot* snapshot,
    const QByteArray* turnaroundPngByteArray)
{
    dust3d::Ds3FileWriter ds3Writer;

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
    collectUsedResourceIds(snapshot, imageIds);

    for (const auto& imageId : imageIds) {
        const QByteArray* pngByteArray = ImageForever::getPngByteArray(imageId);
        if (nullptr == pngByteArray)
            continue;
        if (pngByteArray->size() > 0)
            ds3Writer.add("images/" + imageId.toString() + ".png", "asset", pngByteArray->data(), pngByteArray->size());
    }

    return ds3Writer.save(filename->toUtf8().constData());
}