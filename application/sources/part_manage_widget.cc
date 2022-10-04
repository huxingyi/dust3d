#include <QVBoxLayout>
#include "part_manage_widget.h"
#include "component_preview_grid_widget.h"
#include "component_list_model.h"
#include "theme.h"
#include "document.h"

PartManageWidget::PartManageWidget(const Document *document, QWidget *parent):
    QWidget(parent),
    m_document(document)
{
    QHBoxLayout *toolsLayout = new QHBoxLayout;
    toolsLayout->setSpacing(0);
    toolsLayout->setMargin(0);

    setStyleSheet("QPushButton:disabled {border: 0; color: " + Theme::gray.name() + "}");

    auto createButton = [](QChar icon) {
        QPushButton *button = new QPushButton(icon);
        button->setFixedSize(Theme::toolIconSize, Theme::toolIconSize);
        return button;
    };

    m_levelUpButton = createButton(QChar(fa::levelup));
    m_lockButton = createButton(QChar(fa::lock));
    m_unlockButton = createButton(QChar(fa::unlock));
    m_showButton = createButton(QChar(fa::eye));
    m_hideButton = createButton(QChar(fa::eyeslash));
    m_unlinkButton = createButton(QChar(fa::unlink));
    m_linkButton = createButton(QChar(fa::link));
    m_removeButton = createButton(QChar(fa::trash));

    toolsLayout->addWidget(m_levelUpButton);
    toolsLayout->addWidget(m_hideButton);
    toolsLayout->addWidget(m_showButton);
    toolsLayout->addWidget(m_lockButton);
    toolsLayout->addWidget(m_unlockButton);
    toolsLayout->addWidget(m_unlinkButton);
    toolsLayout->addWidget(m_linkButton);
    toolsLayout->addWidget(m_removeButton);
    toolsLayout->addStretch();

    m_componentPreviewGridWidget = new ComponentPreviewGridWidget(document);

    connect(m_componentPreviewGridWidget->selectionModel(), &QItemSelectionModel::selectionChanged, this, &PartManageWidget::updateToolButtons);
    connect(m_componentPreviewGridWidget->componentListModel(), &ComponentListModel::listingComponentChanged, this, &PartManageWidget::updateLevelUpButton);

    connect(m_levelUpButton, &QPushButton::clicked, [this]() {
        const auto &parent = m_document->findComponentParent(m_componentPreviewGridWidget->componentListModel()->listingComponentId());
        if (nullptr == parent)
            return;
        this->m_componentPreviewGridWidget->componentListModel()->setListingComponentId(parent->id);
    });

    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->addLayout(toolsLayout);
    mainLayout->addWidget(m_componentPreviewGridWidget);

    setLayout(mainLayout);

    updateToolButtons();
    updateLevelUpButton();
}

void PartManageWidget::updateLevelUpButton()
{
    const auto &parent = m_document->findComponentParent(m_componentPreviewGridWidget->componentListModel()->listingComponentId());
    m_levelUpButton->setEnabled(nullptr != parent);
}

void PartManageWidget::updateToolButtons()
{
    auto selectedComponents = m_componentPreviewGridWidget->getSelectedComponents();
    bool enableHideButton = false;
    bool enableShowButton = false;
    bool enableLockButton = false;
    bool enableUnlockButton = false;
    bool enableUnlinkButton = false;
    bool enableLinkButton = false;
    bool enableRemoveButton = false;
    for (const auto &component: selectedComponents) {
        enableRemoveButton = true;
        if (component->linkToPartId.isNull()) {
            continue;
        }
        const auto &part = m_document->findPart(component->linkToPartId);
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
    m_hideButton->setEnabled(enableHideButton);
    m_showButton->setEnabled(enableShowButton);
    m_lockButton->setEnabled(enableLockButton);
    m_unlockButton->setEnabled(enableUnlockButton);
    m_unlinkButton->setEnabled(enableUnlinkButton);
    m_linkButton->setEnabled(enableLinkButton);
    m_removeButton->setEnabled(enableRemoveButton);
}
