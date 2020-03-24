#ifndef DUST3D_FILE_FOREVER_H
#define DUST3D_FILE_FOREVER_H
#include <QUuid>
#include <QByteArray>
#include <QString>

class FileForever
{
public:
    static const QString *getName(const QUuid &id);
    static const QByteArray *getContent(const QUuid &id);
    static QUuid add(const QString &name, const QByteArray &content, QUuid toId=QUuid());
    static void remove(const QUuid &id);
};

#endif
