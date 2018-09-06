#include <QMenu>
#include <vector>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QWidgetAction>
#include "skeletonparttreewidget.h"
#include "skeletonpartwidget.h"
#include "skeletongraphicswidget.h"
#include "floatnumberwidget.h"

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
    //setIconSize(QSize(Theme::miniIconFontSize, Theme::miniIconFontSize));
    
    setStyleSheet("QTreeView {qproperty-indentation: 10; margin-left: 5px; margin-top: 5px;}"
        "QTreeView::branch:has-siblings:!adjoins-item {border-image: url(:/resources/tree-vline.png);}"
        "QTreeView::branch:has-siblings:adjoins-item {border-image: url(:/resources/tree-branch-more.png);}"
        "QTreeView::branch:!has-children:!has-siblings:adjoins-item {border-image: url(:/resources/tree-branch-end.png);}"
        "QTreeView::branch:has-children:!has-siblings:closed, QTreeView::branch:closed:has-children:has-siblings {border-image: none; image: url(:/resources/tree-branch-closed.png);}"
        "QTreeView::branch:open:has-children:!has-siblings, QTreeView::branch:open:has-children:has-siblings  {border-image: none; image: url(:/resources/tree-branch-open.png);}");
    
    m_normalFont.setWeight(QFont::Light);
    m_normalFont.setPixelSize(9);
    m_normalFont.setBold(false);
    
    m_selectedFont.setWeight(QFont::Light);
    m_selectedFont.setPixelSize(9);
    m_selectedFont.setBold(true);
    
    setFont(m_normalFont);
    
    connect(this, &QTreeWidget::customContextMenuRequested, this, &SkeletonPartTreeWidget::showContextMenu);
    connect(this, &QTreeWidget::itemChanged, this, &SkeletonPartTreeWidget::groupChanged);
    connect(this, &QTreeWidget::itemExpanded, this, &SkeletonPartTreeWidget::groupExpanded);
    connect(this, &QTreeWidget::itemCollapsed, this, &SkeletonPartTreeWidget::groupCollapsed);
}

void SkeletonPartTreeWidget::selectComponent(QUuid componentId)
{
    if (m_currentSelectedComponent != componentId) {
        if (!m_currentSelectedComponent.isNull()) {
            auto item = m_componentItemMap.find(m_currentSelectedComponent);
            if (item != m_componentItemMap.end()) {
                item->second->setFont(0, m_normalFont);
            }
        }
        m_currentSelectedComponent = componentId;
        if (!m_currentSelectedComponent.isNull()) {
            auto item = m_componentItemMap.find(m_currentSelectedComponent);
            if (item != m_componentItemMap.end()) {
                item->second->setFont(0, m_selectedFont);
            }
        }
        emit currentComponentChanged(m_currentSelectedComponent);
    }
}

void SkeletonPartTreeWidget::mousePressEvent(QMouseEvent *event)
{
    QModelIndex itemIndex = indexAt(event->pos());
    QTreeView::mousePressEvent(event);
    if (itemIndex.isValid()) {
        QTreeWidgetItem *item = itemFromIndex(itemIndex);
        auto componentId = QUuid(item->data(0, Qt::UserRole).toString());
        if (m_componentItemMap.find(componentId) != m_componentItemMap.end()) {
            selectComponent(componentId);
            return;
        }
        item = item->parent();
        if (nullptr != item) {
            componentId = QUuid(item->data(0, Qt::UserRole).toString());
            if (m_componentItemMap.find(componentId) != m_componentItemMap.end()) {
                selectComponent(componentId);
                return;
            }
        }
    }
    selectComponent(QUuid());
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
    showAction.setIcon(Theme::awesome()->icon(fa::eye));
    QAction hideAction(tr("Hide"), this);
    hideAction.setIcon(Theme::awesome()->icon(fa::eyeslash));
    QAction lockAction(tr("Lock"), this);
    lockAction.setIcon(Theme::awesome()->icon(fa::lock));
    QAction unlockAction(tr("Unlock"), this);
    unlockAction.setIcon(Theme::awesome()->icon(fa::unlock));
    QAction invertAction(tr("Invert"), this);
    QAction cancelInverseAction(tr("Cancel Inverse"), this);
    QAction selectAction(tr("Select"), this);
    
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
            connect(&selectAction, &QAction::triggered, [=]() {
                std::vector<QUuid> partIds;
                m_document->collectComponentDescendantParts(componentId, partIds);
                for (const auto &partId: partIds) {
                    emit addPartToSelection(partId);
                }
            });
            contextMenu.addAction(&selectAction);
            
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
    
    QMenu *smoothMenu = contextMenu.addMenu(tr("Smooth"));
    QWidgetAction smoothAction(this);
    smoothAction.setDefaultWidget(createSmoothMenuWidget(componentId));
    smoothMenu->addAction(&smoothAction);
    
    contextMenu.addSeparator();
    
    QAction hideOthersAction(tr("Hide Others"), this);
    hideOthersAction.setIcon(Theme::awesome()->icon(fa::eyeslash));
    connect(&hideOthersAction, &QAction::triggered, [=]() {
        emit hideOtherComponents(componentId);
    });
    contextMenu.addAction(&hideOthersAction);
    
    QAction lockOthersAction(tr("Lock Others"), this);
    lockOthersAction.setIcon(Theme::awesome()->icon(fa::lock));
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
    hideAllAction.setIcon(Theme::awesome()->icon(fa::eyeslash));
    connect(&hideAllAction, &QAction::triggered, [=]() {
        emit hideAllComponents();
    });
    contextMenu.addAction(&hideAllAction);
    
    QAction showAllAction(tr("Show All"), this);
    showAllAction.setIcon(Theme::awesome()->icon(fa::eye));
    connect(&showAllAction, &QAction::triggered, [=]() {
        emit showAllComponents();
    });
    contextMenu.addAction(&showAllAction);
    
    contextMenu.addSeparator();
    
    QAction lockAllAction(tr("Lock All"), this);
    lockAllAction.setIcon(Theme::awesome()->icon(fa::lock));
    connect(&lockAllAction, &QAction::triggered, [=]() {
        emit lockAllComponents();
    });
    contextMenu.addAction(&lockAllAction);
    
    QAction unlockAllAction(tr("Unlock All"), this);
    unlockAllAction.setIcon(Theme::awesome()->icon(fa::unlock));
    connect(&unlockAllAction, &QAction::triggered, [=]() {
        emit unlockAllComponents();
    });
    contextMenu.addAction(&unlockAllAction);
    
    contextMenu.addSeparator();
    
    QMenu *moveToMenu = contextMenu.addMenu(tr("Move To"));
    
    QAction moveToTopAction(tr("Top"), this);
    moveToTopAction.setIcon(Theme::awesome()->icon(fa::angledoubleup));
    connect(&moveToTopAction, &QAction::triggered, [=]() {
        emit moveComponentToTop(componentId);
    });
    moveToMenu->addAction(&moveToTopAction);
    
    QAction moveUpAction(tr("Up"), this);
    moveUpAction.setIcon(Theme::awesome()->icon(fa::angleup));
    connect(&moveUpAction, &QAction::triggered, [=]() {
        emit moveComponentUp(componentId);
    });
    moveToMenu->addAction(&moveUpAction);
    
    QAction moveDownAction(tr("Down"), this);
    moveDownAction.setIcon(Theme::awesome()->icon(fa::angledown));
    connect(&moveDownAction, &QAction::triggered, [=]() {
        emit moveComponentDown(componentId);
    });
    moveToMenu->addAction(&moveDownAction);
    
    QAction moveToBottomAction(tr("Bottom"), this);
    moveToBottomAction.setIcon(Theme::awesome()->icon(fa::angledoubledown));
    connect(&moveToBottomAction, &QAction::triggered, [=]() {
        emit moveComponentToBottom(componentId);
    });
    moveToMenu->addAction(&moveToBottomAction);
    
    moveToMenu->addSeparator();
    
    QAction moveToNewGroupAction(tr("New Group"), this);
    moveToNewGroupAction.setIcon(Theme::awesome()->icon(fa::file));
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
    deleteAction.setIcon(Theme::awesome()->icon(fa::trash));
    connect(&deleteAction, &QAction::triggered, [=]() {
        emit removeComponent(componentId);
    });
    contextMenu.addAction(&deleteAction);
    
    contextMenu.exec(mapToGlobal(pos));
    
    for (const auto &action: groupsActions) {
        delete action;
    }
}

QWidget *SkeletonPartTreeWidget::createSmoothMenuWidget(QUuid componentId)
{
    QWidget *popup = new QWidget;
    
    const SkeletonComponent *component = m_document->findComponent(componentId);
    if (!component) {
        qDebug() << "Find component failed:" << componentId;
        return popup;
    }
    
    bool showSeamControl = component->linkToPartId.isNull();
    
    FloatNumberWidget *smoothAllWidget = new FloatNumberWidget;
    smoothAllWidget->setItemName(tr("All"));
    smoothAllWidget->setRange(0, 1);
    smoothAllWidget->setValue(component->smoothAll);
    
    connect(smoothAllWidget, &FloatNumberWidget::valueChanged, [=](float value) {
        emit setComponentSmoothAll(componentId, value);
        emit groupOperationAdded();
    });
    
    QPushButton *smoothAllEraser = new QPushButton(QChar(fa::eraser));
    Theme::initAwesomeToolButton(smoothAllEraser);
    
    connect(smoothAllEraser, &QPushButton::clicked, [=]() {
        smoothAllWidget->setValue(0.0);
    });
    
    FloatNumberWidget *smoothSeamWidget = nullptr;
    QPushButton *smoothSeamEraser = nullptr;
    
    if (showSeamControl) {
        smoothSeamWidget = new FloatNumberWidget;
        smoothSeamWidget->setItemName(tr("Seam"));
        smoothSeamWidget->setRange(0, 1);
        smoothSeamWidget->setValue(component->smoothSeam);
        
        connect(smoothSeamWidget, &FloatNumberWidget::valueChanged, [=](float value) {
            emit setComponentSmoothSeam(componentId, value);
            emit groupOperationAdded();
        });
    
        smoothSeamEraser = new QPushButton(QChar(fa::eraser));
        Theme::initAwesomeToolButton(smoothSeamEraser);
        
        connect(smoothSeamEraser, &QPushButton::clicked, [=]() {
            smoothSeamWidget->setValue(0.0);
        });
    }
    
    QHBoxLayout *smoothSeamLayout = nullptr;
    
    QVBoxLayout *layout = new QVBoxLayout;
    QHBoxLayout *smoothAllLayout = new QHBoxLayout;
    if (showSeamControl)
        smoothSeamLayout = new QHBoxLayout;
    smoothAllLayout->addWidget(smoothAllEraser);
    smoothAllLayout->addWidget(smoothAllWidget);
    if (showSeamControl) {
        smoothSeamLayout->addWidget(smoothSeamEraser);
        smoothSeamLayout->addWidget(smoothSeamWidget);
    }
    layout->addLayout(smoothAllLayout);
    if (showSeamControl)
        layout->addLayout(smoothSeamLayout);
    
    popup->setLayout(layout);
    
    return popup;
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
            item->setFlags((item->flags() | Qt::ItemIsEditable | Qt::ItemIsEnabled) & ~(Qt::ItemIsSelectable));
            m_componentItemMap[childId] = item;
            addComponentChildrenToItem(childId, item);
        }
    }
}

void SkeletonPartTreeWidget::deleteItemChildren(QTreeWidgetItem *item)
{
    auto children = item->takeChildren();
    while (!children.isEmpty()) {
        auto first = children.takeFirst();
        auto componentId = QUuid(first->data(0, Qt::UserRole).toString());
        const SkeletonComponent *component = m_document->findComponent(componentId);
        if (nullptr != component) {
            m_componentItemMap.erase(componentId);
            if (!component->linkToPartId.isNull()) {
                m_partItemMap.erase(component->linkToPartId);
            }
        }
        deleteItemChildren(first);
        delete first;
    }
}

void SkeletonPartTreeWidget::componentChildrenChanged(QUuid componentId)
{
    QTreeWidgetItem *parentItem = findComponentItem(componentId);
    if (nullptr == parentItem) {
        qDebug() << "Find component item failed:" << componentId;
        return;
    }
    deleteItemChildren(parentItem);
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

void SkeletonPartTreeWidget::partWrapStateChanged(QUuid partId)
{
    auto item = m_partItemMap.find(partId);
    if (item == m_partItemMap.end()) {
        qDebug() << "Part item not found:" << partId;
        return;
    }
    SkeletonPartWidget *widget = (SkeletonPartWidget *)itemWidget(item->second, 0);
    widget->updateWrapButton();
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

void SkeletonPartTreeWidget::setGraphicsFunctions(SkeletonGraphicsFunctions *graphicsFunctions)
{
    m_graphicsFunctions = graphicsFunctions;
}

void SkeletonPartTreeWidget::keyPressEvent(QKeyEvent *event)
{
    QTreeWidget::keyPressEvent(event);
    if (m_graphicsFunctions)
        m_graphicsFunctions->keyPress(event);
}
