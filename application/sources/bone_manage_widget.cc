#include "bone_manage_widget.h"
#include "bone_list_model.h"
#include "bone_preview_grid_widget.h"
#include "bone_property_widget.h"
#include "document.h"
#include "theme.h"
#include <QVBoxLayout>
#include <QWidgetAction>

BoneManageWidget::BoneManageWidget(Document* document, QWidget* parent)
    : QWidget(parent)
    , m_document(document)
{
    setContextMenuPolicy(Qt::CustomContextMenu);

    QHBoxLayout* toolsLayout = new QHBoxLayout;
    toolsLayout->setSpacing(0);
    toolsLayout->setMargin(0);

    setStyleSheet("QPushButton:disabled {border: 0; color: " + Theme::gray.name() + "}");

    auto createButton = [](QIcon icon, const QString& title) {
        QPushButton* button = new QPushButton(icon, "");
        Theme::initIconButton(button);
        button->setToolTip(title);
        return button;
    };

    m_addButton = createButton(Theme::awesome()->icon(fa::plus), tr("Add new bone"));
    m_selectButton = createButton(Theme::awesome()->icon(fa::objectgroup), tr("Select them on canvas"));
    m_propertyButton = createButton(Theme::awesome()->icon(fa::sliders), tr("Configure properties"));

    toolsLayout->addWidget(m_addButton);
    toolsLayout->addWidget(m_selectButton);
    toolsLayout->addWidget(m_propertyButton);
    toolsLayout->addStretch();

    QWidget* toolsWidget = new QWidget();
    toolsWidget->setObjectName("tools");
    toolsWidget->setStyleSheet("QWidget#tools {background: qlineargradient( x1:0 y1:0, x2:1 y2:0, stop:0 transparent, stop:0.5 " + Theme::black.name() + ", stop:1 transparent)};");
    toolsWidget->setLayout(toolsLayout);

    m_bonePreviewGridWidget = new BonePreviewGridWidget(document);

    connect(m_bonePreviewGridWidget->selectionModel(), &QItemSelectionModel::selectionChanged, this, &BoneManageWidget::updateToolButtons);
    connect(m_bonePreviewGridWidget, &BonePreviewGridWidget::unselectAllOnCanvas, this, &BoneManageWidget::unselectAllOnCanvas);
    connect(m_bonePreviewGridWidget, &BonePreviewGridWidget::selectNodeOnCanvas, this, &BoneManageWidget::selectNodeOnCanvas);

    connect(m_addButton, &QPushButton::clicked, this, [this]() {
        dust3d::Uuid boneId = dust3d::Uuid::createUuid();
        this->m_document->addBone(boneId);
        this->m_document->saveSnapshot();
    });

    connect(m_selectButton, &QPushButton::clicked, [this]() {
        for (const auto& boneId : this->m_bonePreviewGridWidget->getSelectedBoneIds()) {
            for (const auto& nodeIt : m_document->nodeMap) {
                if (nodeIt.second.boneIds.end() == nodeIt.second.boneIds.find(boneId))
                    continue;
                emit this->selectNodeOnCanvas(nodeIt.first);
            }
        }
    });

    connect(m_propertyButton, &QPushButton::clicked, this, &BoneManageWidget::showSelectedBoneProperties);

    connect(this, &BoneManageWidget::groupOperationAdded, m_document, &Document::saveSnapshot);
    connect(this, &BoneManageWidget::removeBone, m_document, &Document::removeBone);

    connect(this, &BoneManageWidget::customContextMenuRequested, this, &BoneManageWidget::showContextMenu);

    QVBoxLayout* mainLayout = new QVBoxLayout;
    mainLayout->addWidget(toolsWidget);
    mainLayout->addWidget(m_bonePreviewGridWidget);

    setLayout(mainLayout);

    updateToolButtons();
}

void BoneManageWidget::showSelectedBoneProperties()
{
    auto boneIds = m_bonePreviewGridWidget->getSelectedBoneIds();
    if (boneIds.empty())
        return;

    auto* propertyWidget = new BonePropertyWidget(m_document, boneIds);

    m_propertyMenu = std::make_unique<QMenu>(this->parentWidget());

    connect(propertyWidget, &BonePropertyWidget::pickBoneJoints, this, [this](const dust3d::Uuid& boneId, size_t joints) {
        if (nullptr == this->m_propertyMenu)
            return;
        this->m_document->startBoneJointsPicking(boneId, joints);
        this->m_propertyMenu->close();
    });

    QWidgetAction* widgetAction = new QWidgetAction(m_propertyMenu.get());
    widgetAction->setDefaultWidget(propertyWidget);
    m_propertyMenu->addAction(widgetAction);

    auto x = mapToGlobal(QPoint(0, 0)).x();
    if (x <= 0)
        x = QCursor::pos().x();
    m_propertyMenu->exec(QPoint(
        x - propertyWidget->width(),
        QCursor::pos().y()));
}

void BoneManageWidget::selectBoneByBoneId(const dust3d::Uuid& boneId)
{
    QModelIndex index = m_bonePreviewGridWidget->boneListModel()->boneIdToIndex(boneId);
    if (index.isValid()) {
        m_bonePreviewGridWidget->selectionModel()->select(index, QItemSelectionModel::ClearAndSelect);
        m_bonePreviewGridWidget->scrollTo(index);
        return;
    }
    dust3dDebug << "Unable to select bone:" << boneId.toString();
}

void BoneManageWidget::updateToolButtons()
{
    auto selectedBones = m_bonePreviewGridWidget->getSelectedBones();
    bool enableSelectButton = false;
    bool enablePropertyButton = false;
    for (const auto& bone : selectedBones) {
        enablePropertyButton = true;
        enableSelectButton = true;
    }
    m_selectButton->setEnabled(enableSelectButton);
    m_propertyButton->setEnabled(enablePropertyButton);
}

void BoneManageWidget::removeSelectedBones()
{
    auto selectedBoneIds = m_bonePreviewGridWidget->getSelectedBoneIds();
    for (const auto& boneId : selectedBoneIds)
        emit removeBone(boneId);
    emit groupOperationAdded();
}

void BoneManageWidget::showContextMenu(const QPoint& pos)
{
    auto selectedBoneIds = m_bonePreviewGridWidget->getSelectedBoneIds();
    if (selectedBoneIds.empty())
        return;

    QMenu contextMenu(this);

    QAction deleteAction(tr("Delete"), this);
    connect(&deleteAction, &QAction::triggered, this, &BoneManageWidget::removeSelectedBones);
    contextMenu.addAction(&deleteAction);

    contextMenu.exec(mapToGlobal(pos));
}
