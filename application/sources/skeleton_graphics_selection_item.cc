#include "skeleton_graphics_selection_item.h"
#include "theme.h"

SkeletonGraphicsSelectionItem::SkeletonGraphicsSelectionItem()
{
    QColor penColor = Theme::white;
    QPen pen(penColor);
    pen.setWidth(0);
    pen.setStyle(Qt::DashLine);
    setPen(pen);
}

void SkeletonGraphicsSelectionItem::updateRange(QPointF beginPos, QPointF endPos)
{
    setRect(QRectF(beginPos, endPos).normalized());
}