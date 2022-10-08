#ifndef DUST3D_APPLICATION_PART_MANAGE_WIDGET_H_
#define DUST3D_APPLICATION_PART_MANAGE_WIDGET_H_

#include <dust3d/base/uuid.h>
#include <QWidget>

class Document;
class ComponentPreviewGridWidget;
class ComponentPropertyWidget;
class QPushButton;

class PartManageWidget: public QWidget
{
    Q_OBJECT
signals:
    void unselectAllOnCanvas();
    void selectPartOnCanvas(const dust3d::Uuid &partId);
public slots:
    void selectComponentByPartId(const dust3d::Uuid &partId);
    void showSelectedComponentProperties();
public:
    PartManageWidget(Document *document, QWidget *parent=nullptr);
private:
    Document *m_document = nullptr;
    ComponentPreviewGridWidget *m_componentPreviewGridWidget = nullptr;
    QPushButton *m_levelUpButton = nullptr;
    QPushButton *m_selectButton = nullptr;
    QPushButton *m_lockButton = nullptr;
    QPushButton *m_unlockButton = nullptr;
    QPushButton *m_showButton = nullptr;
    QPushButton *m_hideButton = nullptr;
    QPushButton *m_unlinkButton = nullptr;
    QPushButton *m_linkButton = nullptr;
    QPushButton *m_propertyButton = nullptr;
    void updateToolButtons();
    void updateLevelUpButton();
};

#endif
