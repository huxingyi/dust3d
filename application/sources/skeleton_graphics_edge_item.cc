#include "skeleton_graphics_edge_item.h"
#include "skeleton_graphics_node_item.h"
#include "theme.h"

SkeletonGraphicsEdgeItem::SkeletonGraphicsEdgeItem()
    : m_firstItem(nullptr)
    , m_secondItem(nullptr)
    , m_hovered(false)
    , m_checked(false)
    , m_profile(Document::Profile::Unknown)
    , m_deactivated(false)
    , m_rotated(false)
{
    setData(0, "edge");
}

void SkeletonGraphicsEdgeItem::setRotated(bool rotated)
{
    m_rotated = rotated;
}

void SkeletonGraphicsEdgeItem::setEndpoints(SkeletonGraphicsNodeItem* first, SkeletonGraphicsNodeItem* second)
{
    m_firstItem = first;
    m_secondItem = second;
    updateAppearance();
}

SkeletonGraphicsNodeItem* SkeletonGraphicsEdgeItem::firstItem()
{
    return m_firstItem;
}

SkeletonGraphicsNodeItem* SkeletonGraphicsEdgeItem::secondItem()
{
    return m_secondItem;
}

void SkeletonGraphicsEdgeItem::updateAppearance()
{
    if (nullptr == m_firstItem || nullptr == m_secondItem)
        return;

    m_profile = m_firstItem->profile();

    QLineF line(m_firstItem->origin(), m_secondItem->origin());

    QPolygonF polygon;
    float radAngle = line.angle() * M_PI / 180;
    float dx = 2 * sin(radAngle);
    float dy = 2 * cos(radAngle);
    QPointF offset1 = QPointF(dx, dy);
    QPointF offset2 = QPointF(-dx, -dy);
    //polygon << line.p1() + offset1 << line.p1() + offset2 << line.p2() + offset2 << line.p2() + offset1;
    polygon << line.p1() + offset1 << line.p1() + offset2 << line.p2();
    setPolygon(polygon);

    QColor color = Qt::gray;

    if (!m_deactivated) {
        switch (m_firstItem->profile()) {
        case Document::Profile::Unknown:
            break;
        case Document::Profile::Main:
            color = m_rotated ? Theme::blue : Theme::red;
            break;
        case Document::Profile::Side:
            color = Theme::green;
            break;
        }
    }

    QColor penColor = color;
    penColor.setAlphaF((m_checked || m_hovered) ? Theme::checkedAlpha : Theme::normalAlpha);
    QPen pen(penColor);
    pen.setWidth(0);
    setPen(pen);
}

dust3d::Uuid SkeletonGraphicsEdgeItem::id()
{
    return m_uuid;
}

void SkeletonGraphicsEdgeItem::setId(dust3d::Uuid id)
{
    m_uuid = id;
}

Document::Profile SkeletonGraphicsEdgeItem::profile()
{
    return m_profile;
}

void SkeletonGraphicsEdgeItem::setHovered(bool hovered)
{
    m_hovered = hovered;
    updateAppearance();
}

void SkeletonGraphicsEdgeItem::setChecked(bool checked)
{
    m_checked = checked;
    updateAppearance();
}

void SkeletonGraphicsEdgeItem::setDeactivated(bool deactivated)
{
    m_deactivated = deactivated;
    updateAppearance();
}

void SkeletonGraphicsEdgeItem::reverse()
{
    std::swap(m_firstItem, m_secondItem);
    updateAppearance();
}

bool SkeletonGraphicsEdgeItem::deactivated()
{
    return m_deactivated;
}

bool SkeletonGraphicsEdgeItem::checked()
{
    return m_checked;
}

bool SkeletonGraphicsEdgeItem::hovered()
{
    return m_hovered;
}