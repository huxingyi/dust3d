#ifndef CURVE_UTIL_H
#define CURVE_UTIL_H
#include <QPointF>
#include <vector>
#include <QPainterPath>
#include <QVector2D>

class HermiteControlNode
{
public:
    QVector2D position;
    QVector2D inTangent;
    QVector2D outTangent;
    
    HermiteControlNode(const QVector2D &p, const QVector2D &tin, const QVector2D &tout)
    {
        position = p;
        inTangent = tin;
        outTangent = tout;
    }
};

class BezierControlNode
{
public:
    QVector2D endpoint;
    QVector2D handles[2];
    
    BezierControlNode(const HermiteControlNode &beginHermite,
        const HermiteControlNode &endHermite)
    {
        endpoint = endHermite.position;
        handles[0] = beginHermite.position + beginHermite.outTangent / 3;
        handles[1] = endHermite.position - endHermite.inTangent / 3;
    }
};

void hermiteCurveToPainterPath(const std::vector<HermiteControlNode> &hermiteControlNodes,
    QPainterPath &toPath);
QVector2D calculateHermiteInterpolation(const std::vector<HermiteControlNode> &hermiteControlNodes, 
    float knot);

#endif
