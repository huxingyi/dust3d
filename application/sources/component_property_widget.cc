#include <unordered_set>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QColorDialog>
#include "component_property_widget.h"
#include "document.h"
#include "theme.h"

ComponentPropertyWidget::ComponentPropertyWidget(Document *document, 
        const std::vector<dust3d::Uuid> &componentIds,
        QWidget *parent):
    QWidget(parent),
    m_document(document),
    m_componentIds(componentIds)
{
    preparePartIds();
    m_color = lastColor();

    QPushButton *colorPreviewArea = new QPushButton;
    colorPreviewArea->setStyleSheet("QPushButton {background-color: " + m_color.name() + "; border-radius: 0;}");
    colorPreviewArea->setFixedSize(Theme::toolIconSize * 1.8, Theme::toolIconSize);

    QPushButton *colorPickerButton = new QPushButton(QChar(fa::eyedropper));
    colorPickerButton->setFixedSize(Theme::toolIconSize, Theme::toolIconSize);
    connect(colorPickerButton, &QPushButton::clicked, this, &ComponentPropertyWidget::showColorDialog);

    QHBoxLayout *colorLayout = new QHBoxLayout;
    colorLayout->addWidget(colorPreviewArea);
    colorLayout->addWidget(colorPickerButton);
    colorLayout->setSizeConstraint(QLayout::SetFixedSize);

    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->addLayout(colorLayout);
    mainLayout->setSizeConstraint(QLayout::SetFixedSize);

    connect(this, &ComponentPropertyWidget::setPartColorState, m_document, &Document::setPartColorState);
    connect(this, &ComponentPropertyWidget::endColorPicking, m_document, &Document::enableBackgroundBlur);
    connect(this, &ComponentPropertyWidget::beginColorPicking, m_document, &Document::disableBackgroundBlur);
    connect(this, &ComponentPropertyWidget::groupOperationAdded, m_document, &Document::saveSnapshot);

    setLayout(mainLayout);

    setFixedSize(minimumSizeHint());
}

void ComponentPropertyWidget::preparePartIds()
{
    std::unordered_set<dust3d::Uuid> addedPartIdSet;
    for (const auto &componentId: m_componentIds) {
        std::vector<dust3d::Uuid> partIds;
        m_document->collectComponentDescendantParts(componentId, partIds);
        for (const auto &it: partIds) {
            if (addedPartIdSet.insert(it).second)
                m_partIds.emplace_back(it);
        }
    }
}

QColor ComponentPropertyWidget::lastColor()
{
    QColor color = Qt::white;
    std::map<QString, int> colorMap; 
    for (const auto &partId: m_partIds) {
        const SkeletonPart *part = m_document->findPart(partId);
        if (nullptr == part)
            continue;
        colorMap[part->color.name()]++;
    }
    if (!colorMap.empty()) {
        color = std::max_element(colorMap.begin(), colorMap.end(), 
                [](const std::map<QString, int>::value_type &a, const std::map<QString, int>::value_type &b) {
            return a.second < b.second;
        })->first;
    }
    return color;
}

void ComponentPropertyWidget::showColorDialog()
{
    emit beginColorPicking();
    QColor color = QColorDialog::getColor(m_color, this);
    emit endColorPicking();
    if (!color.isValid())
        return;

    for (const auto &partId: m_partIds) {
        emit setPartColorState(partId, true, color);
    }
    emit groupOperationAdded();
}
