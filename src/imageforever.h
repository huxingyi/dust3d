#ifndef DUST3D_IMAGE_FOREVER_H
#define DUST3D_IMAGE_FOREVER_H
#include <QImage>
#include <QUuid>

class ImageForever
{
public:
    static const QImage *get(const QUuid &id);
    static QUuid add(const QImage *image, QUuid toId=QUuid());
};

#endif
