#ifndef DUST3D_APPLICATION_PART_TREE_WIDGET_H_
#define DUST3D_APPLICATION_PART_TREE_WIDGET_H_

#include <QTreeWidget>
#include <QMouseEvent>
#include <QTreeWidgetItem>
#include <QTimer>
#include <set>
#include <dust3d/base/uuid.h>
#include <dust3d/base/part_target.h>
#include <dust3d/base/part_base.h>
#include <dust3d/base/combine_mode.h>

class Document;

class PartTreeWidget : public QTreeWidget
{
    Q_OBJECT
signals:
    void currentComponentChanged(dust3d::Uuid componentId);
    void moveComponentUp(dust3d::Uuid componentId);
    void moveComponentDown(dust3d::Uuid componentId);
    void moveComponentToTop(dust3d::Uuid componentId);
    void moveComponentToBottom(dust3d::Uuid componentId);
    void checkPart(dust3d::Uuid partId);
    void createNewComponentAndMoveThisIn(dust3d::Uuid componentId);
    void createNewChildComponent(dust3d::Uuid parentComponentId);
    void renameComponent(dust3d::Uuid componentId, QString name);
    void setComponentExpandState(dust3d::Uuid componentId, bool expanded);
    void setPartTarget(dust3d::Uuid partId, dust3d::PartTarget target);
    void setPartBase(dust3d::Uuid partId, dust3d::PartBase base);
    void moveComponent(dust3d::Uuid componentId, dust3d::Uuid toParentId);
    void removeComponent(dust3d::Uuid componentId);
    void hideOtherComponents(dust3d::Uuid componentId);
    void lockOtherComponents(dust3d::Uuid componentId);
    void hideAllComponents();
    void showAllComponents();
    void showOrHideAllComponents();
    void collapseAllComponents();
    void expandAllComponents();
    void lockAllComponents();
    void unlockAllComponents();
    void setPartLockState(dust3d::Uuid partId, bool locked);
    void setPartVisibleState(dust3d::Uuid partId, bool visible);
    void setPartColorState(dust3d::Uuid partId, bool hasColor, QColor color);
    void setComponentCombineMode(dust3d::Uuid componentId, dust3d::CombineMode combineMode);
    void hideDescendantComponents(dust3d::Uuid componentId);
    void showDescendantComponents(dust3d::Uuid componentId);
    void lockDescendantComponents(dust3d::Uuid componentId);
    void unlockDescendantComponents(dust3d::Uuid componentId);
    void addPartToSelection(dust3d::Uuid partId);
    void groupOperationAdded();
public:
    PartTreeWidget(const Document *document, QWidget *parent);
    QTreeWidgetItem *findComponentItem(dust3d::Uuid componentId);
public slots:
    void componentNameChanged(dust3d::Uuid componentId);
    void componentChildrenChanged(dust3d::Uuid componentId);
    void componentRemoved(dust3d::Uuid componentId);
    void componentAdded(dust3d::Uuid componentId);
    void componentExpandStateChanged(dust3d::Uuid componentId);
    void componentCombineModeChanged(dust3d::Uuid componentId);
    void componentTargetChanged(dust3d::Uuid componentId);
    void partRemoved(dust3d::Uuid partId);
    void partPreviewChanged(dust3d::Uuid partid);
    void partLockStateChanged(dust3d::Uuid partId);
    void partVisibleStateChanged(dust3d::Uuid partId);
    void partSubdivStateChanged(dust3d::Uuid partId);
    void partDisableStateChanged(dust3d::Uuid partId);
    void partXmirrorStateChanged(dust3d::Uuid partId);
    void partDeformChanged(dust3d::Uuid partId);
    void partRoundStateChanged(dust3d::Uuid partId);
    void partChamferStateChanged(dust3d::Uuid partId);
    void partColorStateChanged(dust3d::Uuid partId);
    void partCutRotationChanged(dust3d::Uuid partId);
    void partCutFaceChanged(dust3d::Uuid partId);
    void partHollowThicknessChanged(dust3d::Uuid partId);
    void partMaterialIdChanged(dust3d::Uuid partId);
    void partColorSolubilityChanged(dust3d::Uuid partId);
    void partMetalnessChanged(dust3d::Uuid partId);
    void partRoughnessChanged(dust3d::Uuid partId);
    void partCountershadeStateChanged(dust3d::Uuid partId);
    void partSmoothStateChanged(dust3d::Uuid partId);
    void partChecked(dust3d::Uuid partId);
    void partUnchecked(dust3d::Uuid partId);
    void partComponentChecked(dust3d::Uuid partId);
    void groupChanged(QTreeWidgetItem *item, int column);
    void groupExpanded(QTreeWidgetItem *item);
    void groupCollapsed(QTreeWidgetItem *item);
    void removeAllContent();
    void showContextMenu(const QPoint &pos, bool shorted=false);
protected:
    QSize sizeHint() const override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;
private:
    void addComponentChildrenToItem(dust3d::Uuid componentId, QTreeWidgetItem *parentItem);
    void deleteItemChildren(QTreeWidgetItem *item);
    void selectComponent(dust3d::Uuid componentId, bool multiple=false);
    void updateComponentSelectState(dust3d::Uuid componentId, bool selected);
    void updateComponentAppearance(dust3d::Uuid componentId);
    bool isComponentSelected(dust3d::Uuid componentId);
    std::vector<dust3d::Uuid> collectSelectedComponentIds(const QPoint &pos);
    void handleSingleClick(const QPoint &pos);
    void reloadComponentChildren(const dust3d::Uuid &componentId);
    void removeComponentDelayedTimer(const dust3d::Uuid &componentId);
private:
    const Document *m_document = nullptr;
    QTreeWidgetItem *m_rootItem = nullptr;
    bool m_firstSelect = true;
    std::map<dust3d::Uuid, QTreeWidgetItem *> m_partItemMap;
    std::map<dust3d::Uuid, QTreeWidgetItem *> m_componentItemMap;
    QFont m_normalFont;
    QFont m_selectedFont;
    dust3d::Uuid m_currentSelectedComponentId;
    QBrush m_hightlightedPartBackground;
    dust3d::Uuid m_shiftStartComponentId;
    std::set<dust3d::Uuid> m_selectedComponentIds;
    std::map<dust3d::Uuid, QTimer *> m_delayedComponentTimers;
};

#endif
