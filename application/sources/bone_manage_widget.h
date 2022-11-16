#ifndef DUST3D_APPLICATION_BONE_MANAGE_WIDGET_H_
#define DUST3D_APPLICATION_BONE_MANAGE_WIDGET_H_

#include <QMenu>
#include <QWidget>
#include <dust3d/base/uuid.h>
#include <memory>

class Document;
class BonePreviewGridWidget;
class BonePropertyWidget;
class QPushButton;

class BoneManageWidget : public QWidget {
    Q_OBJECT
signals:
    void unselectAllOnCanvas();
    void selectNodeOnCanvas(const dust3d::Uuid& nodeId);
    void groupOperationAdded();
public slots:
    void selectBoneByBoneId(const dust3d::Uuid& boneId);
    void showSelectedBoneProperties();
    void showContextMenu(const QPoint& pos);

public:
    BoneManageWidget(Document* document, QWidget* parent = nullptr);

private:
    Document* m_document = nullptr;
    BonePreviewGridWidget* m_bonePreviewGridWidget = nullptr;
    QPushButton* m_addButton = nullptr;
    QPushButton* m_selectButton = nullptr;
    QPushButton* m_propertyButton = nullptr;
    std::unique_ptr<QMenu> m_propertyMenu;
    void updateToolButtons();
};

#endif
