#ifndef DUST3D_VERTEX_COLOR_PAINTER_H
#define DUST3D_VERTEX_COLOR_PAINTER_H
#include <QObject>
#include <QVector3D>
#include <vector>
#include <map>
#include <set>
#include <QColor>
#include "outcome.h"
#include "paintmode.h"
#include "voxelgrid.h"
#include "model.h"

class PaintColor : public QColor
{
public:
    float metalness = Model::m_defaultMetalness;
    float roughness = Model::m_defaultRoughness;
    
    PaintColor() :
        QColor()
    {
    }
    
    PaintColor(int r, int g, int b, int a = 255) :
        QColor(r, g, b, a)
    {
    }
    
    PaintColor(const QColor &color) :
        QColor(color)
    {
    }
};

PaintColor operator+(const PaintColor &first, const PaintColor &second);
PaintColor operator-(const PaintColor &first, const PaintColor &second);

class VertexColorPainter : public QObject
{
    Q_OBJECT
public:
    VertexColorPainter(const Outcome &outcome, const QVector3D &mouseRayNear, const QVector3D &mouseRayFar);
    void setRadius(float radius);
    void setBrushColor(const QColor &color);
    void setBrushMetalness(float value);
    void setBrushRoughness(float value);
    void setPaintMode(PaintMode paintMode);
    void setMaskNodeIds(const std::set<QUuid> &nodeIds);
    void setVoxelGrid(VoxelGrid<PaintColor> *voxelGrid);
    
    ~VertexColorPainter();
    Model *takePaintedModel();
    const QVector3D &targetPosition();
signals:
    void finished();
public slots:
    void process();
    void paint();
private:
    float m_radius = 0.0;
    PaintMode m_paintMode = PaintMode::None;
    std::set<QUuid> m_mousePickMaskNodeIds;
    Outcome m_outcome;
    QVector3D m_mouseRayNear;
    QVector3D m_mouseRayFar;
    QVector3D m_targetPosition;
    QColor m_brushColor;
    float m_brushMetalness = Model::m_defaultMetalness;
    float m_brushRoughness = Model::m_defaultRoughness;
    VoxelGrid<PaintColor> *m_voxelGrid = nullptr;
    Model *m_model = nullptr;
    bool calculateMouseModelPosition(QVector3D &mouseModelPosition);
    void paintToVoxelGrid();
    int toVoxelLength(float length);
    void createPaintedModel();
public:
    static const int m_gridSize;
};

#endif
