#ifndef DUST3D_APPLICATION_SKELETON_GRAPHICS_EDGE_ITEM_H_
#define DUST3D_APPLICATION_SKELETON_GRAPHICS_EDGE_ITEM_H_

#include "document.h"
#include <QGraphicsPolygonItem>

class SkeletonGraphicsNodeItem;

class SkeletonGraphicsEdgeItem : public QGraphicsPolygonItem {
public:
    SkeletonGraphicsEdgeItem();
    void setRotated(bool rotated);
    void setEndpoints(SkeletonGraphicsNodeItem* first, SkeletonGraphicsNodeItem* second);
    SkeletonGraphicsNodeItem* firstItem();
    SkeletonGraphicsNodeItem* secondItem();
    void updateAppearance();
    dust3d::Uuid id();
    void setId(dust3d::Uuid id);
    Document::Profile profile();
    void setHovered(bool hovered);
    void setChecked(bool checked);
    void setDeactivated(bool deactivated);
    void reverse();
    bool deactivated();
    bool checked();
    bool hovered();

private:
    dust3d::Uuid m_uuid;
    SkeletonGraphicsNodeItem* m_firstItem;
    SkeletonGraphicsNodeItem* m_secondItem;
    QPolygonF m_selectionPolygon;
    bool m_hovered = false;
    bool m_checked = false;
    Document::Profile m_profile = Document::Profile::Unknown;
    bool m_deactivated = false;
    bool m_rotated = false;
};

#endif
