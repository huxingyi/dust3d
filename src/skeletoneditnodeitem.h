#ifndef SKELETON_EDIT_NODE_ITEM_H
#define SKELETON_EDIT_NODE_ITEM_H
#include <QGraphicsEllipseItem>
#include <map>
#include <QString>
#include <QColor>

class SkeletonEditEdgeItem;

class SkeletonEditNodeItem : public QGraphicsEllipseItem
{
public:
    SkeletonEditNodeItem(const QRectF &rect, QGraphicsItem *parent = 0);
    QPointF origin();
    void setOrigin(QPointF point);
    float radius();
    void setRadius(float radius);
    bool hovered();
    void setHovered(bool hovered);
    bool checked();
    void setChecked(bool checked);
    void markAsBranch(bool isBranch);
    bool isBranch();
    void markAsRoot(bool isRoot);
    bool isRoot();
    SkeletonEditNodeItem *nextSidePair();
    void setNextSidePair(SkeletonEditNodeItem *nodeItem);
    const QColor &sideColor();
    QColor nextSideColor();
    const QString &sideColorName();
    QString nextSideColorName();
    void setSideColorName(const QString &name);
private:
    bool m_hovered;
    bool m_checked;
    SkeletonEditNodeItem *m_nextSidePair;
    QColor m_sideColor;
    QString m_sideColorName;
    bool m_isBranch;
    bool m_isRoot;
private:
    void updateAppearance();
};

#endif

