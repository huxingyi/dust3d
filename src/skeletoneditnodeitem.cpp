#include <QPen>
#include "skeletoneditnodeitem.h"
#include "theme.h"

SkeletonEditNodeItem::SkeletonEditNodeItem(const QRectF &rect, QGraphicsItem *parent) :
    QGraphicsEllipseItem(rect, parent),
    m_hovered(false),
    m_checked(false),
    m_nextSidePair(NULL),
    m_sideColor(Theme::red),
    m_sideColorName("red")
{
    setData(0, "node");
    updateAppearance();
}

const QString &SkeletonEditNodeItem::sideColorName()
{
    return m_sideColorName;
}

QString SkeletonEditNodeItem::nextSideColorName()
{
    return Theme::nextSideColorNameMap[m_sideColorName];
}

void SkeletonEditNodeItem::setSideColorName(const QString &name)
{
    m_sideColorName = name;
    m_sideColor = Theme::sideColorNameToColorMap[m_sideColorName];
    updateAppearance();
}

bool SkeletonEditNodeItem::hovered()
{
    return m_hovered;
}

void SkeletonEditNodeItem::setHovered(bool hovered)
{
    m_hovered = hovered;
    updateAppearance();
}

bool SkeletonEditNodeItem::checked()
{
    return m_checked;
}

void SkeletonEditNodeItem::setChecked(bool checked)
{
    m_checked = checked;
    updateAppearance();
}

SkeletonEditNodeItem *SkeletonEditNodeItem::nextSidePair()
{
    return m_nextSidePair;
}

void SkeletonEditNodeItem::setNextSidePair(SkeletonEditNodeItem *nodeItem)
{
    m_nextSidePair = nodeItem;
    updateAppearance();
}

const QColor &SkeletonEditNodeItem::sideColor()
{
    return m_sideColor;
}

QPointF SkeletonEditNodeItem::origin()
{
    return QPointF(rect().x() + rect().width() / 2,
        rect().y() + rect().height() / 2);
}

float SkeletonEditNodeItem::radius()
{
    return rect().width() / 2;
}

void SkeletonEditNodeItem::setRadius(float radius)
{
    QPointF oldOrigin = origin();
    setRect(oldOrigin.x() - radius, oldOrigin.y() - radius,
        radius * 2, radius * 2);
}

void SkeletonEditNodeItem::setOrigin(QPointF point)
{
    QPointF moveBy = point - origin();
    QRectF newRect = rect();
    newRect.adjust(moveBy.x(), moveBy.y(), moveBy.x(), moveBy.y());
    setRect(newRect);
}

void SkeletonEditNodeItem::updateAppearance()
{
    QColor penColor = m_sideColor;
    penColor.setAlphaF(m_checked ? Theme::checkedAlpha : Theme::normalAlpha);
    QPen pen(penColor);
    pen.setWidth(Theme::skeletonNodeBorderSize);
    setPen(pen);
    
    QColor brushColor = m_sideColor;
    brushColor.setAlphaF((m_hovered || m_checked) ? Theme::fillAlpha : 0);
    QBrush brush(brushColor);
    setBrush(brush);
}

QColor SkeletonEditNodeItem::nextSideColor()
{
    return Theme::sideColorNameToColorMap[Theme::nextSideColorNameMap[m_sideColorName]];
}

