#ifndef DUST3D_APPLICATION_BONE_MANAGE_WIDGET_H_
#define DUST3D_APPLICATION_BONE_MANAGE_WIDGET_H_

#include <QComboBox>
#include <QMenu>
#include <QWidget>
#include <QTreeView>
#include <QStandardItemModel>
#include <QPushButton>
#include <dust3d/base/uuid.h>
#include <memory>
#include <map>
#include <vector>
#include "bone_structure.h"
#include "rig_skeleton_mesh_generator.h"
#include "model_widget.h"

class Document;
class SkeletonGraphicsWidget;

class BoneManageWidget : public QWidget {
    Q_OBJECT

public slots:
    void showContextMenu(const QPoint& pos);
    void onRigTypeChanged(const QString& rigType);
    void onBoneSelectionChanged();
    void assignSelectedEdgesToBone();

public:
    BoneManageWidget(Document* document, QWidget* parent = nullptr);
    void setSkeletonGraphicsWidget(SkeletonGraphicsWidget* graphicsWidget);

private:
    Document* m_document = nullptr;
    SkeletonGraphicsWidget* m_skeletonGraphicsWidget = nullptr;
    QComboBox* m_rigTypeComboBox = nullptr;
    QTreeView* m_boneTreeView = nullptr;
    QStandardItemModel* m_boneTreeModel = nullptr;
    ModelWidget* m_modelWidget = nullptr;
    QPushButton* m_assignButton = nullptr;
    std::unique_ptr<QMenu> m_contextMenu;
    std::map<QString, RigStructure> m_rigStructures;
    QString m_selectedBoneName;
    
    void loadRigStructures();
    bool loadRigFromXml(const QString& filePath);
    void updateBoneTreeView(const QString& rigType);
    void generateRigSkeletonMesh(const QString& rigType, const QString& selectedBoneName = "");
};

#endif
