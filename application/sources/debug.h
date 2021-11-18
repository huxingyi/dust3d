#ifndef DUST3D_APPLICATION_DEBUG_H_
#define DUST3D_APPLICATION_DEBUG_H_

#include <QDebug>
#include <dust3d/base/uuid.h>

QDebug operator<<(QDebug debug, const dust3d::Uuid &uuid);
QDebug operator<<(QDebug debug, const std::string &string);

#endif
