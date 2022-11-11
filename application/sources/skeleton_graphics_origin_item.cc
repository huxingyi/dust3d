#include "skeleton_graphics_origin_item.h"
#include "theme.h"

SkeletonGraphicsOriginItem::SkeletonGraphicsOriginItem(Document::Profile profile)
    : m_profile(profile)
    , m_hovered(false)
    , m_checked(false)
    , m_rotated(false)
{
    setData(0, "origin");
}
void SkeletonGraphicsOriginItem::setRotated(bool rotated)
{
    m_rotated = rotated;
}
void SkeletonGraphicsOriginItem::updateAppearance()
{
    QColor color = Theme::white;

    switch (m_profile) {
    case Document::Profile::Unknown:
        break;
    case Document::Profile::Main:
        color = m_rotated ? Theme::blue : Theme::red;
        break;
    case Document::Profile::Side:
        color = Theme::green;
        break;
    }

    QColor penColor = color;
    penColor.setAlphaF(m_checked ? Theme::checkedAlpha : Theme::normalAlpha);
    QPen pen(penColor);
    pen.setWidth(0);
    setPen(pen);

    QColor brushColor = color;
    brushColor.setAlphaF((m_checked || m_hovered) ? Theme::checkedAlpha : Theme::normalAlpha);
    QBrush brush(brushColor);
    setBrush(brush);
}
void SkeletonGraphicsOriginItem::setOrigin(QPointF point)
{
    m_origin = point;
    QPolygonF triangle;
    const int triangleRadius = 10;
    triangle.append(QPointF(point.x() - triangleRadius, point.y()));
    triangle.append(QPointF(point.x() + triangleRadius, point.y()));
    triangle.append(QPointF(point.x(), point.y() - triangleRadius));
    triangle.append(QPointF(point.x() - triangleRadius, point.y()));
    setPolygon(triangle);
    updateAppearance();
}
QPointF SkeletonGraphicsOriginItem::origin()
{
    return m_origin;
}
Document::Profile SkeletonGraphicsOriginItem::profile()
{
    return m_profile;
}
void SkeletonGraphicsOriginItem::setHovered(bool hovered)
{
    m_hovered = hovered;
    updateAppearance();
}
void SkeletonGraphicsOriginItem::setChecked(bool checked)
{
    m_checked = checked;
    updateAppearance();
}
bool SkeletonGraphicsOriginItem::checked()
{
    return m_checked;
}
bool SkeletonGraphicsOriginItem::hovered()
{
    return m_hovered;
}
