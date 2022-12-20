#ifndef DUST3D_APPLICATION_PART_MANAGE_WIDGET_H_
#define DUST3D_APPLICATION_PART_MANAGE_WIDGET_H_

#include <QWidget>
#include <dust3d/base/part_target.h>
#include <dust3d/base/uuid.h>

class Document;
class ComponentPreviewGridWidget;
class ComponentPropertyWidget;
class QPushButton;

class PartManageWidget : public QWidget {
    Q_OBJECT
signals:
    void unselectAllOnCanvas();
    void selectPartOnCanvas(const dust3d::Uuid& partId);
    void setPartTarget(const dust3d::Uuid& partId, dust3d::PartTarget target);
    void groupComponents(const std::vector<dust3d::Uuid>& componentIds);
    void ungroupComponent(const dust3d::Uuid& componentId);
    void groupOperationAdded();
public slots:
    void selectComponentByPartId(const dust3d::Uuid& partId);
    void showSelectedComponentProperties();
    void showContextMenu(const QPoint& pos);

public:
    PartManageWidget(Document* document, QWidget* parent = nullptr);

private:
    Document* m_document = nullptr;
    ComponentPreviewGridWidget* m_componentPreviewGridWidget = nullptr;
    QPushButton* m_levelUpButton = nullptr;
    QPushButton* m_selectButton = nullptr;
    QPushButton* m_lockButton = nullptr;
    QPushButton* m_unlockButton = nullptr;
    QPushButton* m_showButton = nullptr;
    QPushButton* m_hideButton = nullptr;
    QPushButton* m_unlinkButton = nullptr;
    QPushButton* m_linkButton = nullptr;
    QPushButton* m_propertyButton = nullptr;
    void updateToolButtons();
    void updateLevelUpButton();
    bool hasSelectedGroupedComponent();
};

#endif
