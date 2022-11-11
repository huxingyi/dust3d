#ifndef DUST3D_APPLICATION_SKELETON_NODE_ITEM_H_
#define DUST3D_APPLICATION_SKELETON_NODE_ITEM_H_

#include "document.h"
#include <QGraphicsEllipseItem>

class SkeletonGraphicsNodeItem : public QGraphicsEllipseItem {
public:
    SkeletonGraphicsNodeItem(Document::Profile profile = Document::Profile::Unknown);
    void setRotated(bool rotated);
    void updateAppearance();
    void setOrigin(QPointF point);
    QPointF origin();
    float radius();
    void setRadius(float radius);
    void setMarkColor(QColor color);
    Document::Profile profile();
    dust3d::Uuid id();
    void setId(dust3d::Uuid id);
    void setHovered(bool hovered);
    void setChecked(bool checked);
    void setDeactivated(bool deactivated);
    bool deactivated();
    bool checked();
    bool hovered();

private:
    dust3d::Uuid m_uuid;
    Document::Profile m_profile = Document::Profile::Unknown;
    bool m_hovered = false;
    bool m_checked = false;
    QColor m_markColor;
    bool m_deactivated = false;
    bool m_rotated = false;
};

#endif
