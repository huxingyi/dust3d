#ifndef DUST3D_CENTRIPETAL_CATMULL_ROM_SPLINE_H
#define DUST3D_CENTRIPETAL_CATMULL_ROM_SPLINE_H
#include <vector>
#include <QVector3D>

class CentripetalCatmullRomSpline
{
public:
    struct SplineNode
    {
        int source = -1;
        QVector3D position;
    };
    
    CentripetalCatmullRomSpline(bool isClosed);
    void addPoint(int source, const QVector3D &position, bool isKnot);
    bool interpolate();
    const std::vector<SplineNode> &splineNodes();
private:
    std::vector<SplineNode> m_splineNodes;
    std::vector<size_t> m_splineKnots;
    
    bool m_isClosed = false;
    bool interpolateClosed();
    bool interpolateOpened();
    float atKnot(float t, const QVector3D &p0, const QVector3D &p1);
    void interpolateSegment(std::vector<QVector3D> &knots,
        size_t from, size_t to);
};

#endif
