#include <QMenu>
#include <vector>
#include "skeletonparttreewidget.h"
#include "skeletonpartwidget.h"

SkeletonPartTreeWidget::SkeletonPartTreeWidget(const SkeletonDocument *document, QWidget *parent) :
    QTreeWidget(parent),
    m_document(document)
{
    setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    setFocusPolicy(Qt::NoFocus);
    
    QPalette palette = this->palette();
    palette.setColor(QPalette::Window, Qt::transparent);
    palette.setColor(QPalette::Base, Qt::transparent);
    setPalette(palette);
    
    setColumnCount(1);
    setHeaderHidden(true);
    
    m_componentItemMap[QUuid()] = invisibleRootItem();
    
    setContextMenuPolicy(Qt::CustomContextMenu);
    
    setStyleSheet("QTreeView {qproperty-indentation: 10; margin-left: 5px; margin-top: 5px;}"
        "QTreeView::branch:has-siblings:!adjoins-item {border-image: url(:/resources/tree-vline.png);}"
        "QTreeView::branch:has-siblings:adjoins-item {border-image: url(:/resources/tree-branch-more.png);}"
        "QTreeView::branch:!has-children:!has-siblings:adjoins-item {border-image: url(:/resources/tree-branch-end.png);}"
        "QTreeView::branch:has-children:!has-siblings:closed, QTreeView::branch:closed:has-children:has-siblings {border-image: none; image: url(:/resources/tree-branch-closed.png);}"
        "QTreeView::branch:open:has-children:!has-siblings, QTreeView::branch:open:has-children:has-siblings  {border-image: none; image: url(:/resources/tree-branch-open.png);}");
    
    QFont font;
    font.setWeight(QFont::Light);
    font.setPixelSize(9);
    font.setBold(false);
    setFont(font);
    
    connect(this, &QTreeWidget::customContextMenuRequested, this, &SkeletonPartTreeWidget::showContextMenu);
    connect(this, &QTreeWidget::itemChanged, this, &SkeletonPartTreeWidget::groupChanged);
    connect(this, &QTreeWidget::itemExpanded, this, &SkeletonPartTreeWidget::groupExpanded);
    connect(this, &QTreeWidget::itemCollapsed, this, &SkeletonPartTreeWidget::groupCollapsed);
    connect(this, &QTreeWidget::currentItemChanged, this, &SkeletonPartTreeWidget::currentGroupChanged);
}

void SkeletonPartTreeWidget::currentGroupChanged(QTreeWidgetItem *current, QTreeWidgetItem *previous)
{
    if (nullptr != current) {
        auto componentId = QUuid(current->data(0, Qt::UserRole).toString());
        const SkeletonComponent *component = m_document->findComponent(componentId);
        if (nullptr != component) {
            emit currentComponentChanged(componentId);
            return;
        }
    }
    emit currentComponentChanged(QUuid());
}

void SkeletonPartTreeWidget::mousePressEvent(QMouseEvent *event)
{
    QModelIndex item = indexAt(event->pos());
    if (item.isValid()) {
        QTreeView::mousePressEvent(event);
    } else {
        clearSelection();
        QTreeView::mousePressEvent(event);
        emit currentComponentChanged(QUuid());
    }
}

void SkeletonPartTreeWidget::showContextMenu(const QPoint &pos)
{
    QTreeWidgetItem *item = itemAt(pos);
    if (nullptr == item)
        return;
    
    auto componentId = QUuid(item->data(0, Qt::UserRole).toString());
    const SkeletonComponent *component = m_document->findComponent(componentId);
    if (nullptr == component) {
        qDebug() << "Find component failed:" << componentId;
        return;
    }
    
    const SkeletonPart *part = nullptr;
    if (!component->linkToPartId.isNull()) {
        part = m_document->findPart(component->linkToPartId);
    }
    
    QMenu contextMenu(this);
    
    if (!component->name.isEmpty())
        contextMenu.addSection(component->name);
    
    QAction showAction(tr("Show"), this);
    QAction hideAction(tr("Hide"), this);
    QAction lockAction(tr("Lock"), this);
    QAction unlockAction(tr("Unlock"), this);
    QAction invertAction(tr("Invert"), this);
    QAction cancelInverseAction(tr("Cancel Inverse"), this);
    
    if (!component->linkToPartId.isNull()) {
        emit checkPart(component->linkToPartId);
        
        if (nullptr != part) {
            if (part->visible) {
                connect(&hideAction, &QAction::triggered, [=]() {
                    emit setPartVisibleState(component->linkToPartId, false);
                });
                contextMenu.addAction(&hideAction);
            } else {
                connect(&showAction, &QAction::triggered, [=]() {
                    emit setPartVisibleState(component->linkToPartId, true);
                });
                contextMenu.addAction(&showAction);
            }
            
            if (part->locked) {
                connect(&unlockAction, &QAction::triggered, [=]() {
                    emit setPartLockState(component->linkToPartId, false);
                });
                contextMenu.addAction(&unlockAction);
            } else {
                connect(&lockAction, &QAction::triggered, [=]() {
                    emit setPartLockState(component->linkToPartId, true);
                });
                contextMenu.addAction(&lockAction);
            }
        }
    } else {
        if (!component->childrenIds.empty()) {
            connect(&showAction, &QAction::triggered, [=]() {
                emit showDescendantComponents(componentId);
            });
            contextMenu.addAction(&showAction);
            
            connect(&hideAction, &QAction::triggered, [=]() {
                emit hideDescendantComponents(componentId);
            });
            contextMenu.addAction(&hideAction);
            
            connect(&lockAction, &QAction::triggered, [=]() {
                emit lockDescendantComponents(componentId);
            });
            contextMenu.addAction(&lockAction);
            
            connect(&unlockAction, &QAction::triggered, [=]() {
                emit unlockDescendantComponents(componentId);
            });
            contextMenu.addAction(&unlockAction);
        }
    }
    
    if (!component->inverse) {
        connect(&invertAction, &QAction::triggered, [=]() {
            emit setComponentInverseState(componentId, true);
        });
        contextMenu.addAction(&invertAction);
    }

    if (component->inverse) {
        connect(&cancelInverseAction, &QAction::triggered, [=]() {
            emit setComponentInverseState(componentId, false);
        });
        contextMenu.addAction(&cancelInverseAction);
    }
    
    contextMenu.addSeparator();
    
    QAction hideOthersAction(tr("Hide Others"), this);
    connect(&hideOthersAction, &QAction::triggered, [=]() {
        emit hideOtherComponents(componentId);
    });
    contextMenu.addAction(&hideOthersAction);
    
    QAction lockOthersAction(tr("Lock Others"), this);
    connect(&lockOthersAction, &QAction::triggered, [=]() {
        emit lockOtherComponents(componentId);
    });
    contextMenu.addAction(&lockOthersAction);
    
    contextMenu.addSeparator();
    
    QAction collapseAllAction(tr("Collapse All"), this);
    connect(&collapseAllAction, &QAction::triggered, [=]() {
        emit collapseAllComponents();
    });
    contextMenu.addAction(&collapseAllAction);
    
    QAction expandAllAction(tr("Expand All"), this);
    connect(&expandAllAction, &QAction::triggered, [=]() {
        emit expandAllComponents();
    });
    contextMenu.addAction(&expandAllAction);
    
    contextMenu.addSeparator();
    
    QAction hideAllAction(tr("Hide All"), this);
    connect(&hideAllAction, &QAction::triggered, [=]() {
        emit hideAllComponents();
    });
    contextMenu.addAction(&hideAllAction);
    
    QAction showAllAction(tr("Show All"), this);
    connect(&showAllAction, &QAction::triggered, [=]() {
        emit showAllComponents();
    });
    contextMenu.addAction(&showAllAction);
    
    contextMenu.addSeparator();
    
    QAction lockAllAction(tr("Lock All"), this);
    connect(&lockAllAction, &QAction::triggered, [=]() {
        emit lockAllComponents();
    });
    contextMenu.addAction(&lockAllAction);
    
    QAction unlockAllAction(tr("Unlock All"), this);
    connect(&unlockAllAction, &QAction::triggered, [=]() {
        emit unlockAllComponents();
    });
    contextMenu.addAction(&unlockAllAction);
    
    contextMenu.addSeparator();
    
    QMenu *moveToMenu = contextMenu.addMenu(tr("Move To"));
    
    QAction moveUpAction(tr("Up"), this);
    connect(&moveUpAction, &QAction::triggered, [=]() {
        emit moveComponentUp(componentId);
    });
    moveToMenu->addAction(&moveUpAction);
    
    QAction moveDownAction(tr("Down"), this);
    connect(&moveDownAction, &QAction::triggered, [=]() {
        emit moveComponentDown(componentId);
    });
    moveToMenu->addAction(&moveDownAction);
    
    QAction moveToTopAction(tr("Top"), this);
    connect(&moveToTopAction, &QAction::triggered, [=]() {
        emit moveComponentToTop(componentId);
    });
    moveToMenu->addAction(&moveToTopAction);
    
    QAction moveToBottomAction(tr("Bottom"), this);
    connect(&moveToBottomAction, &QAction::triggered, [=]() {
        emit moveComponentToBottom(componentId);
    });
    moveToMenu->addAction(&moveToBottomAction);
    
    moveToMenu->addSeparator();
    
    QAction moveToNewGroupAction(tr("New Group"), this);
    moveToMenu->addAction(&moveToNewGroupAction);
    connect(&moveToNewGroupAction, &QAction::triggered, [=]() {
        emit createNewComponentAndMoveThisIn(componentId);
    });
    
    moveToMenu->addSeparator();

    QAction moveToRootGroupAction(tr("Root"), this);
    moveToMenu->addAction(&moveToRootGroupAction);
    connect(&moveToRootGroupAction, &QAction::triggered, [=]() {
        emit moveComponent(componentId, QUuid());
    });
    
    std::vector<QAction *> groupsActions;
    std::function<void(QUuid, int)> addChildGroupsFunc;
    addChildGroupsFunc = [this, &groupsActions, &addChildGroupsFunc, &moveToMenu, &componentId](QUuid currentId, int tabs) -> void {
        const SkeletonComponent *current = m_document->findComponent(currentId);
        if (nullptr == current)
            return;
        if (!current->id.isNull() && current->linkDataType().isEmpty()) {
            QAction *action = new QAction(QString(" ").repeated(tabs * 4) + current->name, this);
            connect(action, &QAction::triggered, [=]() {
                emit moveComponent(componentId, current->id);
            });
            groupsActions.push_back(action);
            moveToMenu->addAction(action);
        }
        for (const auto &childId: current->childrenIds) {
            addChildGroupsFunc(childId, tabs + 1);
        }
    };
    addChildGroupsFunc(QUuid(), 0);
    
    contextMenu.addSeparator();
    
    QAction deleteAction(tr("Delete"), this);
    connect(&deleteAction, &QAction::triggered, [=]() {
        emit removeComponent(componentId);
    });
    contextMenu.addAction(&deleteAction);
    
    contextMenu.exec(mapToGlobal(pos));
    
    for (const auto &action: groupsActions) {
        delete action;
    }
}

QTreeWidgetItem *SkeletonPartTreeWidget::findComponentItem(QUuid componentId)
{
    auto findResult = m_componentItemMap.find(componentId);
    if (findResult == m_componentItemMap.end())
        return nullptr;
    
    return findResult->second;
}

void SkeletonPartTreeWidget::componentNameChanged(QUuid componentId)
{
    auto componentItem = m_componentItemMap.find(componentId);
    if (componentItem == m_componentItemMap.end()) {
        qDebug() << "Find component item failed:" << componentId;
        return;
    }
    
    const SkeletonComponent *component = m_document->findComponent(componentId);
    if (nullptr == component) {
        qDebug() << "Find component failed:" << componentId;
        return;
    }
    
    componentItem->second->setText(0, component->name);
}

void SkeletonPartTreeWidget::componentExpandStateChanged(QUuid componentId)
{
    auto componentItem = m_componentItemMap.find(componentId);
    if (componentItem == m_componentItemMap.end()) {
        qDebug() << "Find component item failed:" << componentId;
        return;
    }
    
    const SkeletonComponent *component = m_document->findComponent(componentId);
    if (nullptr == component) {
        qDebug() << "Find component failed:" << componentId;
        return;
    }
    
    componentItem->second->setExpanded(component->expanded);
}

void SkeletonPartTreeWidget::addComponentChildrenToItem(QUuid componentId, QTreeWidgetItem *parentItem)
{
    const SkeletonComponent *parentComponent = m_document->findComponent(componentId);
    if (nullptr == parentComponent)
        return;
    
    for (const auto &childId: parentComponent->childrenIds) {
        const SkeletonComponent *component = m_document->findComponent(childId);
        if (nullptr == component)
            continue;
        if (!component->linkToPartId.isNull()) {
            QTreeWidgetItem *item = new QTreeWidgetItem();
            parentItem->addChild(item);
            item->setData(0, Qt::UserRole, QVariant(component->id.toString()));
            item->setFlags(item->flags() & ~(Qt::ItemIsEditable | Qt::ItemIsEnabled | Qt::ItemIsSelectable));
            QUuid partId = component->linkToPartId;
            SkeletonPartWidget *widget = new SkeletonPartWidget(m_document, partId);
            item->setSizeHint(0, SkeletonPartWidget::preferredSize());
            setItemWidget(item, 0, widget);
            widget->reload();
            m_partItemMap[partId] = item;
        } else {
            QTreeWidgetItem *item = new QTreeWidgetItem(QStringList(component->name));
            parentItem->addChild(item);
            item->setData(0, Qt::UserRole, QVariant(component->id.toString()));
            item->setChildIndicatorPolicy(QTreeWidgetItem::ShowIndicator);
            item->setExpanded(component->expanded);
            item->setFlags(item->flags() | Qt::ItemIsEditable | Qt::ItemIsEnabled | Qt::ItemIsSelectable);
            m_componentItemMap[childId] = item;
            addComponentChildrenToItem(childId, item);
        }
    }
}

void SkeletonPartTreeWidget::componentChildrenChanged(QUuid componentId)
{
    QTreeWidgetItem *parentItem = findComponentItem(componentId);
    if (nullptr == parentItem) {
        qDebug() << "Find component item failed:" << componentId;
        return;
    }
    qDeleteAll(parentItem->takeChildren());
    addComponentChildrenToItem(componentId, parentItem);
    
    // Fix the last item show in the wrong place sometimes
    int childCount = invisibleRootItem()->childCount();
    if (childCount > 0) {
        QTreeWidgetItem *lastItem = invisibleRootItem()->child(childCount - 1);
        bool isExpanded = lastItem->isExpanded();
        lastItem->setExpanded(!isExpanded);
        lastItem->setExpanded(isExpanded);
    }
}

void SkeletonPartTreeWidget::removeAllContent()
{
    qDeleteAll(invisibleRootItem()->takeChildren());
    m_partItemMap.clear();
    m_componentItemMap.clear();
    m_componentItemMap[QUuid()] = invisibleRootItem();
}

void SkeletonPartTreeWidget::componentRemoved(QUuid componentId)
{
    auto componentItem = m_componentItemMap.find(componentId);
    if (componentItem == m_componentItemMap.end())
        return;
    
    m_componentItemMap.erase(componentId);
}

void SkeletonPartTreeWidget::componentAdded(QUuid componentId)
{
    // ignore
}

void SkeletonPartTreeWidget::partRemoved(QUuid partId)
{
    auto partItem = m_partItemMap.find(partId);
    if (partItem == m_partItemMap.end())
        return;
    
    m_partItemMap.erase(partItem);
}

void SkeletonPartTreeWidget::groupChanged(QTreeWidgetItem *item, int column)
{
    if (0 != column)
        return;
    
    auto componentId = QUuid(item->data(0, Qt::UserRole).toString());
    
    const SkeletonComponent *component = m_document->findComponent(componentId);
    if (nullptr == component) {
        qDebug() << "Find component failed:" << componentId;
        return;
    }
    
    if (item->text(0) != component->name)
        emit renameComponent(componentId, item->text(0));
}

void SkeletonPartTreeWidget::groupExpanded(QTreeWidgetItem *item)
{
    QUuid componentId = QUuid(item->data(0, Qt::UserRole).toString());
    emit setComponentExpandState(componentId, true);
}

void SkeletonPartTreeWidget::groupCollapsed(QTreeWidgetItem *item)
{
    QUuid componentId = QUuid(item->data(0, Qt::UserRole).toString());
    emit setComponentExpandState(componentId, false);
}

void SkeletonPartTreeWidget::partPreviewChanged(QUuid partId)
{
    auto item = m_partItemMap.find(partId);
    if (item == m_partItemMap.end()) {
        qDebug() << "Part item not found:" << partId;
        return;
    }
    SkeletonPartWidget *widget = (SkeletonPartWidget *)itemWidget(item->second, 0);
    widget->updatePreview();
}

void SkeletonPartTreeWidget::partLockStateChanged(QUuid partId)
{
    auto item = m_partItemMap.find(partId);
    if (item == m_partItemMap.end()) {
        qDebug() << "Part item not found:" << partId;
        return;
    }
    SkeletonPartWidget *widget = (SkeletonPartWidget *)itemWidget(item->second, 0);
    widget->updateLockButton();
}

void SkeletonPartTreeWidget::partVisibleStateChanged(QUuid partId)
{
    auto item = m_partItemMap.find(partId);
    if (item == m_partItemMap.end()) {
        qDebug() << "Part item not found:" << partId;
        return;
    }
    SkeletonPartWidget *widget = (SkeletonPartWidget *)itemWidget(item->second, 0);
    widget->updateVisibleButton();
}

void SkeletonPartTreeWidget::partSubdivStateChanged(QUuid partId)
{
    auto item = m_partItemMap.find(partId);
    if (item == m_partItemMap.end()) {
        qDebug() << "Part item not found:" << partId;
        return;
    }
    SkeletonPartWidget *widget = (SkeletonPartWidget *)itemWidget(item->second, 0);
    widget->updateSubdivButton();
}

void SkeletonPartTreeWidget::partDisableStateChanged(QUuid partId)
{
    auto item = m_partItemMap.find(partId);
    if (item == m_partItemMap.end()) {
        qDebug() << "Part item not found:" << partId;
        return;
    }
    SkeletonPartWidget *widget = (SkeletonPartWidget *)itemWidget(item->second, 0);
    widget->updateDisableButton();
}

void SkeletonPartTreeWidget::partXmirrorStateChanged(QUuid partId)
{
    auto item = m_partItemMap.find(partId);
    if (item == m_partItemMap.end()) {
        qDebug() << "Part item not found:" << partId;
        return;
    }
    SkeletonPartWidget *widget = (SkeletonPartWidget *)itemWidget(item->second, 0);
    widget->updateXmirrorButton();
}

void SkeletonPartTreeWidget::partDeformChanged(QUuid partId)
{
    auto item = m_partItemMap.find(partId);
    if (item == m_partItemMap.end()) {
        qDebug() << "Part item not found:" << partId;
        return;
    }
    SkeletonPartWidget *widget = (SkeletonPartWidget *)itemWidget(item->second, 0);
    widget->updateDeformButton();
}

void SkeletonPartTreeWidget::partRoundStateChanged(QUuid partId)
{
    auto item = m_partItemMap.find(partId);
    if (item == m_partItemMap.end()) {
        qDebug() << "Part item not found:" << partId;
        return;
    }
    SkeletonPartWidget *widget = (SkeletonPartWidget *)itemWidget(item->second, 0);
    widget->updateRoundButton();
}

void SkeletonPartTreeWidget::partColorStateChanged(QUuid partId)
{
    auto item = m_partItemMap.find(partId);
    if (item == m_partItemMap.end()) {
        qDebug() << "Part item not found:" << partId;
        return;
    }
    SkeletonPartWidget *widget = (SkeletonPartWidget *)itemWidget(item->second, 0);
    widget->updateColorButton();
}

void SkeletonPartTreeWidget::partChecked(QUuid partId)
{
    auto item = m_partItemMap.find(partId);
    if (item == m_partItemMap.end()) {
        qDebug() << "Part item not found:" << partId;
        return;
    }
    SkeletonPartWidget *widget = (SkeletonPartWidget *)itemWidget(item->second, 0);
    widget->updateCheckedState(true);
}

void SkeletonPartTreeWidget::partUnchecked(QUuid partId)
{
    auto item = m_partItemMap.find(partId);
    if (item == m_partItemMap.end()) {
        //qDebug() << "Part item not found:" << partId;
        return;
    }
    SkeletonPartWidget *widget = (SkeletonPartWidget *)itemWidget(item->second, 0);
    widget->updateCheckedState(false);
}

QSize SkeletonPartTreeWidget::sizeHint() const
{
    QSize size = SkeletonPartWidget::preferredSize();
    return QSize(size.width() * 1.35, size.height() * 5.5);
}

