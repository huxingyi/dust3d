#include <QMenu>
#include <vector>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QWidgetAction>
#include <QRadialGradient>
#include <QBrush>
#include <QGuiApplication>
#include <QComboBox>
#include <QFormLayout>
#include <QClipboard>
#include <QMimeData>
#include <QApplication>
#include "part_tree_widget.h"
#include "part_widget.h"
#include "skeleton_graphics_widget.h"
#include "float_number_widget.h"
#include "int_number_widget.h"
#include "document.h"

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
    
    QRadialGradient gradient(QPointF(0.215, 0.3), 0.3);
    QColor fillColor = QColor(0xfb, 0xf9, 0x87);
    fillColor.setAlphaF(0.85);
    gradient.setCoordinateMode(QGradient::StretchToDeviceMode);
    gradient.setColorAt(0, fillColor);
    gradient.setColorAt(1, Qt::transparent);
    m_hightlightedPartBackground = QBrush(gradient);
    
    QTreeWidgetItem *item = new QTreeWidgetItem(QStringList(tr("Root")));
    invisibleRootItem()->addChild(item);
    item->setData(0, Qt::UserRole, QVariant(QString(dust3d::Uuid().toString().c_str())));
    item->setChildIndicatorPolicy(QTreeWidgetItem::ShowIndicator);
    item->setExpanded(true);
    item->setFlags((item->flags() | Qt::ItemIsEnabled) & ~(Qt::ItemIsSelectable));
    m_rootItem = item;
    m_componentItemMap[dust3d::Uuid()] = item;
    m_firstSelect = true;
    selectComponent(dust3d::Uuid());
    
    connect(this, &QTreeWidget::customContextMenuRequested, this, [&](const QPoint &pos) {
        showContextMenu(pos);
    });
    connect(this, &QTreeWidget::itemChanged, this, &PartTreeWidget::groupChanged);
    connect(this, &QTreeWidget::itemExpanded, this, &PartTreeWidget::groupExpanded);
    connect(this, &QTreeWidget::itemCollapsed, this, &PartTreeWidget::groupCollapsed);
}

void PartTreeWidget::selectComponent(dust3d::Uuid componentId, bool multiple)
{
    if (m_firstSelect) {
        m_firstSelect = false;
        updateComponentSelectState(dust3d::Uuid(), true);
        return;
    }
    if (multiple) {
        if (!m_currentSelectedComponentId.isNull()) {
            m_selectedComponentIds.insert(m_currentSelectedComponentId);
            m_currentSelectedComponentId = dust3d::Uuid();
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
            componentId = dust3d::Uuid();
    }
    if (!m_selectedComponentIds.empty()) {
        for (const auto &id: m_selectedComponentIds) {
            updateComponentSelectState(id, false);
        }
        m_selectedComponentIds.clear();
    }
    if (m_currentSelectedComponentId != componentId) {
        //if (!m_currentSelectedComponentId.isNull()) {
            updateComponentSelectState(m_currentSelectedComponentId, false);
        //}
        m_currentSelectedComponentId = componentId;
        //if (!m_currentSelectedComponentId.isNull()) {
            updateComponentSelectState(m_currentSelectedComponentId, true);
        //}
        emit currentComponentChanged(m_currentSelectedComponentId);
    }
}

void PartTreeWidget::updateComponentAppearance(dust3d::Uuid componentId)
{
    updateComponentSelectState(componentId, isComponentSelected(componentId));
}

void PartTreeWidget::updateComponentSelectState(dust3d::Uuid componentId, bool selected)
{
    const SkeletonComponent *component = m_document->findComponent(componentId);
    if (nullptr == component) {
        qDebug() << "SkeletonComponent not found:" << componentId;
        return;
    }
    if (!component->linkToPartId.isNull()) {
        auto item = m_partItemMap.find(component->linkToPartId);
        if (item != m_partItemMap.end()) {
            PartWidget *widget = (PartWidget *)itemWidget(item->second, 0);
            // Unnormal state updating call should be called before check state updating call
            widget->updateUnnormalState(component->combineMode != dust3d::CombineMode::Normal);
            widget->updateCheckedState(selected);
        }
        return;
    }
    auto item = m_componentItemMap.find(componentId);
    if (item != m_componentItemMap.end()) {
        item->second->setFont(0, selected ? m_selectedFont : m_normalFont);
        if (component->combineMode != dust3d::CombineMode::Normal)
            item->second->setForeground(0, selected ? QBrush(Theme::blue) : QBrush(Theme::blue));
        else
            item->second->setForeground(0, selected ? QBrush(Theme::red) : QBrush(Theme::white));
    }
}

void PartTreeWidget::mouseDoubleClickEvent(QMouseEvent *event)
{
    QWidget::mouseDoubleClickEvent(event);
    auto componentIds = collectSelectedComponentIds(event->pos());
    for (const auto &componentId: componentIds) {
        std::vector<dust3d::Uuid> partIds;
        m_document->collectComponentDescendantParts(componentId, partIds);
        for (const auto &partId: partIds) {
            emit addPartToSelection(partId);
        }
    }
    event->accept();
}

void PartTreeWidget::handleSingleClick(const QPoint &pos)
{
    QModelIndex itemIndex = indexAt(pos);

    bool multiple = QGuiApplication::queryKeyboardModifiers().testFlag(Qt::ControlModifier);
    if (itemIndex.isValid()) {
        QTreeWidgetItem *item = itemFromIndex(itemIndex);
        auto componentId = dust3d::Uuid(item->data(0, Qt::UserRole).toString().toUtf8().constData());
        if (QGuiApplication::queryKeyboardModifiers().testFlag(Qt::ShiftModifier)) {
            if (!m_shiftStartComponentId.isNull()) {
                const SkeletonComponent *parent = m_document->findComponentParent(m_shiftStartComponentId);
                if (parent) {
                    if (!parent->childrenIds.empty()) {
                        bool startAdd = false;
                        bool stopAdd = false;
                        std::vector<dust3d::Uuid> waitQueue;
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
                                m_currentSelectedComponentId = dust3d::Uuid();
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
        selectComponent(dust3d::Uuid());
}

void PartTreeWidget::mousePressEvent(QMouseEvent *event)
{
    QTreeView::mousePressEvent(event);
    
    if (event->button() != Qt::LeftButton)
        return;
    
    handleSingleClick(event->pos());
}

std::vector<dust3d::Uuid> PartTreeWidget::collectSelectedComponentIds(const QPoint &pos)
{
    std::set<dust3d::Uuid> unorderedComponentIds = m_selectedComponentIds;
    if (!m_currentSelectedComponentId.isNull())
        unorderedComponentIds.insert(m_currentSelectedComponentId);
    
    if (unorderedComponentIds.empty()) {
        QTreeWidgetItem *item = itemAt(pos);
        if (nullptr != item) {
            dust3d::Uuid componentId = dust3d::Uuid(item->data(0, Qt::UserRole).toString().toUtf8().constData());
            unorderedComponentIds.insert(componentId);
        }
    }
    
    std::vector<dust3d::Uuid> componentIds;
    std::vector<dust3d::Uuid> candidates;
    m_document->collectComponentDescendantComponents(dust3d::Uuid(), candidates);
    for (const auto &cand: candidates) {
        if (unorderedComponentIds.find(cand) != unorderedComponentIds.end())
            componentIds.push_back(cand);
    }
    
    return componentIds;
}

void PartTreeWidget::showContextMenu(const QPoint &pos, bool shorted)
{
    const SkeletonComponent *component = nullptr;
    const SkeletonPart *part = nullptr;
    PartWidget *partWidget = nullptr;
    
    std::vector<dust3d::Uuid> componentIds = collectSelectedComponentIds(pos);
    
    if (shorted) {
        if (componentIds.size() > 1)
            return;
    }
    
    QMenu contextMenu(this);
    
    if (componentIds.size() == 1) {
        dust3d::Uuid componentId = *componentIds.begin();
        component = m_document->findComponent(componentId);
        if (nullptr == component) {
            qDebug() << "SkeletonComponent not found:" << componentId;
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
        QLabel *previewLabel = new QLabel;
        previewLabel->setFixedSize(Theme::partPreviewImageSize, Theme::partPreviewImageSize);
        previewLabel->setPixmap(part->previewPixmap);
        layout->addWidget(previewLabel);
    } else {
        QLabel *previewLabel = new QLabel;
        previewLabel->setFixedHeight(Theme::partPreviewImageSize);
        previewLabel->setStyleSheet("QLabel {color: " + (component && (component->combineMode != dust3d::CombineMode::Normal) ? Theme::blue.name() : Theme::red.name()) + "}");
        if (nullptr != component) {
            previewLabel->setText(component->name);
        } else if (!componentIds.empty()) {
            previewLabel->setText(tr("(%1 items)").arg(QString::number(componentIds.size())));
        } else {
            previewLabel->hide();
        }
        layout->addWidget(previewLabel);
    }
    
    QComboBox *combineModeSelectBox = nullptr;
    QComboBox *partTargetSelectBox = nullptr;
    QComboBox *partBaseSelectBox = nullptr;
    
    if (nullptr != component) {
        
        if (nullptr == part || part->hasCombineModeFunction()) {
            combineModeSelectBox = new QComboBox;
            for (size_t i = 0; i < (size_t)dust3d::CombineMode::Count; ++i) {
                dust3d::CombineMode mode = (dust3d::CombineMode)i;
                combineModeSelectBox->addItem(tr(CombineModeToDispName(mode).c_str()));
            }
            combineModeSelectBox->setCurrentIndex((int)component->combineMode);
            connect(combineModeSelectBox, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, [=](int index) {
                emit setComponentCombineMode(component->id, (dust3d::CombineMode)index);
                emit groupOperationAdded();
            });
        }

        if (nullptr != part && nullptr != partWidget) {
            partTargetSelectBox = new QComboBox;
            for (size_t i = 0; i < (size_t)dust3d::PartTarget::Count; ++i) {
                dust3d::PartTarget target = (dust3d::PartTarget)i;
                partTargetSelectBox->addItem(tr(PartTargetToDispName(target).c_str()));
            }
            partTargetSelectBox->setCurrentIndex((int)part->target);
            connect(partTargetSelectBox, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, [=](int index) {
                emit setPartTarget(part->id, (dust3d::PartTarget)index);
                emit groupOperationAdded();
            });
        }

        if (nullptr != part && part->hasBaseFunction() && nullptr != partWidget) {
            partBaseSelectBox = new QComboBox;
            for (size_t i = 0; i < (size_t)dust3d::PartBase::Count; ++i) {
                dust3d::PartBase base = (dust3d::PartBase)i;
                partBaseSelectBox->addItem(tr(PartBaseToDispName(base).c_str()));
            }
            partBaseSelectBox->setCurrentIndex((int)part->base);
            connect(partBaseSelectBox, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, [=](int index) {
                emit setPartBase(part->id, (dust3d::PartBase)index);
                emit groupOperationAdded();
            });
        }
    }
    
    if (componentIds.size() > 1) {
        
        if (nullptr == partBaseSelectBox) {
            std::set<dust3d::PartBase> partBases;
            for (const auto &componentId: componentIds) {
                const SkeletonComponent *oneComponent = m_document->findComponent(componentId);
                if (nullptr == oneComponent || oneComponent->linkToPartId.isNull())
                    continue;
                const SkeletonPart *onePart = m_document->findPart(oneComponent->linkToPartId);
                if (nullptr == onePart)
                    continue;
                partBases.insert(onePart->base);
            }
            if (!partBases.empty()) {
                int startIndex = (1 == partBases.size()) ? 0 : 1;
                partBaseSelectBox = new QComboBox;
                if (0 != startIndex)
                    partBaseSelectBox->addItem(tr("Not Change"));
                for (size_t i = 0; i < (size_t)dust3d::PartBase::Count; ++i) {
                    dust3d::PartBase base = (dust3d::PartBase)i;
                    partBaseSelectBox->addItem(tr(PartBaseToDispName(base).c_str()));
                }
                if (0 != startIndex)
                    partBaseSelectBox->setCurrentIndex(0);
                else
                    partBaseSelectBox->setCurrentIndex((int)*partBases.begin());
                connect(partBaseSelectBox, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, [=](int index) {
                    if (index < startIndex)
                        return;
                    for (const auto &componentId: componentIds) {
                        const SkeletonComponent *oneComponent = m_document->findComponent(componentId);
                        if (nullptr == oneComponent || oneComponent->linkToPartId.isNull())
                            continue;
                        emit setPartBase(oneComponent->linkToPartId, (dust3d::PartBase)(index - startIndex));
                    }
                    emit groupOperationAdded();
                });
            }
        }
        
        if (nullptr == combineModeSelectBox) {
            std::set<dust3d::CombineMode> combineModes;
            for (const auto &componentId: componentIds) {
                const SkeletonComponent *oneComponent = m_document->findComponent(componentId);
                if (nullptr == oneComponent)
                    continue;
                combineModes.insert(oneComponent->combineMode);
            }
            if (!combineModes.empty()) {
                int startIndex = (1 == combineModes.size()) ? 0 : 1;
                combineModeSelectBox = new QComboBox;
                if (0 != startIndex)
                    combineModeSelectBox->addItem(tr("Not Change"));
                for (size_t i = 0; i < (size_t)dust3d::CombineMode::Count; ++i) {
                    dust3d::CombineMode mode = (dust3d::CombineMode)i;
                    combineModeSelectBox->addItem(tr(CombineModeToDispName(mode).c_str()));
                }
                if (0 != startIndex)
                    combineModeSelectBox->setCurrentIndex(0);
                else
                    combineModeSelectBox->setCurrentIndex((int)*combineModes.begin());
                connect(combineModeSelectBox, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, [=](int index) {
                    if (index < startIndex)
                        return;
                    for (const auto &componentId: componentIds) {
                        emit setComponentCombineMode(componentId, (dust3d::CombineMode)(index - startIndex));
                    }
                    emit groupOperationAdded();
                });
            }
        }
    }
    
    QWidget *widget = new QWidget;
    QFormLayout *componentSettingsLayout = new QFormLayout;
    
    if (nullptr != partBaseSelectBox)
        componentSettingsLayout->addRow(tr("Base"), partBaseSelectBox);
    if (nullptr != partTargetSelectBox)
        componentSettingsLayout->addRow(tr("Target"), partTargetSelectBox);
    if (nullptr != combineModeSelectBox)
        componentSettingsLayout->addRow(tr("Mode"), combineModeSelectBox);
    
    QVBoxLayout *newLayout = new QVBoxLayout;
    newLayout->addLayout(layout);
    newLayout->addLayout(componentSettingsLayout);
    widget->setLayout(newLayout);
        
    forDisplayPartImage.setDefaultWidget(widget);
    //if (!componentIds.empty()) {
        contextMenu.addAction(&forDisplayPartImage);
        contextMenu.addSeparator();
    //}
    
    if (shorted) {
        auto globalPos = mapToGlobal(pos);
        globalPos.setX(globalPos.x() - contextMenu.sizeHint().width() - pos.x());
        contextMenu.exec(globalPos);
        return;
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
    QAction refreshAction(tr("Refresh"), this);
    QAction copyColorAction(tr("Copy Color"), this);
    QAction pasteColorAction(tr("Paste Color"), this);
    
    QColor colorInClipboard;
    bool hasColorInClipboard = false;
    const QClipboard *clipboard = QApplication::clipboard();
    const QMimeData *mimeData = clipboard->mimeData();
    if (mimeData->hasText()) {
        auto text = mimeData->text();
        if (text.startsWith("#")) {
            colorInClipboard.setNamedColor(text);
            hasColorInClipboard = colorInClipboard.isValid();
        }
    }
    
    connect(&refreshAction, &QAction::triggered, [=]() {
        componentChildrenChanged(dust3d::Uuid());
    });
    contextMenu.addAction(&refreshAction);
    
    if (nullptr != component && nullptr != part) {
        connect(&selectAction, &QAction::triggered, [=]() {
            emit addPartToSelection(component->linkToPartId);
        });
        contextMenu.addAction(&selectAction);
        
        connect(&copyColorAction, &QAction::triggered, [=]() {
            QClipboard *clipboard = QApplication::clipboard();
            clipboard->setText(part->color.name(QColor::HexArgb));
        });
        contextMenu.addAction(&copyColorAction);
        
        if (hasColorInClipboard) {
            connect(&pasteColorAction, &QAction::triggered, [=]() {
                emit setPartColorState(component->linkToPartId, true, colorInClipboard);
            });
            contextMenu.addAction(&pasteColorAction);
        }
        
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
                std::vector<dust3d::Uuid> partIds;
                m_document->collectComponentDescendantParts(componentId, partIds);
                for (const auto &partId: partIds) {
                    emit addPartToSelection(partId);
                }
            }
        });
        contextMenu.addAction(&selectAction);
        
        if (hasColorInClipboard) {
            connect(&pasteColorAction, &QAction::triggered, [=]() {
                for (const auto &componentId: componentIds) {
                    std::vector<dust3d::Uuid> partIds;
                    m_document->collectComponentDescendantParts(componentId, partIds);
                    for (const auto &partId: partIds) {
                        emit setPartColorState(partId, true, colorInClipboard);
                    }
                }
            });
            contextMenu.addAction(&pasteColorAction);
        }

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
    
    contextMenu.addSeparator();
    
    QAction collapseAllAction(tr("Collapse All"), this);
    connect(&collapseAllAction, &QAction::triggered, [=]() {
        m_rootItem->setExpanded(false);
        emit collapseAllComponents();
    });
    contextMenu.addAction(&collapseAllAction);
    
    QAction expandAllAction(tr("Expand All"), this);
    connect(&expandAllAction, &QAction::triggered, [=]() {
        m_rootItem->setExpanded(true);
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
        emit createNewChildComponent(nullptr == component ? dust3d::Uuid() : component->id);
    });
    contextMenu.addAction(&newGroupAction);
    
    contextMenu.addSeparator();
    
    std::vector<QAction *> groupsActions;
    QAction renameAction(tr("Rename"), this);
    QAction deleteAction(tr("Delete"), this);
    QAction moveToTopAction(tr("Top"), this);
    QAction moveUpAction(tr("Up"), this);
    QAction moveDownAction(tr("Down"), this);
    QAction moveToBottomAction(tr("Bottom"), this);
    QAction moveToNewGroupAction(tr("New Group"), this);
    QAction moveToRootGroupAction(tr("Root"), this);
    std::function<void(dust3d::Uuid, int)> addChildGroupsFunc;
    if (!componentIds.empty()) {
        QMenu *moveToMenu = contextMenu.addMenu(tr("Move To"));
        
        if (nullptr != component) {
            moveToTopAction.setIcon(Theme::awesome()->icon(fa::angledoubleup));
            connect(&moveToTopAction, &QAction::triggered, [=]() {
                emit moveComponentToTop(component->id);
                emit groupOperationAdded();
            });
            moveToMenu->addAction(&moveToTopAction);
            
            moveUpAction.setIcon(Theme::awesome()->icon(fa::angleup));
            connect(&moveUpAction, &QAction::triggered, [=]() {
                emit moveComponentUp(component->id);
                emit groupOperationAdded();
            });
            moveToMenu->addAction(&moveUpAction);
            
            moveDownAction.setIcon(Theme::awesome()->icon(fa::angledown));
            connect(&moveDownAction, &QAction::triggered, [=]() {
                emit moveComponentDown(component->id);
                emit groupOperationAdded();
            });
            moveToMenu->addAction(&moveDownAction);
            
            moveToBottomAction.setIcon(Theme::awesome()->icon(fa::angledoubledown));
            connect(&moveToBottomAction, &QAction::triggered, [=]() {
                emit moveComponentToBottom(component->id);
                emit groupOperationAdded();
            });
            moveToMenu->addAction(&moveToBottomAction);
            
            moveToMenu->addSeparator();
            
            moveToNewGroupAction.setIcon(Theme::awesome()->icon(fa::file));
            moveToMenu->addAction(&moveToNewGroupAction);
            connect(&moveToNewGroupAction, &QAction::triggered, [=]() {
                emit createNewComponentAndMoveThisIn(component->id);
                emit groupOperationAdded();
            });
            
            moveToMenu->addSeparator();
        }
        
        moveToMenu->addAction(&moveToRootGroupAction);
        connect(&moveToRootGroupAction, &QAction::triggered, [=]() {
            for (const auto &componentId: componentIds)
                emit moveComponent(componentId, dust3d::Uuid());
        });
        
        addChildGroupsFunc = [this, &groupsActions, &addChildGroupsFunc, &moveToMenu, &componentIds](dust3d::Uuid currentId, int tabs) -> void {
            const SkeletonComponent *current = m_document->findComponent(currentId);
            if (nullptr == current)
                return;
            if (!current->id.isNull() && current->linkDataType().isEmpty()) {
                QAction *action = new QAction(QString(" ").repeated(tabs * 4) + current->name, this);
                connect(action, &QAction::triggered, [=]() {
                    for (const auto &componentId: componentIds)
                        emit moveComponent(componentId, current->id);
                    emit groupOperationAdded();
                });
                groupsActions.push_back(action);
                moveToMenu->addAction(action);
            }
            for (const auto &childId: current->childrenIds) {
                addChildGroupsFunc(childId, tabs + 1);
            }
        };
        addChildGroupsFunc(dust3d::Uuid(), 0);
        
        if (nullptr != component && nullptr == part) {
            auto componentId = component->id;
            connect(&renameAction, &QAction::triggered, [=]() {
                auto findItem = m_componentItemMap.find(componentId);
                if (findItem != m_componentItemMap.end()) {
                    editItem(findItem->second);
                }
            });
            contextMenu.addAction(&renameAction);
        }
        
        contextMenu.addSeparator();
        
        deleteAction.setIcon(Theme::awesome()->icon(fa::trash));
        connect(&deleteAction, &QAction::triggered, [=]() {
            for (const auto &componentId: componentIds)
                emit removeComponent(componentId);
            emit groupOperationAdded();
        });
        contextMenu.addAction(&deleteAction);
    }
    
    auto globalPos = mapToGlobal(pos);
    globalPos.setX(globalPos.x() - contextMenu.sizeHint().width() - pos.x());
    contextMenu.exec(globalPos);
    
    for (const auto &action: groupsActions) {
        delete action;
    }
}

QTreeWidgetItem *PartTreeWidget::findComponentItem(dust3d::Uuid componentId)
{
    auto findResult = m_componentItemMap.find(componentId);
    if (findResult == m_componentItemMap.end())
        return nullptr;
    
    return findResult->second;
}

void PartTreeWidget::componentNameChanged(dust3d::Uuid componentId)
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

void PartTreeWidget::componentExpandStateChanged(dust3d::Uuid componentId)
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

void PartTreeWidget::componentCombineModeChanged(dust3d::Uuid componentId)
{
    updateComponentAppearance(componentId);
}

void PartTreeWidget::componentTargetChanged(dust3d::Uuid componentId)
{
    updateComponentAppearance(componentId);
}

void PartTreeWidget::addComponentChildrenToItem(dust3d::Uuid componentId, QTreeWidgetItem *parentItem)
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
            item->setData(0, Qt::UserRole, QVariant(QString(component->id.toString().c_str())));
            item->setFlags(item->flags() & ~(Qt::ItemIsEditable | Qt::ItemIsEnabled | Qt::ItemIsSelectable));
            dust3d::Uuid partId = component->linkToPartId;
            PartWidget *widget = new PartWidget(m_document, partId);
            item->setSizeHint(0, PartWidget::preferredSize());
            setItemWidget(item, 0, widget);
            widget->reload();
            m_partItemMap[partId] = item;
        } else {
            QTreeWidgetItem *item = new QTreeWidgetItem(QStringList(component->name));
            parentItem->addChild(item);
            item->setData(0, Qt::UserRole, QVariant(QString(component->id.toString().c_str())));
            item->setChildIndicatorPolicy(QTreeWidgetItem::ShowIndicator);
            item->setExpanded(component->expanded);
            item->setFlags((item->flags() | Qt::ItemIsEditable | Qt::ItemIsEnabled) & ~(Qt::ItemIsSelectable));
            m_componentItemMap[childId] = item;
            addComponentChildrenToItem(childId, item);
        }
        updateComponentSelectState(childId, isComponentSelected(childId));
    }
}

void PartTreeWidget::partComponentChecked(dust3d::Uuid partId)
{
    auto item = m_partItemMap.find(partId);
    if (item == m_partItemMap.end()) {
        return;
    }
    QTreeWidget::scrollToItem(item->second);
}

void PartTreeWidget::deleteItemChildren(QTreeWidgetItem *item)
{
    auto children = item->takeChildren();
    while (!children.isEmpty()) {
        auto first = children.takeFirst();
        auto componentId = dust3d::Uuid(first->data(0, Qt::UserRole).toString().toUtf8().constData());
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

void PartTreeWidget::componentChildrenChanged(dust3d::Uuid componentId)
{
    removeComponentDelayedTimer(componentId);
    
    QTimer *delayedTimer = new QTimer(this);
    delayedTimer->setSingleShot(true);
    delayedTimer->setInterval(200);
    
    connect(delayedTimer, &QTimer::timeout, this, [=]() {
        removeComponentDelayedTimer(componentId);
        reloadComponentChildren(componentId);
    });
    
    m_delayedComponentTimers.insert({componentId, delayedTimer});
    delayedTimer->start();
}

void PartTreeWidget::removeComponentDelayedTimer(const dust3d::Uuid &componentId)
{
    auto findTimer = m_delayedComponentTimers.find(componentId);
    if (findTimer != m_delayedComponentTimers.end()) {
        findTimer->second->deleteLater();
        m_delayedComponentTimers.erase(findTimer);
    }
}

void PartTreeWidget::reloadComponentChildren(const dust3d::Uuid &componentId)
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
    qDeleteAll(m_rootItem->takeChildren());
    m_partItemMap.clear();
    m_componentItemMap.clear();
    m_componentItemMap[dust3d::Uuid()] = m_rootItem;
}

void PartTreeWidget::componentRemoved(dust3d::Uuid componentId)
{
    auto componentItem = m_componentItemMap.find(componentId);
    if (componentItem == m_componentItemMap.end())
        return;
    m_selectedComponentIds.erase(componentId);
    if (m_currentSelectedComponentId == componentId)
        m_currentSelectedComponentId = dust3d::Uuid();
    m_componentItemMap.erase(componentId);
}

void PartTreeWidget::componentAdded(dust3d::Uuid componentId)
{
    // void
}

void PartTreeWidget::partRemoved(dust3d::Uuid partId)
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
    
    auto componentId = dust3d::Uuid(item->data(0, Qt::UserRole).toString().toUtf8().constData());
    
    const SkeletonComponent *component = m_document->findComponent(componentId);
    if (nullptr == component) {
        qDebug() << "Find component failed:" << componentId;
        return;
    }
    
    if (item->text(0) != component->name)
        emit renameComponent(componentId, item->text(0));
}

void PartTreeWidget::groupExpanded(QTreeWidgetItem *item)
{
    dust3d::Uuid componentId = dust3d::Uuid(item->data(0, Qt::UserRole).toString().toUtf8().constData());
    emit setComponentExpandState(componentId, true);
}

void PartTreeWidget::groupCollapsed(QTreeWidgetItem *item)
{
    dust3d::Uuid componentId = dust3d::Uuid(item->data(0, Qt::UserRole).toString().toUtf8().constData());
    emit setComponentExpandState(componentId, false);
}

void PartTreeWidget::partPreviewChanged(dust3d::Uuid partId)
{
    auto item = m_partItemMap.find(partId);
    if (item == m_partItemMap.end()) {
        qDebug() << "Part item not found:" << partId;
        return;
    }
    PartWidget *widget = (PartWidget *)itemWidget(item->second, 0);
    widget->updatePreview();
}

void PartTreeWidget::partLockStateChanged(dust3d::Uuid partId)
{
    auto item = m_partItemMap.find(partId);
    if (item == m_partItemMap.end()) {
        qDebug() << "Part item not found:" << partId;
        return;
    }
    PartWidget *widget = (PartWidget *)itemWidget(item->second, 0);
    widget->updateLockButton();
}

void PartTreeWidget::partVisibleStateChanged(dust3d::Uuid partId)
{
    auto item = m_partItemMap.find(partId);
    if (item == m_partItemMap.end()) {
        qDebug() << "Part item not found:" << partId;
        return;
    }
    PartWidget *widget = (PartWidget *)itemWidget(item->second, 0);
    widget->updateVisibleButton();
}

void PartTreeWidget::partSubdivStateChanged(dust3d::Uuid partId)
{
    auto item = m_partItemMap.find(partId);
    if (item == m_partItemMap.end()) {
        qDebug() << "Part item not found:" << partId;
        return;
    }
    PartWidget *widget = (PartWidget *)itemWidget(item->second, 0);
    widget->updateSubdivButton();
}

void PartTreeWidget::partDisableStateChanged(dust3d::Uuid partId)
{
    auto item = m_partItemMap.find(partId);
    if (item == m_partItemMap.end()) {
        qDebug() << "Part item not found:" << partId;
        return;
    }
    PartWidget *widget = (PartWidget *)itemWidget(item->second, 0);
    widget->updateDisableButton();
}

void PartTreeWidget::partXmirrorStateChanged(dust3d::Uuid partId)
{
    auto item = m_partItemMap.find(partId);
    if (item == m_partItemMap.end()) {
        qDebug() << "Part item not found:" << partId;
        return;
    }
    PartWidget *widget = (PartWidget *)itemWidget(item->second, 0);
    widget->updateXmirrorButton();
}

void PartTreeWidget::partDeformChanged(dust3d::Uuid partId)
{
    auto item = m_partItemMap.find(partId);
    if (item == m_partItemMap.end()) {
        qDebug() << "Part item not found:" << partId;
        return;
    }
    PartWidget *widget = (PartWidget *)itemWidget(item->second, 0);
    widget->updateDeformButton();
}

void PartTreeWidget::partRoundStateChanged(dust3d::Uuid partId)
{
    auto item = m_partItemMap.find(partId);
    if (item == m_partItemMap.end()) {
        qDebug() << "Part item not found:" << partId;
        return;
    }
    PartWidget *widget = (PartWidget *)itemWidget(item->second, 0);
    widget->updateRoundButton();
}

void PartTreeWidget::partChamferStateChanged(dust3d::Uuid partId)
{
    auto item = m_partItemMap.find(partId);
    if (item == m_partItemMap.end()) {
        qDebug() << "Part item not found:" << partId;
        return;
    }
    PartWidget *widget = (PartWidget *)itemWidget(item->second, 0);
    widget->updateChamferButton();
}

void PartTreeWidget::partColorStateChanged(dust3d::Uuid partId)
{
    auto item = m_partItemMap.find(partId);
    if (item == m_partItemMap.end()) {
        qDebug() << "Part item not found:" << partId;
        return;
    }
    PartWidget *widget = (PartWidget *)itemWidget(item->second, 0);
    widget->updateColorButton();
}

void PartTreeWidget::partCutRotationChanged(dust3d::Uuid partId)
{
    auto item = m_partItemMap.find(partId);
    if (item == m_partItemMap.end()) {
        qDebug() << "Part item not found:" << partId;
        return;
    }
    PartWidget *widget = (PartWidget *)itemWidget(item->second, 0);
    widget->updateCutRotationButton();
}

void PartTreeWidget::partCutFaceChanged(dust3d::Uuid partId)
{
    auto item = m_partItemMap.find(partId);
    if (item == m_partItemMap.end()) {
        qDebug() << "Part item not found:" << partId;
        return;
    }
    PartWidget *widget = (PartWidget *)itemWidget(item->second, 0);
    widget->updateCutRotationButton();
}

void PartTreeWidget::partHollowThicknessChanged(dust3d::Uuid partId)
{
    auto item = m_partItemMap.find(partId);
    if (item == m_partItemMap.end()) {
        qDebug() << "Part item not found:" << partId;
        return;
    }
    PartWidget *widget = (PartWidget *)itemWidget(item->second, 0);
    widget->updateCutRotationButton();
}

void PartTreeWidget::partMaterialIdChanged(dust3d::Uuid partId)
{
    auto item = m_partItemMap.find(partId);
    if (item == m_partItemMap.end()) {
        qDebug() << "Part item not found:" << partId;
        return;
    }
    PartWidget *widget = (PartWidget *)itemWidget(item->second, 0);
    widget->updateColorButton();
}

void PartTreeWidget::partColorSolubilityChanged(dust3d::Uuid partId)
{
    auto item = m_partItemMap.find(partId);
    if (item == m_partItemMap.end()) {
        qDebug() << "Part item not found:" << partId;
        return;
    }
    PartWidget *widget = (PartWidget *)itemWidget(item->second, 0);
    widget->updateColorButton();
}

void PartTreeWidget::partMetalnessChanged(dust3d::Uuid partId)
{
    auto item = m_partItemMap.find(partId);
    if (item == m_partItemMap.end()) {
        qDebug() << "Part item not found:" << partId;
        return;
    }
    PartWidget *widget = (PartWidget *)itemWidget(item->second, 0);
    widget->updateColorButton();
}

void PartTreeWidget::partRoughnessChanged(dust3d::Uuid partId)
{
    auto item = m_partItemMap.find(partId);
    if (item == m_partItemMap.end()) {
        qDebug() << "Part item not found:" << partId;
        return;
    }
    PartWidget *widget = (PartWidget *)itemWidget(item->second, 0);
    widget->updateColorButton();
}

void PartTreeWidget::partCountershadeStateChanged(dust3d::Uuid partId)
{
    auto item = m_partItemMap.find(partId);
    if (item == m_partItemMap.end()) {
        qDebug() << "Part item not found:" << partId;
        return;
    }
    PartWidget *widget = (PartWidget *)itemWidget(item->second, 0);
    widget->updateColorButton();
}

void PartTreeWidget::partSmoothStateChanged(dust3d::Uuid partId)
{
    auto item = m_partItemMap.find(partId);
    if (item == m_partItemMap.end()) {
        qDebug() << "Part item not found:" << partId;
        return;
    }
    PartWidget *widget = (PartWidget *)itemWidget(item->second, 0);
    widget->updateSmoothButton();
}

void PartTreeWidget::partChecked(dust3d::Uuid partId)
{
    auto item = m_partItemMap.find(partId);
    if (item == m_partItemMap.end()) {
        qDebug() << "Part item not found:" << partId;
        return;
    }
    item->second->setBackground(0, m_hightlightedPartBackground);
}

void PartTreeWidget::partUnchecked(dust3d::Uuid partId)
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

bool PartTreeWidget::isComponentSelected(dust3d::Uuid componentId)
{
    return (m_currentSelectedComponentId == componentId ||
        m_selectedComponentIds.find(componentId) != m_selectedComponentIds.end());
}
