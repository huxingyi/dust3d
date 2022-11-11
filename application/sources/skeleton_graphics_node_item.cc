#include "skeleton_graphics_node_item.h"
#include "theme.h"

SkeletonGraphicsNodeItem::SkeletonGraphicsNodeItem(Document::Profile profile)
    : m_profile(profile)
    , m_hovered(false)
    , m_checked(false)
    , m_markColor(Qt::transparent)
    , m_deactivated(false)
    , m_rotated(false)
{
    setData(0, "node");
    setRadius(32);
}

void SkeletonGraphicsNodeItem::setRotated(bool rotated)
{
    m_rotated = rotated;
}

void SkeletonGraphicsNodeItem::updateAppearance()
{
    QColor color = Qt::gray;

    if (!m_deactivated) {
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
    }

    QColor penColor = color;
    penColor.setAlphaF((m_checked || m_hovered) ? Theme::checkedAlpha : Theme::normalAlpha);
    QPen pen(penColor);
    pen.setWidth(0);
    setPen(pen);

    QColor brushColor;
    Qt::BrushStyle style;
    if (m_markColor == Qt::transparent) {
        brushColor = color;
        brushColor.setAlphaF((m_checked || m_hovered) ? Theme::fillAlpha : 0);
        style = Qt::SolidPattern;
    } else {
        brushColor = m_markColor;
        brushColor.setAlphaF((m_checked || m_hovered) ? Theme::fillAlpha * 4 : Theme::fillAlpha * 1.5);
        style = Qt::Dense4Pattern;
    }
    if (m_checked)
        brushColor.setAlphaF(brushColor.alphaF() * 1.2);
    QBrush brush(brushColor);
    brush.setStyle(style);
    setBrush(brush);
}

void SkeletonGraphicsNodeItem::setOrigin(QPointF point)
{
    QPointF moveBy = point - origin();
    QRectF newRect = rect();
    newRect.adjust(moveBy.x(), moveBy.y(), moveBy.x(), moveBy.y());
    setRect(newRect);
    updateAppearance();
}

QPointF SkeletonGraphicsNodeItem::origin()
{
    return QPointF(rect().x() + rect().width() / 2,
        rect().y() + rect().height() / 2);
}

float SkeletonGraphicsNodeItem::radius()
{
    return rect().width() / 2;
}

void SkeletonGraphicsNodeItem::setRadius(float radius)
{
    if (radius < 1)
        radius = 1;
    QPointF oldOrigin = origin();
    setRect(oldOrigin.x() - radius, oldOrigin.y() - radius,
        radius * 2, radius * 2);
    updateAppearance();
}

void SkeletonGraphicsNodeItem::setMarkColor(QColor color)
{
    m_markColor = color;
    updateAppearance();
}

Document::Profile SkeletonGraphicsNodeItem::profile()
{
    return m_profile;
}

dust3d::Uuid SkeletonGraphicsNodeItem::id()
{
    return m_uuid;
}

void SkeletonGraphicsNodeItem::setId(dust3d::Uuid id)
{
    m_uuid = id;
}

void SkeletonGraphicsNodeItem::setHovered(bool hovered)
{
    m_hovered = hovered;
    updateAppearance();
}

void SkeletonGraphicsNodeItem::setChecked(bool checked)
{
    m_checked = checked;
    updateAppearance();
}

void SkeletonGraphicsNodeItem::setDeactivated(bool deactivated)
{
    m_deactivated = deactivated;
    updateAppearance();
}

bool SkeletonGraphicsNodeItem::deactivated()
{
    return m_deactivated;
}

bool SkeletonGraphicsNodeItem::checked()
{
    return m_checked;
}

bool SkeletonGraphicsNodeItem::hovered()
{
    return m_hovered;
}