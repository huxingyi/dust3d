#include "glb_forever.h"
#include <QMutex>
#include <QMutexLocker>
#include <map>

struct GlbForeverItem {
    QByteArray* data;
    dust3d::Uuid id;
};
static std::map<dust3d::Uuid, GlbForeverItem> g_glbForeverMap;
static QMutex g_glbMapMutex;

const QByteArray* GlbForever::get(const dust3d::Uuid& id)
{
    QMutexLocker locker(&g_glbMapMutex);
    auto findResult = g_glbForeverMap.find(id);
    if (findResult == g_glbForeverMap.end())
        return nullptr;
    return findResult->second.data;
}

dust3d::Uuid GlbForever::add(const QByteArray* data, dust3d::Uuid toId)
{
    QMutexLocker locker(&g_glbMapMutex);
    if (nullptr == data || data->isEmpty())
        return dust3d::Uuid();
    dust3d::Uuid newId = toId.isNull() ? dust3d::Uuid::createUuid() : toId;
    if (g_glbForeverMap.find(newId) != g_glbForeverMap.end())
        return newId;
    QByteArray* newData = new QByteArray(*data);
    g_glbForeverMap[newId] = { newData, newId };
    return newId;
}

void GlbForever::remove(const dust3d::Uuid& id)
{
    QMutexLocker locker(&g_glbMapMutex);
    auto findGlb = g_glbForeverMap.find(id);
    if (findGlb == g_glbForeverMap.end())
        return;
    delete findGlb->second.data;
    g_glbForeverMap.erase(id);
}
