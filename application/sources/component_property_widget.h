#ifndef DUST3D_APPLICATION_COMPONENT_PROPERTY_WIDGET_H_
#define DUST3D_APPLICATION_COMPONENT_PROPERTY_WIDGET_H_

#include <dust3d/base/uuid.h>
#include <QWidget>

class Document;

class ComponentPropertyWidget: public QWidget
{
    Q_OBJECT
signals:
    void beginColorPicking();
    void endColorPicking();
    void setPartColorState(const dust3d::Uuid &partId, bool hasColor, const QColor &color);
    void groupOperationAdded();
public:
    ComponentPropertyWidget(Document *document, 
        const std::vector<dust3d::Uuid> &componentIds, 
        QWidget *parent=nullptr);
public slots:
    void showColorDialog();
private:
    Document *m_document = nullptr;
    std::vector<dust3d::Uuid> m_componentIds;
    std::vector<dust3d::Uuid> m_partIds;
    QColor m_color;
    QColor lastColor();
    void preparePartIds();
};

#endif
