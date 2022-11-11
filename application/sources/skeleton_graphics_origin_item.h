#ifndef DUST3D_APPLICATION_SKELETON_ORIGIN_ITEM_H_
#define DUST3D_APPLICATION_SKELETON_ORIGIN_ITEM_H_

#include "document.h"
#include <QGraphicsPolygonItem>

class SkeletonGraphicsOriginItem : public QGraphicsPolygonItem {
public:
    SkeletonGraphicsOriginItem(Document::Profile profile = Document::Profile::Unknown);
    void setRotated(bool rotated);
    void updateAppearance();
    void setOrigin(QPointF point);
    QPointF origin();
    Document::Profile profile();
    void setHovered(bool hovered);
    void setChecked(bool checked);
    bool checked();
    bool hovered();

private:
    Document::Profile m_profile = Document::Profile::Unknown;
    bool m_hovered = false;
    bool m_checked = false;
    QPointF m_origin;
    bool m_rotated = false;
};

#endif
