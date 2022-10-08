#ifndef DUST3D_APPLICATION_COMPONENT_PROPERTY_WIDGET_H_
#define DUST3D_APPLICATION_COMPONENT_PROPERTY_WIDGET_H_

#include <dust3d/base/uuid.h>
#include <QWidget>

class Document;

class ComponentPropertyWidget: public QWidget
{
    Q_OBJECT
public:
    ComponentPropertyWidget(Document *document, QWidget *parent=nullptr);
public slots:
    void setComponentIds(const std::vector<dust3d::Uuid> &componentIds);
private:
    Document *m_document = nullptr;
    std::vector<dust3d::Uuid> m_componentIds;
};

#endif
