#include "curveutil.h"

void hermiteCurveToPainterPath(const std::vector<HermiteControlNode> &hermiteControlNodes,
    QPainterPath &toPath)
{
    if (!hermiteControlNodes.empty()) {
        QPainterPath path(QPointF(hermiteControlNodes[0].position.x(),
            hermiteControlNodes[0].position.y()));
        for (size_t i = 1; i < hermiteControlNodes.size(); i++) {
            const auto &beginHermite = hermiteControlNodes[i - 1];
            const auto &endHermite = hermiteControlNodes[i];
            BezierControlNode bezier(beginHermite, endHermite);
            path.cubicTo(QPointF(bezier.handles[0].x(), bezier.handles[0].y()),
                QPointF(bezier.handles[1].x(), bezier.handles[1].y()),
                QPointF(bezier.endpoint.x(), bezier.endpoint.y()));
        }
        toPath = path;
    }
}

QVector2D calculateHermiteInterpolation(const std::vector<HermiteControlNode> &hermiteControlNodes, 
    float knot)
{
    if (hermiteControlNodes.size() < 2)
        return QVector2D();
    
    int startControlIndex = 0;
    int stopControlIndex = hermiteControlNodes.size() - 1;
    for (size_t i = 0; i < hermiteControlNodes.size(); i++) {
        if (hermiteControlNodes[i].position.x() > knot)
            break;
        startControlIndex = i;
    }
    for (int i = (int)hermiteControlNodes.size() - 1; i >= 0; i--) {
        if (hermiteControlNodes[i].position.x() < knot)
            break;
        stopControlIndex = i;
    }
    if (startControlIndex >= stopControlIndex)
        return QVector2D();
    
    const auto &startControlNode = hermiteControlNodes[startControlIndex];
    const auto &stopControlNode = hermiteControlNodes[stopControlIndex];
    
    if (startControlNode.position.x() >= stopControlNode.position.x())
        return startControlNode.position;
    
    float length = (float)(stopControlNode.position.x() - startControlNode.position.x());
    float t = (knot - startControlNode.position.x()) / length;
    float t2 = t * t;
    float t3 = t2 * t;
    float h1 = 2 * t3 - 3 * t2 + 1;
    float h2 = -2 * t3 + 3 * t2;
    float h3 = t3 - 2 * t2 + t;
    float h4 = t3 -  t2;
    
    QVector2D interpolatedPosition = h1 * startControlNode.position +
        h2 * stopControlNode.position +
        h3 * startControlNode.outTangent +
        h4 * stopControlNode.inTangent;

    return interpolatedPosition;
}
