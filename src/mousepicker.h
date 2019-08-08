#ifndef DUST3D_MOUSE_PICKER_H
#define DUST3D_MOUSE_PICKER_H
#include <QObject>
#include <QVector3D>
#include <vector>
#include "outcome.h"

class MousePicker : public QObject
{
    Q_OBJECT
public:
    MousePicker(const Outcome &outcome, const QVector3D &mouseRayNear, const QVector3D &mouseRayFar);
    ~MousePicker();
    const QVector3D &targetPosition();
signals:
    void finished();
public slots:
    void process();
private:
    Outcome m_outcome;
    QVector3D m_mouseRayNear;
    QVector3D m_mouseRayFar;
    QVector3D m_targetPosition;
    static bool intersectSegmentAndPlane(const QVector3D &segmentPoint0, const QVector3D &segmentPoint1,
        const QVector3D &pointOnPlane, const QVector3D &planeNormal,
        QVector3D *intersection=nullptr);
    static bool intersectSegmentAndTriangle(const QVector3D &segmentPoint0, const QVector3D &segmentPoint1,
        const std::vector<QVector3D> &triangle,
        const QVector3D &triangleNormal,
        QVector3D *intersection=nullptr);
};

#endif
