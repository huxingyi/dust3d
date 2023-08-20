#include "part_manage_widget.h"
#include "component_list_model.h"
#include "component_preview_grid_widget.h"
#include "component_property_widget.h"
#include "document.h"
#include "theme.h"
#include <QMenu>
#include <QVBoxLayout>
#include <QWidgetAction>

PartManageWidget::PartManageWidget(Document* document, QWidget* parent)
    : QWidget(parent)
    , m_document(document)
{
    setContextMenuPolicy(Qt::CustomContextMenu);

    QHBoxLayout* toolsLayout = new QHBoxLayout;
    toolsLayout->setSpacing(0);
    toolsLayout->setContentsMargins(0, 0, 0, 0);

    setStyleSheet("QPushButton:disabled {border: 0; color: " + Theme::gray.name() + "}");

    auto createButton = [](QIcon icon, const QString& title) {
        QPushButton* button = new QPushButton(icon, "");
        Theme::initIconButton(button);
        button->setToolTip(title);
        return button;
    };

    m_levelUpButton = createButton(Theme::awesome()->icon(fa::levelup), tr("Go level up"));
    m_selectButton = createButton(Theme::awesome()->icon(fa::objectgroup), tr("Select them on canvas"));
    m_lockButton = createButton(Theme::awesome()->icon(fa::lock), tr("Lock them on canvas"));
    m_unlockButton = createButton(Theme::awesome()->icon(fa::unlock), tr("Unlock them on canvas"));
    m_showButton = createButton(Theme::awesome()->icon(fa::eye), tr("Show them on canvas"));
    m_hideButton = createButton(Theme::awesome()->icon(fa::eyeslash), tr("Hide them on canvas"));
    m_unlinkButton = createButton(Theme::awesome()->icon(fa::unlink), tr("Exclude them from result generation"));
    m_linkButton = createButton(Theme::awesome()->icon(fa::link), tr("Include them in result generation"));
    m_propertyButton = createButton(Theme::awesome()->icon(fa::sliders), tr("Configure properties"));

    toolsLayout->addWidget(m_levelUpButton);
    toolsLayout->addWidget(m_selectButton);
    toolsLayout->addWidget(m_hideButton);
    toolsLayout->addWidget(m_showButton);
    toolsLayout->addWidget(m_lockButton);
    toolsLayout->addWidget(m_unlockButton);
    toolsLayout->addWidget(m_unlinkButton);
    toolsLayout->addWidget(m_linkButton);
    toolsLayout->addWidget(m_propertyButton);
    toolsLayout->addStretch();

    QWidget* toolsWidget = new QWidget();
    toolsWidget->setObjectName("tools");
    toolsWidget->setStyleSheet("QWidget#tools {background: qlineargradient( x1:0 y1:0, x2:1 y2:0, stop:0 transparent, stop:0.5 " + Theme::black.name() + ", stop:1 transparent)};");
    toolsWidget->setLayout(toolsLayout);

    m_componentPreviewGridWidget = new ComponentPreviewGridWidget(document);

    connect(m_componentPreviewGridWidget->selectionModel(), &QItemSelectionModel::selectionChanged, this, &PartManageWidget::updateToolButtons);
    connect(m_componentPreviewGridWidget->componentListModel(), &ComponentListModel::listingComponentChanged, this, &PartManageWidget::updateLevelUpButton);
    connect(m_componentPreviewGridWidget, &ComponentPreviewGridWidget::unselectAllOnCanvas, this, &PartManageWidget::unselectAllOnCanvas);
    connect(m_componentPreviewGridWidget, &ComponentPreviewGridWidget::selectPartOnCanvas, this, &PartManageWidget::selectPartOnCanvas);

    //connect(m_componentPreviewGridWidget->selectionModel(), &QItemSelectionModel::selectionChanged, this, &PartManageWidget::showSelectedComponentProperties);

    connect(m_levelUpButton, &QPushButton::clicked, [this]() {
        const auto& parent = m_document->findComponentParent(this->m_componentPreviewGridWidget->componentListModel()->listingComponentId());
        if (nullptr == parent)
            return;
        this->m_componentPreviewGridWidget->componentListModel()->setListingComponentId(parent->id);
    });

    connect(m_hideButton, &QPushButton::clicked, [this]() {
        for (const auto& partId : this->m_componentPreviewGridWidget->getSelectedPartIds())
            this->m_document->setPartVisibleState(partId, false);
        this->m_document->saveSnapshot();
    });
    connect(m_showButton, &QPushButton::clicked, [this]() {
        for (const auto& partId : this->m_componentPreviewGridWidget->getSelectedPartIds())
            this->m_document->setPartVisibleState(partId, true);
        this->m_document->saveSnapshot();
    });

    connect(m_unlockButton, &QPushButton::clicked, [this]() {
        for (const auto& partId : this->m_componentPreviewGridWidget->getSelectedPartIds())
            this->m_document->setPartLockState(partId, false);
        this->m_document->saveSnapshot();
    });
    connect(m_lockButton, &QPushButton::clicked, [this]() {
        for (const auto& partId : this->m_componentPreviewGridWidget->getSelectedPartIds())
            this->m_document->setPartLockState(partId, true);
        this->m_document->saveSnapshot();
    });

    connect(m_unlinkButton, &QPushButton::clicked, [this]() {
        for (const auto& partId : this->m_componentPreviewGridWidget->getSelectedPartIds())
            this->m_document->setPartDisableState(partId, true);
        this->m_document->saveSnapshot();
    });
    connect(m_linkButton, &QPushButton::clicked, [this]() {
        for (const auto& partId : this->m_componentPreviewGridWidget->getSelectedPartIds())
            this->m_document->setPartDisableState(partId, false);
        this->m_document->saveSnapshot();
    });

    connect(m_selectButton, &QPushButton::clicked, [this]() {
        for (const auto& componentId : this->m_componentPreviewGridWidget->getSelectedComponentIds()) {
            std::vector<dust3d::Uuid> partIds;
            this->m_document->collectComponentDescendantParts(componentId, partIds);
            for (const auto& partId : partIds)
                emit this->selectPartOnCanvas(partId);
        }
    });

    connect(m_propertyButton, &QPushButton::clicked, this, &PartManageWidget::showSelectedComponentProperties);

    connect(m_document, &Document::partLockStateChanged, this, &PartManageWidget::updateToolButtons);
    connect(m_document, &Document::partVisibleStateChanged, this, &PartManageWidget::updateToolButtons);
    connect(m_document, &Document::partDisableStateChanged, this, &PartManageWidget::updateToolButtons);
    connect(m_document, &Document::componentChildrenChanged, this, &PartManageWidget::updateToolButtons);

    connect(this, &PartManageWidget::groupComponents, m_document, &Document::groupComponents);
    connect(this, &PartManageWidget::removeComponent, m_document, &Document::removeComponent);
    connect(this, &PartManageWidget::ungroupComponent, m_document, &Document::ungroupComponent);
    connect(this, &PartManageWidget::moveComponentUp, m_document, &Document::moveComponentUp);
    connect(this, &PartManageWidget::moveComponentDown, m_document, &Document::moveComponentDown);
    connect(this, &PartManageWidget::moveComponentToTop, m_document, &Document::moveComponentToTop);
    connect(this, &PartManageWidget::moveComponentToBottom, m_document, &Document::moveComponentToBottom);
    connect(this, &PartManageWidget::setPartTarget, m_document, &Document::setPartTarget);
    connect(this, &PartManageWidget::groupOperationAdded, m_document, &Document::saveSnapshot);

    connect(this, &PartManageWidget::customContextMenuRequested, this, &PartManageWidget::showContextMenu);

    QVBoxLayout* mainLayout = new QVBoxLayout;
    mainLayout->addWidget(toolsWidget);
    mainLayout->addWidget(m_componentPreviewGridWidget);

    setLayout(mainLayout);

    updateToolButtons();
    updateLevelUpButton();
}

void PartManageWidget::showSelectedComponentProperties()
{
    auto componentIds = m_componentPreviewGridWidget->getSelectedComponentIds();
    if (componentIds.empty())
        return;

    auto* propertyWidget = new ComponentPropertyWidget(m_document, componentIds);

    m_contextMenu.reset(new QMenu(this->parentWidget()));
    QWidgetAction* widgetAction = new QWidgetAction(m_contextMenu.get());
    widgetAction->setDefaultWidget(propertyWidget);
    m_contextMenu->addAction(widgetAction);

    auto x = mapToGlobal(QPoint(0, 0)).x();
    if (x <= 0)
        x = QCursor::pos().x();
    m_contextMenu->popup(QPoint(
        x - propertyWidget->width(),
        QCursor::pos().y()));
}

void PartManageWidget::selectComponentByPartId(const dust3d::Uuid& partId)
{
    const auto& part = m_document->findPart(partId);
    if (nullptr == part)
        return;
    auto componentId = part->componentId;
    QModelIndex index = m_componentPreviewGridWidget->componentListModel()->componentIdToIndex(componentId);
    if (index.isValid()) {
        m_componentPreviewGridWidget->selectionModel()->select(index, QItemSelectionModel::ClearAndSelect);
        m_componentPreviewGridWidget->scrollTo(index);
        return;
    }
    const auto& component = m_document->findComponent(componentId);
    if (nullptr == component)
        return;
    m_componentPreviewGridWidget->componentListModel()->setListingComponentId(component->parentId);
    index = m_componentPreviewGridWidget->componentListModel()->componentIdToIndex(componentId);
    if (index.isValid()) {
        m_componentPreviewGridWidget->selectionModel()->select(index, QItemSelectionModel::ClearAndSelect);
        m_componentPreviewGridWidget->scrollTo(index);
        return;
    }
    dust3dDebug << "Unable to select component:" << componentId.toString();
}

void PartManageWidget::updateLevelUpButton()
{
    const auto& parent = m_document->findComponentParent(m_componentPreviewGridWidget->componentListModel()->listingComponentId());
    m_levelUpButton->setEnabled(nullptr != parent);
}

void PartManageWidget::updateToolButtons()
{
    auto selectedComponents = m_componentPreviewGridWidget->getSelectedComponents();
    bool enableHideButton = false;
    bool enableSelectButton = false;
    bool enableShowButton = false;
    bool enableLockButton = false;
    bool enableUnlockButton = false;
    bool enableUnlinkButton = false;
    bool enableLinkButton = false;
    bool enablePropertyButton = false;
    for (const auto& component : selectedComponents) {
        enablePropertyButton = true;
        enableSelectButton = true;
        if (component->linkToPartId.isNull()) {
            continue;
        }
        const auto& part = m_document->findPart(component->linkToPartId);
        if (part->visible) {
            enableHideButton = true;
        } else {
            enableShowButton = true;
        }
        if (part->locked) {
            enableUnlockButton = true;
        } else {
            enableLockButton = true;
        }
        if (part->disabled) {
            enableLinkButton = true;
        } else {
            enableUnlinkButton = true;
        }
    }
    m_selectButton->setEnabled(enableSelectButton);
    m_hideButton->setEnabled(enableHideButton);
    m_showButton->setEnabled(enableShowButton);
    m_lockButton->setEnabled(enableLockButton);
    m_unlockButton->setEnabled(enableUnlockButton);
    m_unlinkButton->setEnabled(enableUnlinkButton);
    m_linkButton->setEnabled(enableLinkButton);
    m_propertyButton->setEnabled(enablePropertyButton);
}

bool PartManageWidget::hasSelectedGroupedComponent()
{
    auto selectedComponents = m_componentPreviewGridWidget->getSelectedComponents();
    for (auto& component : selectedComponents) {
        if (!component->childrenIds.empty())
            return true;
    }
    return false;
}

void PartManageWidget::showContextMenu(const QPoint& pos)
{
    auto selectedComponentIds = m_componentPreviewGridWidget->getSelectedComponentIds();
    if (selectedComponentIds.empty())
        return;

    m_contextMenu.reset(new QMenu(this));

    QAction* makeGroupAction = new QAction(tr("Make group"), m_contextMenu.get());
    makeGroupAction->setIcon(Theme::awesome()->icon(fa::folder));
    connect(makeGroupAction, &QAction::triggered, this, [=]() {
        emit this->groupComponents(selectedComponentIds);
        emit this->groupOperationAdded();
    });
    m_contextMenu->addAction(makeGroupAction);

    QAction* ungroupAction = new QAction(tr("Ungroup"), m_contextMenu.get());
    if (hasSelectedGroupedComponent()) {
        connect(ungroupAction, &QAction::triggered, this, [=]() {
            for (const auto& it : selectedComponentIds)
                emit this->ungroupComponent(it);
            emit this->groupOperationAdded();
        });
        m_contextMenu->addAction(ungroupAction);
    }

    QAction* moveToTopAction = new QAction(tr("Begin"), m_contextMenu.get());
    QAction* moveUpAction = new QAction(tr("Previous"), m_contextMenu.get());
    QAction* moveDownAction = new QAction(tr("Next"), m_contextMenu.get());
    QAction* moveToBottomAction = new QAction(tr("End"), m_contextMenu.get());
    if (1 == selectedComponentIds.size()) {
        QMenu* moveToMenu = m_contextMenu->addMenu(tr("Move To"));

        moveToTopAction->setIcon(Theme::awesome()->icon(fa::angledoubleup));
        connect(moveToTopAction, &QAction::triggered, [=]() {
            for (const auto& it : selectedComponentIds)
                emit moveComponentToTop(it);
            emit groupOperationAdded();
        });
        moveToMenu->addAction(moveToTopAction);

        moveUpAction->setIcon(Theme::awesome()->icon(fa::angleup));
        connect(moveUpAction, &QAction::triggered, [=]() {
            for (const auto& it : selectedComponentIds)
                emit moveComponentUp(it);
            emit groupOperationAdded();
        });
        moveToMenu->addAction(moveUpAction);

        moveDownAction->setIcon(Theme::awesome()->icon(fa::angledown));
        connect(moveDownAction, &QAction::triggered, [=]() {
            for (const auto& it : selectedComponentIds)
                emit moveComponentDown(it);
            emit groupOperationAdded();
        });
        moveToMenu->addAction(moveDownAction);

        moveToBottomAction->setIcon(Theme::awesome()->icon(fa::angledoubledown));
        connect(moveToBottomAction, &QAction::triggered, [=]() {
            for (const auto& it : selectedComponentIds)
                emit moveComponentToBottom(it);
            emit groupOperationAdded();
        });
        moveToMenu->addAction(moveToBottomAction);

        moveToMenu->addSeparator();
    }

    QAction* convertToCutFaceAction = new QAction(tr("Convert to Cut Face"), m_contextMenu.get());
    QAction* convertToStitchingLineAction = new QAction(tr("Convert to Stitching Line"), m_contextMenu.get());
    QAction* convertToPartAction = new QAction(tr("Convert to Model"), m_contextMenu.get());
    auto selectedPartIds = m_componentPreviewGridWidget->getSelectedPartIds();
    if (!selectedPartIds.empty()) {
        bool addConvertToPartAction = false;
        bool addConvertToCutFaceAction = false;
        bool addConvertToStitchingLineAction = false;
        for (const auto& it : selectedPartIds) {
            const Document::Part* part = m_document->findPart(it);
            if (dust3d::PartTarget::Model != part->target) {
                addConvertToPartAction = true;
            } else {
                addConvertToCutFaceAction = true;
                addConvertToStitchingLineAction = true;
            }
        }
        if (addConvertToPartAction) {
            connect(convertToPartAction, &QAction::triggered, this, [=]() {
                for (const auto& it : selectedPartIds)
                    emit this->setPartTarget(it, dust3d::PartTarget::Model);
                emit this->groupOperationAdded();
            });
            m_contextMenu->addAction(convertToPartAction);
        }
        if (addConvertToCutFaceAction) {
            connect(convertToCutFaceAction, &QAction::triggered, this, [=]() {
                for (const auto& it : selectedPartIds)
                    emit this->setPartTarget(it, dust3d::PartTarget::CutFace);
                emit this->groupOperationAdded();
            });
            m_contextMenu->addAction(convertToCutFaceAction);
        }
        if (addConvertToStitchingLineAction) {
            connect(convertToStitchingLineAction, &QAction::triggered, this, [=]() {
                for (const auto& it : selectedPartIds)
                    emit this->setPartTarget(it, dust3d::PartTarget::StitchingLine);
                emit this->groupOperationAdded();
            });
            m_contextMenu->addAction(convertToStitchingLineAction);
        }
    }

    QAction* deleteAction = new QAction(tr("Delete"), m_contextMenu.get());
    deleteAction->setIcon(Theme::awesome()->icon(fa::remove));
    connect(deleteAction, &QAction::triggered, this, [=]() {
        for (const auto& componentId : selectedComponentIds)
            emit this->removeComponent(componentId);
        emit this->groupOperationAdded();
    });
    m_contextMenu->addAction(deleteAction);

    m_contextMenu->popup(mapToGlobal(pos));
}
