#ifndef DUST3D_APPLICATION_IMAGE_FOREVER_H_
#define DUST3D_APPLICATION_IMAGE_FOREVER_H_

#include <QImage>
#include <QByteArray>
#include <dust3d/base/uuid.h>

class ImageForever
{
public:
    static const QImage *get(const dust3d::Uuid &id);
    static void copy(const dust3d::Uuid &id, QImage &image);
    static const QByteArray *getPngByteArray(const dust3d::Uuid &id);
    static dust3d::Uuid add(const QImage *image, dust3d::Uuid toId=dust3d::Uuid());
    static void remove(const dust3d::Uuid &id);
};

#endif
