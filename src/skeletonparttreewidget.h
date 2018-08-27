#ifndef SKELETON_PART_TREE_WIDGET_H
#define SKELETON_PART_TREE_WIDGET_H
#include <QTreeWidget>
#include <QUuid>
#include <QMouseEvent>
#include "skeletondocument.h"

class SkeletonPartTreeWidget : public QTreeWidget
{
    Q_OBJECT
signals:
    void currentComponentChanged(QUuid componentId);
    void moveComponentUp(QUuid componentId);
    void moveComponentDown(QUuid componentId);
    void moveComponentToTop(QUuid componentId);
    void moveComponentToBottom(QUuid componentId);
    void checkPart(QUuid partId);
    void createNewComponentAndMoveThisIn(QUuid componentId);
    void renameComponent(QUuid componentId, QString name);
    void setComponentExpandState(QUuid componentId, bool expanded);
    void moveComponent(QUuid componentId, QUuid toParentId);
    void removeComponent(QUuid componentId);
    void hideOtherComponents(QUuid componentId);
    void lockOtherComponents(QUuid componentId);
    void hideAllComponents();
    void showAllComponents();
    void collapseAllComponents();
    void expandAllComponents();
    void lockAllComponents();
    void unlockAllComponents();
    void setPartLockState(QUuid partId, bool locked);
    void setPartVisibleState(QUuid partId, bool visible);
    void setComponentInverseState(QUuid componentId, bool inverse);
    void hideDescendantComponents(QUuid componentId);
    void showDescendantComponents(QUuid componentId);
    void lockDescendantComponents(QUuid componentId);
    void unlockDescendantComponents(QUuid componentId);
public:
    SkeletonPartTreeWidget(const SkeletonDocument *document, QWidget *parent);
    QTreeWidgetItem *findComponentItem(QUuid componentId);
public slots:
    void componentNameChanged(QUuid componentId);
    void componentChildrenChanged(QUuid componentId);
    void componentRemoved(QUuid componentId);
    void componentAdded(QUuid componentId);
    void componentExpandStateChanged(QUuid componentId);
    void partRemoved(QUuid partId);
    void partPreviewChanged(QUuid partid);
    void partLockStateChanged(QUuid partId);
    void partVisibleStateChanged(QUuid partId);
    void partSubdivStateChanged(QUuid partId);
    void partDisableStateChanged(QUuid partId);
    void partXmirrorStateChanged(QUuid partId);
    void partDeformChanged(QUuid partId);
    void partRoundStateChanged(QUuid partId);
    void partColorStateChanged(QUuid partId);
    void partChecked(QUuid partId);
    void partUnchecked(QUuid partId);
    void groupChanged(QTreeWidgetItem *item, int column);
    void groupExpanded(QTreeWidgetItem *item);
    void groupCollapsed(QTreeWidgetItem *item);
    void currentGroupChanged(QTreeWidgetItem *current, QTreeWidgetItem *previous);
    void removeAllContent();
    void showContextMenu(const QPoint &pos);
protected:
    virtual QSize sizeHint() const;
    virtual void mousePressEvent(QMouseEvent *event);
private:
    void addComponentChildrenToItem(QUuid componentId, QTreeWidgetItem *parentItem);
private:
    const SkeletonDocument *m_document = nullptr;
    std::map<QUuid, QTreeWidgetItem *> m_partItemMap;
    std::map<QUuid, QTreeWidgetItem *> m_componentItemMap;
};

#endif
