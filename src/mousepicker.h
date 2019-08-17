#ifndef DUST3D_MOUSE_PICKER_H
#define DUST3D_MOUSE_PICKER_H
#include <QObject>
#include <QVector3D>
#include <vector>
#include <map>
#include <set>
#include "outcome.h"
#include "paintmode.h"

class MousePicker : public QObject
{
    Q_OBJECT
public:
    MousePicker(const Outcome &outcome, const QVector3D &mouseRayNear, const QVector3D &mouseRayFar);
    void setRadius(float radius);
    void setPaintImages(const std::map<QUuid, QUuid> &paintImages);
    void setPaintMode(PaintMode paintMode);
    void setMaskNodeIds(const std::set<QUuid> &nodeIds);
    const std::map<QUuid, QUuid> &resultPaintImages();
    const std::set<QUuid> &changedPartIds();
    
    ~MousePicker();
    const QVector3D &targetPosition();
signals:
    void finished();
public slots:
    void process();
    void pick();
private:
    float m_radius = 0.0;
    std::map<QUuid, QUuid> m_paintImages;
    PaintMode m_paintMode = PaintMode::None;
    std::set<QUuid> m_changedPartIds;
    std::set<QUuid> m_mousePickMaskNodeIds;
    bool m_enablePaint = false;
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
    bool calculateMouseModelPosition(QVector3D &mouseModelPosition);
    void paintToImage(const QUuid &partId, float x, float y, float radius, bool inverted=false);
};

#endif
