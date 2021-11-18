#include <map>
#include <QMutex>
#include <QMutexLocker>
#include <QtCore/qbuffer.h>
#include "image_forever.h"

struct ImageForeverItem
{
    QImage *image;
    dust3d::Uuid id;
    QByteArray *imageByteArray;
};
static std::map<dust3d::Uuid, ImageForeverItem> g_foreverMap;
static QMutex g_mapMutex;

const QImage *ImageForever::get(const dust3d::Uuid &id)
{
    QMutexLocker locker(&g_mapMutex);
    auto findResult = g_foreverMap.find(id);
    if (findResult == g_foreverMap.end())
        return nullptr;
    return findResult->second.image;
}

void ImageForever::copy(const dust3d::Uuid &id, QImage &image)
{
    QMutexLocker locker(&g_mapMutex);
    auto findResult = g_foreverMap.find(id);
    if (findResult == g_foreverMap.end())
        return;
    image = *findResult->second.image;
}

const QByteArray *ImageForever::getPngByteArray(const dust3d::Uuid &id)
{
    QMutexLocker locker(&g_mapMutex);
    auto findResult = g_foreverMap.find(id);
    if (findResult == g_foreverMap.end())
        return nullptr;
    return findResult->second.imageByteArray;
}

dust3d::Uuid ImageForever::add(const QImage *image, dust3d::Uuid toId)
{
    QMutexLocker locker(&g_mapMutex);
    if (nullptr == image)
        return dust3d::Uuid();
    dust3d::Uuid newId = toId.isNull() ? dust3d::Uuid::createUuid() : toId;
    if (g_foreverMap.find(newId) != g_foreverMap.end())
        return newId;
    QImage *newImage = new QImage(*image);
    QByteArray *imageByteArray = new QByteArray();
    QBuffer pngBuffer(imageByteArray);
    pngBuffer.open(QIODevice::WriteOnly);
    newImage->save(&pngBuffer, "PNG");
    g_foreverMap[newId] = {newImage, newId, imageByteArray};
    return newId;
}

void ImageForever::remove(const dust3d::Uuid &id)
{
    QMutexLocker locker(&g_mapMutex);
    auto findImage = g_foreverMap.find(id);
    if (findImage == g_foreverMap.end())
        return;
    delete findImage->second.image;
    delete findImage->second.imageByteArray;
    g_foreverMap.erase(id);
}
