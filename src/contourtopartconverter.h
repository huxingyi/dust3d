#ifndef DUST3D_CONTOUR_TO_PART_CONVERTER_H
#define DUST3D_CONTOUR_TO_PART_CONVERTER_H
#include <QObject>
#include <QPolygonF>
#include <QVector3D>
#include <QVector2D>
#include <set>
#include "snapshot.h"

class ContourToPartConverter : public QObject
{
    Q_OBJECT
public:
    ContourToPartConverter(const QPolygonF &mainProfile, const QPolygonF &sideProfile, const QSizeF &canvasSize);
    const Snapshot &getSnapshot();
signals:
    void finished();
public slots:
    void process();
private:
    QPolygonF m_mainProfile;
    QPolygonF m_sideProfile;
    QSizeF m_canvasSize;
    Snapshot m_snapshot;
    static const float m_targetImageHeight;
    static const float m_minEdgeLength;
    static const float m_radiusEpsilon;
    std::vector<std::pair<QVector3D, float>> m_nodes;
    void convert();
    void extractSkeleton(const QPolygonF &polygon,
        std::vector<std::pair<QVector2D, float>> *skeleton);
    int calculateNodeRadius(const QVector3D &node,
        const QVector3D &direction,
        const std::set<std::pair<int, int>> &black);
    void nodesToSnapshot();
    void smoothRadius(std::vector<std::pair<QVector2D, float>> *skeleton);
    void optimizeNodes();
};

#endif
