#ifndef DUST3D_APPLICATION_GLB_FOREVER_H_
#define DUST3D_APPLICATION_GLB_FOREVER_H_

#include <QByteArray>
#include <dust3d/base/uuid.h>

class GlbForever {
public:
    static const QByteArray* get(const dust3d::Uuid& id);
    static dust3d::Uuid add(const QByteArray* data, dust3d::Uuid toId = dust3d::Uuid());
    static void remove(const dust3d::Uuid& id);
};

#endif
