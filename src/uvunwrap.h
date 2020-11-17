#ifndef DUST3D_UV_UNWRAP_H
#define DUST3D_UV_UNWRAP_H
#include <set>
#include <QVector2D>
#include "object.h"

void uvUnwrap(const Object &object,
    std::vector<std::vector<QVector2D>> &triangleVertexUvs,
    std::set<int> &seamVertices,
    std::map<QUuid, std::vector<QRectF>> &uvRects);

#endif
