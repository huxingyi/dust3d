#include "component_property_widget.h"

ComponentPropertyWidget::ComponentPropertyWidget(Document *document, QWidget *parent):
    QWidget(parent),
    m_document(document)
{
    setFixedSize(300, 200);
}

void ComponentPropertyWidget::setComponentIds(const std::vector<dust3d::Uuid> &componentIds)
{
    if (m_componentIds == componentIds)
        return;
    m_componentIds = componentIds;
    if (m_componentIds.empty()) {
        //hide();
        return;
    }
    // TODO:
}
