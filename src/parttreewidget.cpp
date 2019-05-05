#include <QMenu>
#include <vector>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QWidgetAction>
#include <QRadialGradient>
#include <QBrush>
#include <QGuiApplication>
#include <QComboBox>
#include "parttreewidget.h"
#include "partwidget.h"
#include "skeletongraphicswidget.h"
#include "floatnumberwidget.h"

PartTreeWidget::PartTreeWidget(const Document *document, QWidget *parent) :
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
    //m_normalFont.setPixelSize(9);
    m_normalFont.setBold(false);
    
    m_selectedFont.setWeight(QFont::Light);
    //m_selectedFont.setPixelSize(9);
    m_selectedFont.setBold(true);
    
    setFont(m_normalFont);
    
    QRadialGradient gradient(QPointF(0.115, 0.3), 0.3);
    QColor fillColor = QColor(0xfb, 0xf9, 0x87);
    fillColor.setAlphaF(0.85);
    gradient.setCoordinateMode(QGradient::StretchToDeviceMode);
    gradient.setColorAt(0, fillColor);
    gradient.setColorAt(1, Qt::transparent);
    m_hightlightedPartBackground = QBrush(gradient);
    
    connect(this, &QTreeWidget::customContextMenuRequested, this, &PartTreeWidget::showContextMenu);
    connect(this, &QTreeWidget::itemChanged, this, &PartTreeWidget::groupChanged);
    connect(this, &QTreeWidget::itemExpanded, this, &PartTreeWidget::groupExpanded);
    connect(this, &QTreeWidget::itemCollapsed, this, &PartTreeWidget::groupCollapsed);
}

void PartTreeWidget::selectComponent(QUuid componentId, bool multiple)
{
    if (multiple) {
        if (!m_currentSelectedComponentId.isNull()) {
            m_selectedComponentIds.insert(m_currentSelectedComponentId);
            m_currentSelectedComponentId = QUuid();
            emit currentComponentChanged(m_currentSelectedComponentId);
        }
        if (m_selectedComponentIds.find(componentId) != m_selectedComponentIds.end()) {
            updateComponentSelectState(componentId, false);
            m_selectedComponentIds.erase(componentId);
        } else {
            updateComponentSelectState(componentId, true);
            m_selectedComponentIds.insert(componentId);
        }
        if (m_selectedComponentIds.size() > 1) {
            return;
        }
        if (m_selectedComponentIds.size() == 1)
            componentId = *m_selectedComponentIds.begin();
        else
            componentId = QUuid();
    }
    if (!m_selectedComponentIds.empty()) {
        for (const auto &id: m_selectedComponentIds) {
            updateComponentSelectState(id, false);
        }
        m_selectedComponentIds.clear();
    }
    if (m_currentSelectedComponentId != componentId) {
        if (!m_currentSelectedComponentId.isNull()) {
            updateComponentSelectState(m_currentSelectedComponentId, false);
        }
        m_currentSelectedComponentId = componentId;
        if (!m_currentSelectedComponentId.isNull()) {
            updateComponentSelectState(m_currentSelectedComponentId, true);
        }
        emit currentComponentChanged(m_currentSelectedComponentId);
    }
}

void PartTreeWidget::updateComponentAppearance(QUuid componentId)
{
    updateComponentSelectState(componentId, isComponentSelected(componentId));
}

void PartTreeWidget::updateComponentSelectState(QUuid componentId, bool selected)
{
    const Component *component = m_document->findComponent(componentId);
    if (nullptr == component) {
        qDebug() << "Component not found:" << componentId;
        return;
    }
    if (!component->linkToPartId.isNull()) {
        auto item = m_partItemMap.find(component->linkToPartId);
        if (item != m_componentItemMap.end()) {
            PartWidget *widget = (PartWidget *)itemWidget(item->second, 0);
            // Unnormal state updating call should be called before check state updating call
            widget->updateUnnormalState(component->combineMode != CombineMode::Normal);
            widget->updateCheckedState(selected);
        }
        return;
    }
    auto item = m_componentItemMap.find(componentId);
    if (item != m_componentItemMap.end()) {
        item->second->setFont(0, selected ? m_selectedFont : m_normalFont);
        if (component->combineMode != CombineMode::Normal)
            item->second->setForeground(0, selected ? QBrush(Theme::blue) : QBrush(Theme::blue));
        else
            item->second->setForeground(0, selected ? QBrush(Theme::red) : QBrush(Theme::white));
    }
}

void PartTreeWidget::mousePressEvent(QMouseEvent *event)
{
    QModelIndex itemIndex = indexAt(event->pos());
    QTreeView::mousePressEvent(event);
    if (event->button() == Qt::LeftButton) {
        bool multiple = QGuiApplication::queryKeyboardModifiers().testFlag(Qt::ControlModifier);
        if (itemIndex.isValid()) {
            QTreeWidgetItem *item = itemFromIndex(itemIndex);
            auto componentId = QUuid(item->data(0, Qt::UserRole).toString());
            if (QGuiApplication::queryKeyboardModifiers().testFlag(Qt::ShiftModifier)) {
                if (!m_shiftStartComponentId.isNull()) {
                    const Component *parent = m_document->findComponentParent(m_shiftStartComponentId);
                    if (parent) {
                        if (!parent->childrenIds.empty()) {
                            bool startAdd = false;
                            bool stopAdd = false;
                            std::vector<QUuid> waitQueue;
                            for (const auto &childId: parent->childrenIds) {
                                if (m_shiftStartComponentId == childId || componentId == childId) {
                                    if (startAdd) {
                                        stopAdd = true;
                                    } else {
                                        startAdd = true;
                                    }
                                }
                                if (startAdd)
                                    waitQueue.push_back(childId);
                                if (stopAdd)
                                    break;
                            }
                            if (stopAdd && !waitQueue.empty()) {
                                if (!m_selectedComponentIds.empty()) {
                                    for (const auto &id: m_selectedComponentIds) {
                                        updateComponentSelectState(id, false);
                                    }
                                    m_selectedComponentIds.clear();
                                }
                                if (!m_currentSelectedComponentId.isNull()) {
                                    m_currentSelectedComponentId = QUuid();
                                    emit currentComponentChanged(m_currentSelectedComponentId);
                                }
                                for (const auto &waitId: waitQueue) {
                                    selectComponent(waitId, true);
                                }
                            }
                        }
                    }
                }
                return;
            } else {
                m_shiftStartComponentId = componentId;
            }
            selectComponent(componentId, multiple);
            return;
        }
        if (!multiple)
            selectComponent(QUuid());
    }
}

void PartTreeWidget::showContextMenu(const QPoint &pos)
{
    const Component *component = nullptr;
    const SkeletonPart *part = nullptr;
    PartWidget *partWidget = nullptr;
    
    std::set<QUuid> unorderedComponentIds = m_selectedComponentIds;
    if (!m_currentSelectedComponentId.isNull())
        unorderedComponentIds.insert(m_currentSelectedComponentId);
    
    if (unorderedComponentIds.empty()) {
        QTreeWidgetItem *item = itemAt(pos);
        if (nullptr != item) {
            QUuid componentId = QUuid(item->data(0, Qt::UserRole).toString());
            unorderedComponentIds.insert(componentId);
        }
    }
    
    std::vector<QUuid> componentIds;
    std::vector<QUuid> candidates;
    m_document->collectComponentDescendantComponents(QUuid(), candidates);
    for (const auto &cand: candidates) {
        if (unorderedComponentIds.find(cand) != unorderedComponentIds.end())
            componentIds.push_back(cand);
    }
    
    QMenu contextMenu(this);
    
    if (componentIds.size() == 1) {
        QUuid componentId = *componentIds.begin();
        component = m_document->findComponent(componentId);
        if (nullptr == component) {
            qDebug() << "Component not found:" << componentId;
            return;
        }
        if (component && !component->linkToPartId.isNull()) {
            part = m_document->findPart(component->linkToPartId);
        }
    }
    
    QHBoxLayout *layout = new QHBoxLayout;
    QWidgetAction forDisplayPartImage(this);
    layout->setAlignment(Qt::AlignCenter);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    if (nullptr != part) {
        auto findItem = m_partItemMap.find(part->id);
        if (findItem != m_partItemMap.end()) {
            partWidget = (PartWidget *)itemWidget(findItem->second, 0);
        }
    }
    if (nullptr != part && nullptr != partWidget) {
        ModelWidget *previewWidget = new ModelWidget;
        previewWidget->enableMove(false);
        previewWidget->enableZoom(false);
        previewWidget->setFixedSize(Theme::partPreviewImageSize, Theme::partPreviewImageSize);
        previewWidget->setXRotation(partWidget->previewWidget()->xRot());
        previewWidget->setYRotation(partWidget->previewWidget()->yRot());
        previewWidget->setZRotation(partWidget->previewWidget()->zRot());
        previewWidget->updateMesh(part->takePreviewMesh());
        layout->addWidget(previewWidget);
    } else {
        QLabel *previewLabel = new QLabel;
        previewLabel->setFixedHeight(Theme::partPreviewImageSize);
        previewLabel->setStyleSheet("QLabel {color: " + (component && (component->combineMode != CombineMode::Normal) ? Theme::blue.name() : Theme::red.name()) + "}");
        if (nullptr != component) {
            previewLabel->setText(component->name);
        } else if (!componentIds.empty()) {
            previewLabel->setText(tr("(%1 items)").arg(QString::number(componentIds.size())));
        }
        layout->addWidget(previewLabel);
    }
    QWidget *widget = new QWidget;
    if (nullptr != component) {
        QComboBox *combineModeSelectBox = new QComboBox;
        for (size_t i = 0; i < (size_t)CombineMode::Count; ++i) {
            CombineMode mode = (CombineMode)i;
            combineModeSelectBox->addItem(CombineModeToDispName(mode));
        }
        combineModeSelectBox->setCurrentIndex((int)component->combineMode);
        
        connect(combineModeSelectBox, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, [=](int index) {
            emit setComponentCombineMode(component->id, (CombineMode)index);
        });
        
        QHBoxLayout *combineModeLayout = new QHBoxLayout;
        combineModeLayout->setAlignment(Qt::AlignCenter);
        combineModeLayout->setContentsMargins(0, 0, 0, 0);
        combineModeLayout->setSpacing(0);
        combineModeLayout->addWidget(combineModeSelectBox);
    
        QVBoxLayout *newLayout = new QVBoxLayout;
        newLayout->addLayout(layout);
        newLayout->addLayout(combineModeLayout);
        widget->setLayout(newLayout);
    } else {
        widget->setLayout(layout);
    }
    forDisplayPartImage.setDefaultWidget(widget);
    if (!componentIds.empty()) {
        contextMenu.addAction(&forDisplayPartImage);
        contextMenu.addSeparator();
    }
    
    QAction showAction(tr("Show"), this);
    showAction.setIcon(Theme::awesome()->icon(fa::eye));
    QAction hideAction(tr("Hide"), this);
    hideAction.setIcon(Theme::awesome()->icon(fa::eyeslash));
    QAction lockAction(tr("Lock"), this);
    lockAction.setIcon(Theme::awesome()->icon(fa::lock));
    QAction unlockAction(tr("Unlock"), this);
    unlockAction.setIcon(Theme::awesome()->icon(fa::unlock));
    QAction selectAction(tr("Select"), this);
    
    if (nullptr != component && nullptr != part) {
        connect(&selectAction, &QAction::triggered, [=]() {
            emit addPartToSelection(component->linkToPartId);
        });
        contextMenu.addAction(&selectAction);
        
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
    } else if (!componentIds.empty()) {
        connect(&selectAction, &QAction::triggered, [=]() {
            for (const auto &componentId: componentIds) {
                std::vector<QUuid> partIds;
                m_document->collectComponentDescendantParts(componentId, partIds);
                for (const auto &partId: partIds) {
                    emit addPartToSelection(partId);
                }
            }
        });
        contextMenu.addAction(&selectAction);

        connect(&showAction, &QAction::triggered, [=]() {
            for (const auto &componentId: componentIds)
                emit showDescendantComponents(componentId);
        });
        contextMenu.addAction(&showAction);

        connect(&hideAction, &QAction::triggered, [=]() {
            for (const auto &componentId: componentIds)
                emit hideDescendantComponents(componentId);
        });
        contextMenu.addAction(&hideAction);

        connect(&lockAction, &QAction::triggered, [=]() {
            for (const auto &componentId: componentIds)
                emit lockDescendantComponents(componentId);
        });
        contextMenu.addAction(&lockAction);

        connect(&unlockAction, &QAction::triggered, [=]() {
            for (const auto &componentId: componentIds)
                emit unlockDescendantComponents(componentId);
        });
        contextMenu.addAction(&unlockAction);
    }
    
    /*
    if (component && !component->inverse) {
        connect(&invertAction, &QAction::triggered, [=]() {
            emit setComponentInverseState(component->id, true);
        });
        contextMenu.addAction(&invertAction);
    }

    if (component && component->inverse) {
        connect(&cancelInverseAction, &QAction::triggered, [=]() {
            emit setComponentInverseState(component->id, false);
        });
        contextMenu.addAction(&cancelInverseAction);
    }
    */
    
    contextMenu.addSeparator();
    
    /*
    QWidgetAction smoothAction(this);
    QAction hideOthersAction(tr("Hide Others"), this);
    QAction lockOthersAction(tr("Lock Others"), this);
    if (nullptr != component) {
        QMenu *smoothMenu = contextMenu.addMenu(tr("Smooth"));
        
        smoothAction.setDefaultWidget(createSmoothMenuWidget(component->id));
        smoothMenu->addAction(&smoothAction);
        
        contextMenu.addSeparator();
    
        hideOthersAction.setIcon(Theme::awesome()->icon(fa::eyeslash));
        connect(&hideOthersAction, &QAction::triggered, [=]() {
            emit hideOtherComponents(component->id);
        });
        contextMenu.addAction(&hideOthersAction);
        
        lockOthersAction.setIcon(Theme::awesome()->icon(fa::lock));
        connect(&lockOthersAction, &QAction::triggered, [=]() {
            emit lockOtherComponents(component->id);
        });
        contextMenu.addAction(&lockOthersAction);
        
        contextMenu.addSeparator();
    }
    */
    
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
    
    QAction newGroupAction(tr("New Group"), this);
    newGroupAction.setIcon(Theme::awesome()->icon(fa::file));
    connect(&newGroupAction, &QAction::triggered, [=]() {
        emit createNewChildComponent(nullptr == component ? QUuid() : component->id);
    });
    contextMenu.addAction(&newGroupAction);
    
    contextMenu.addSeparator();
    
    std::vector<QAction *> groupsActions;
    QAction deleteAction(tr("Delete"), this);
    QAction moveToTopAction(tr("Top"), this);
    QAction moveUpAction(tr("Up"), this);
    QAction moveDownAction(tr("Down"), this);
    QAction moveToBottomAction(tr("Bottom"), this);
    QAction moveToNewGroupAction(tr("New Group"), this);
    QAction moveToRootGroupAction(tr("Root"), this);
    std::function<void(QUuid, int)> addChildGroupsFunc;
    if (!componentIds.empty()) {
        QMenu *moveToMenu = contextMenu.addMenu(tr("Move To"));
        
        if (nullptr != component) {
            moveToTopAction.setIcon(Theme::awesome()->icon(fa::angledoubleup));
            connect(&moveToTopAction, &QAction::triggered, [=]() {
                emit moveComponentToTop(component->id);
            });
            moveToMenu->addAction(&moveToTopAction);
            
            moveUpAction.setIcon(Theme::awesome()->icon(fa::angleup));
            connect(&moveUpAction, &QAction::triggered, [=]() {
                emit moveComponentUp(component->id);
            });
            moveToMenu->addAction(&moveUpAction);
            
            moveDownAction.setIcon(Theme::awesome()->icon(fa::angledown));
            connect(&moveDownAction, &QAction::triggered, [=]() {
                emit moveComponentDown(component->id);
            });
            moveToMenu->addAction(&moveDownAction);
            
            moveToBottomAction.setIcon(Theme::awesome()->icon(fa::angledoubledown));
            connect(&moveToBottomAction, &QAction::triggered, [=]() {
                emit moveComponentToBottom(component->id);
            });
            moveToMenu->addAction(&moveToBottomAction);
            
            moveToMenu->addSeparator();
            
            moveToNewGroupAction.setIcon(Theme::awesome()->icon(fa::file));
            moveToMenu->addAction(&moveToNewGroupAction);
            connect(&moveToNewGroupAction, &QAction::triggered, [=]() {
                emit createNewComponentAndMoveThisIn(component->id);
            });
            
            moveToMenu->addSeparator();
        }
        
        moveToMenu->addAction(&moveToRootGroupAction);
        connect(&moveToRootGroupAction, &QAction::triggered, [=]() {
            for (const auto &componentId: componentIds)
                emit moveComponent(componentId, QUuid());
        });
        
        addChildGroupsFunc = [this, &groupsActions, &addChildGroupsFunc, &moveToMenu, &componentIds](QUuid currentId, int tabs) -> void {
            const Component *current = m_document->findComponent(currentId);
            if (nullptr == current)
                return;
            if (!current->id.isNull() && current->linkDataType().isEmpty()) {
                QAction *action = new QAction(QString(" ").repeated(tabs * 4) + current->name, this);
                connect(action, &QAction::triggered, [=]() {
                    for (const auto &componentId: componentIds)
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
        
        deleteAction.setIcon(Theme::awesome()->icon(fa::trash));
        connect(&deleteAction, &QAction::triggered, [=]() {
            for (const auto &componentId: componentIds)
                emit removeComponent(componentId);
        });
        contextMenu.addAction(&deleteAction);
    }
    
    contextMenu.exec(mapToGlobal(pos));
    
    for (const auto &action: groupsActions) {
        delete action;
    }
}

QWidget *PartTreeWidget::createSmoothMenuWidget(QUuid componentId)
{
    QWidget *popup = new QWidget;
    
    const Component *component = m_document->findComponent(componentId);
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

QTreeWidgetItem *PartTreeWidget::findComponentItem(QUuid componentId)
{
    auto findResult = m_componentItemMap.find(componentId);
    if (findResult == m_componentItemMap.end())
        return nullptr;
    
    return findResult->second;
}

void PartTreeWidget::componentNameChanged(QUuid componentId)
{
    auto componentItem = m_componentItemMap.find(componentId);
    if (componentItem == m_componentItemMap.end()) {
        qDebug() << "Find component item failed:" << componentId;
        return;
    }
    
    const Component *component = m_document->findComponent(componentId);
    if (nullptr == component) {
        qDebug() << "Find component failed:" << componentId;
        return;
    }
    
    componentItem->second->setText(0, component->name);
}

void PartTreeWidget::componentExpandStateChanged(QUuid componentId)
{
    auto componentItem = m_componentItemMap.find(componentId);
    if (componentItem == m_componentItemMap.end()) {
        qDebug() << "Find component item failed:" << componentId;
        return;
    }
    
    const Component *component = m_document->findComponent(componentId);
    if (nullptr == component) {
        qDebug() << "Find component failed:" << componentId;
        return;
    }
    
    componentItem->second->setExpanded(component->expanded);
}

void PartTreeWidget::componentCombineModeChanged(QUuid componentId)
{
    updateComponentAppearance(componentId);
}

void PartTreeWidget::addComponentChildrenToItem(QUuid componentId, QTreeWidgetItem *parentItem)
{
    const Component *parentComponent = m_document->findComponent(componentId);
    if (nullptr == parentComponent)
        return;
    
    for (const auto &childId: parentComponent->childrenIds) {
        const Component *component = m_document->findComponent(childId);
        if (nullptr == component)
            continue;
        if (!component->linkToPartId.isNull()) {
            QTreeWidgetItem *item = new QTreeWidgetItem();
            parentItem->addChild(item);
            item->setData(0, Qt::UserRole, QVariant(component->id.toString()));
            item->setFlags(item->flags() & ~(Qt::ItemIsEditable | Qt::ItemIsEnabled | Qt::ItemIsSelectable));
            QUuid partId = component->linkToPartId;
            PartWidget *widget = new PartWidget(m_document, partId);
            item->setSizeHint(0, PartWidget::preferredSize());
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
        updateComponentSelectState(childId, isComponentSelected(childId));
    }
}

void PartTreeWidget::deleteItemChildren(QTreeWidgetItem *item)
{
    auto children = item->takeChildren();
    while (!children.isEmpty()) {
        auto first = children.takeFirst();
        auto componentId = QUuid(first->data(0, Qt::UserRole).toString());
        const Component *component = m_document->findComponent(componentId);
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

void PartTreeWidget::componentChildrenChanged(QUuid componentId)
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

void PartTreeWidget::removeAllContent()
{
    qDeleteAll(invisibleRootItem()->takeChildren());
    m_partItemMap.clear();
    m_componentItemMap.clear();
    m_componentItemMap[QUuid()] = invisibleRootItem();
}

void PartTreeWidget::componentRemoved(QUuid componentId)
{
    auto componentItem = m_componentItemMap.find(componentId);
    if (componentItem == m_componentItemMap.end())
        return;
    m_selectedComponentIds.erase(componentId);
    if (m_currentSelectedComponentId == componentId)
        m_currentSelectedComponentId = QUuid();
    m_componentItemMap.erase(componentId);
}

void PartTreeWidget::componentAdded(QUuid componentId)
{
    // ignore
}

void PartTreeWidget::partRemoved(QUuid partId)
{
    auto partItem = m_partItemMap.find(partId);
    if (partItem == m_partItemMap.end())
        return;
    
    m_partItemMap.erase(partItem);
}

void PartTreeWidget::groupChanged(QTreeWidgetItem *item, int column)
{
    if (0 != column)
        return;
    
    auto componentId = QUuid(item->data(0, Qt::UserRole).toString());
    
    const Component *component = m_document->findComponent(componentId);
    if (nullptr == component) {
        qDebug() << "Find component failed:" << componentId;
        return;
    }
    
    if (item->text(0) != component->name)
        emit renameComponent(componentId, item->text(0));
}

void PartTreeWidget::groupExpanded(QTreeWidgetItem *item)
{
    QUuid componentId = QUuid(item->data(0, Qt::UserRole).toString());
    emit setComponentExpandState(componentId, true);
}

void PartTreeWidget::groupCollapsed(QTreeWidgetItem *item)
{
    QUuid componentId = QUuid(item->data(0, Qt::UserRole).toString());
    emit setComponentExpandState(componentId, false);
}

void PartTreeWidget::partPreviewChanged(QUuid partId)
{
    auto item = m_partItemMap.find(partId);
    if (item == m_partItemMap.end()) {
        qDebug() << "Part item not found:" << partId;
        return;
    }
    PartWidget *widget = (PartWidget *)itemWidget(item->second, 0);
    widget->updatePreview();
}

void PartTreeWidget::partLockStateChanged(QUuid partId)
{
    auto item = m_partItemMap.find(partId);
    if (item == m_partItemMap.end()) {
        qDebug() << "Part item not found:" << partId;
        return;
    }
    PartWidget *widget = (PartWidget *)itemWidget(item->second, 0);
    widget->updateLockButton();
}

void PartTreeWidget::partVisibleStateChanged(QUuid partId)
{
    auto item = m_partItemMap.find(partId);
    if (item == m_partItemMap.end()) {
        qDebug() << "Part item not found:" << partId;
        return;
    }
    PartWidget *widget = (PartWidget *)itemWidget(item->second, 0);
    widget->updateVisibleButton();
}

void PartTreeWidget::partSubdivStateChanged(QUuid partId)
{
    auto item = m_partItemMap.find(partId);
    if (item == m_partItemMap.end()) {
        qDebug() << "Part item not found:" << partId;
        return;
    }
    PartWidget *widget = (PartWidget *)itemWidget(item->second, 0);
    widget->updateSubdivButton();
}

void PartTreeWidget::partDisableStateChanged(QUuid partId)
{
    auto item = m_partItemMap.find(partId);
    if (item == m_partItemMap.end()) {
        qDebug() << "Part item not found:" << partId;
        return;
    }
    PartWidget *widget = (PartWidget *)itemWidget(item->second, 0);
    widget->updateDisableButton();
}

void PartTreeWidget::partXmirrorStateChanged(QUuid partId)
{
    auto item = m_partItemMap.find(partId);
    if (item == m_partItemMap.end()) {
        qDebug() << "Part item not found:" << partId;
        return;
    }
    PartWidget *widget = (PartWidget *)itemWidget(item->second, 0);
    widget->updateXmirrorButton();
}

void PartTreeWidget::partDeformChanged(QUuid partId)
{
    auto item = m_partItemMap.find(partId);
    if (item == m_partItemMap.end()) {
        qDebug() << "Part item not found:" << partId;
        return;
    }
    PartWidget *widget = (PartWidget *)itemWidget(item->second, 0);
    widget->updateDeformButton();
}

void PartTreeWidget::partRoundStateChanged(QUuid partId)
{
    auto item = m_partItemMap.find(partId);
    if (item == m_partItemMap.end()) {
        qDebug() << "Part item not found:" << partId;
        return;
    }
    PartWidget *widget = (PartWidget *)itemWidget(item->second, 0);
    widget->updateRoundButton();
}

void PartTreeWidget::partChamferStateChanged(QUuid partId)
{
    auto item = m_partItemMap.find(partId);
    if (item == m_partItemMap.end()) {
        qDebug() << "Part item not found:" << partId;
        return;
    }
    PartWidget *widget = (PartWidget *)itemWidget(item->second, 0);
    widget->updateChamferButton();
}

void PartTreeWidget::partColorStateChanged(QUuid partId)
{
    auto item = m_partItemMap.find(partId);
    if (item == m_partItemMap.end()) {
        qDebug() << "Part item not found:" << partId;
        return;
    }
    PartWidget *widget = (PartWidget *)itemWidget(item->second, 0);
    widget->updateColorButton();
}

void PartTreeWidget::partCutRotationChanged(QUuid partId)
{
    auto item = m_partItemMap.find(partId);
    if (item == m_partItemMap.end()) {
        qDebug() << "Part item not found:" << partId;
        return;
    }
    PartWidget *widget = (PartWidget *)itemWidget(item->second, 0);
    widget->updateCutRotationButton();
}

void PartTreeWidget::partCutFaceChanged(QUuid partId)
{
    auto item = m_partItemMap.find(partId);
    if (item == m_partItemMap.end()) {
        qDebug() << "Part item not found:" << partId;
        return;
    }
    PartWidget *widget = (PartWidget *)itemWidget(item->second, 0);
    widget->updateCutRotationButton();
}

void PartTreeWidget::partMaterialIdChanged(QUuid partId)
{
    auto item = m_partItemMap.find(partId);
    if (item == m_partItemMap.end()) {
        qDebug() << "Part item not found:" << partId;
        return;
    }
    PartWidget *widget = (PartWidget *)itemWidget(item->second, 0);
    widget->updateColorButton();
}

void PartTreeWidget::partChecked(QUuid partId)
{
    auto item = m_partItemMap.find(partId);
    if (item == m_partItemMap.end()) {
        qDebug() << "Part item not found:" << partId;
        return;
    }
    item->second->setBackground(0, m_hightlightedPartBackground);
}

void PartTreeWidget::partUnchecked(QUuid partId)
{
    auto item = m_partItemMap.find(partId);
    if (item == m_partItemMap.end()) {
        //qDebug() << "Part item not found:" << partId;
        return;
    }
    item->second->setBackground(0, QBrush(Qt::transparent));
}

QSize PartTreeWidget::sizeHint() const
{
    return QSize(Theme::sidebarPreferredWidth, 0);
}

bool PartTreeWidget::isComponentSelected(QUuid componentId)
{
    return (m_currentSelectedComponentId == componentId ||
        m_selectedComponentIds.find(componentId) != m_selectedComponentIds.end());
}
