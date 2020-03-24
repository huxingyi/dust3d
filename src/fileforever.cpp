#include <map>
#include <QMutex>
#include <QMutexLocker>
#include <QtCore/qbuffer.h>
#include <QFile>
#include "fileforever.h"

struct FileForeverItem
{
    QString name;
    QUuid id;
    QByteArray *byteArray;
};
static std::map<QUuid, FileForeverItem> g_foreverMap;
static QMutex g_mapMutex;

const QString *FileForever::getName(const QUuid &id)
{
    QMutexLocker locker(&g_mapMutex);
    auto findResult = g_foreverMap.find(id);
    if (findResult == g_foreverMap.end())
        return nullptr;
    return &findResult->second.name;
}

const QByteArray *FileForever::getContent(const QUuid &id)
{
    QMutexLocker locker(&g_mapMutex);
    auto findResult = g_foreverMap.find(id);
    if (findResult == g_foreverMap.end())
        return nullptr;
    return findResult->second.byteArray;
}

QUuid FileForever::add(const QString &name, const QByteArray &content, QUuid toId)
{
    QMutexLocker locker(&g_mapMutex);
    QUuid newId = toId.isNull() ? QUuid::createUuid() : toId;
    if (g_foreverMap.find(newId) != g_foreverMap.end())
        return newId;
    QByteArray *byteArray = new QByteArray(content);
    g_foreverMap[newId] = {name, newId, byteArray};
    return newId;
}

void FileForever::remove(const QUuid &id)
{
    QMutexLocker locker(&g_mapMutex);
    auto findFile = g_foreverMap.find(id);
    if (findFile == g_foreverMap.end())
        return;
    delete findFile->second.byteArray;
    g_foreverMap.erase(id);
}
