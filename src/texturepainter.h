#ifndef DUST3D_TEXTURE_PAINTER_H
#define DUST3D_TEXTURE_PAINTER_H
#include <QObject>
#include <QVector3D>
#include <vector>
#include <map>
#include <set>
#include <QColor>
#include <QImage>
#include <unordered_map>
#include <unordered_set>
#include <deque>
#include "object.h"
#include "paintmode.h"
#include "model.h"

struct TexturePainterStroke
{
    QVector3D mouseRayNear;
    QVector3D mouseRayFar;
};

class TexturePainterContext
{
public:
    Object *object = nullptr;
    QImage *colorImage = nullptr;
    //std::unordered_map<size_t, std::unordered_set<size_t>> *faceAroundVertexMap = nullptr;
    
    ~TexturePainterContext()
    {
        delete object;
        delete colorImage;
    }
};

class TexturePainter : public QObject
{
    Q_OBJECT
public:
    TexturePainter(const QVector3D &mouseRayNear, const QVector3D &mouseRayFar);
    void setContext(TexturePainterContext *context);
    void setRadius(float radius);
    void setBrushColor(const QColor &color);
    void setPaintMode(PaintMode paintMode);
    void setMaskNodeIds(const std::set<QUuid> &nodeIds);
    
    QImage *takeColorImage();
    
    ~TexturePainter();
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
    QVector3D m_mouseRayNear;
    QVector3D m_mouseRayFar;
    QVector3D m_targetPosition;
    QColor m_brushColor;
    TexturePainterContext *m_context = nullptr;
    QImage *m_colorImage = nullptr;
    
    //void buildFaceAroundVertexMap();
    //void collectNearbyTriangles(size_t triangleIndex, std::unordered_set<size_t> *triangleIndices);
    void paintStroke(QPainter &painter, const TexturePainterStroke &stroke);
};

#endif
