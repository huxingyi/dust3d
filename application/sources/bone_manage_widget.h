#ifndef DUST3D_APPLICATION_BONE_MANAGE_WIDGET_H_
#define DUST3D_APPLICATION_BONE_MANAGE_WIDGET_H_

#include <QComboBox>
#include <QMenu>
#include <QWidget>
#include <QTreeView>
#include <QThread>
#include <QStandardItemModel>
#include <QPushButton>
#include <memory>
#include <map>
#include <vector>
#include "bone_structure.h"
#include "rig_skeleton_mesh_generator.h"
#include "rig_skeleton_mesh_worker.h"
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
    void onRigSkeletonMeshReady();
    void onMeshGenerationThreadFinished();

public:
    BoneManageWidget(Document* document, QWidget* parent = nullptr);
    ~BoneManageWidget();
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
    std::unique_ptr<RigSkeletonMeshWorker> m_meshWorker;
    QThread* m_meshGenerationThread = nullptr;
    bool m_rigSkeletonMeshObsolete = false;
    QString m_pendingRigType;
    QString m_pendingSelectedBoneName;
    
    void loadRigStructures();
    bool loadRigFromXml(const QString& filePath);
    void updateBoneTreeView(const QString& rigType);
    void generateRigSkeletonMesh(const QString& rigType, const QString& selectedBoneName = "");
};

#endif
