#ifndef DUST3D_APPLICATION_SKELETON_SELECTION_ITEM_H_
#define DUST3D_APPLICATION_SKELETON_SELECTION_ITEM_H_

#include "document.h"
#include <QGraphicsRectItem>

class SkeletonGraphicsSelectionItem : public QGraphicsRectItem {
public:
    SkeletonGraphicsSelectionItem();
    void updateRange(QPointF beginPos, QPointF endPos);
};

#endif
