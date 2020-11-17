#include <QDebug>
#include "triangletangentresolve.h"

void triangleTangentResolve(const Object &object, std::vector<QVector3D> &tangents)
{
    tangents.resize(object.triangles.size());
    
    if (nullptr == object.triangleVertexUvs())
        return;
    
    const std::vector<std::vector<QVector2D>> &triangleVertexUvs = *object.triangleVertexUvs();
    
    for (decltype(object.triangles.size()) i = 0; i < object.triangles.size(); i++) {
        tangents[i] = {0, 0, 0};
        const auto &uv = triangleVertexUvs[i];
        QVector2D uv1 = {uv[0][0], uv[0][1]};
        QVector2D uv2 = {uv[1][0], uv[1][1]};
        QVector2D uv3 = {uv[2][0], uv[2][1]};
        const auto &triangle = object.triangles[i];
        const QVector3D &pos1 = object.vertices[triangle[0]];
        const QVector3D &pos2 = object.vertices[triangle[1]];
        const QVector3D &pos3 = object.vertices[triangle[2]];
        QVector3D edge1 = pos2 - pos1;
        QVector3D edge2 = pos3 - pos1;
        QVector2D deltaUv1 = uv2 - uv1;
        QVector2D deltaUv2 = uv3 - uv1;
        auto bottom = deltaUv1.x() * deltaUv2.y() - deltaUv2.x() * deltaUv1.y();
        if (qFuzzyIsNull(bottom)) {
            bottom = 0.000001;
        }
        float f = 1.0 / bottom;
        QVector3D tangent = {
            f * (deltaUv2.y() * edge1.x() - deltaUv1.y() * edge2.x()),
            f * (deltaUv2.y() * edge1.y() - deltaUv1.y() * edge2.y()),
            f * (deltaUv2.y() * edge1.z() - deltaUv1.y() * edge2.z())
        };
        tangents[i] = tangent.normalized();
    }
}
