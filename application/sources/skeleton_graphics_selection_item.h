#ifndef DUST3D_APPLICATION_SKELETON_SELECTION_ITEM_H_
#define DUST3D_APPLICATION_SKELETON_SELECTION_ITEM_H_

#include "document.h"
#include <QGraphicsPathItem>
#include <QPainterPath>
#include <QPolygonF>

class SkeletonGraphicsSelectionItem : public QGraphicsPathItem {
public:
    SkeletonGraphicsSelectionItem();
    // Start a new freehand selection at the given scene position.
    void reset(QPointF startPos);
    // Append a point to the freehand selection as the user drags.
    void addPoint(QPointF pos);
    // The closed region (in scene coordinates) used to test which items are selected.
    QPainterPath selectionShape() const;

private:
    QPolygonF m_polygon;
    void updatePath();
};

#endif
