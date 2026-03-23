#ifndef DUST3D_APPLICATION_BONE_MANAGE_WIDGET_H_
#define DUST3D_APPLICATION_BONE_MANAGE_WIDGET_H_

#include <QComboBox>
#include <QMenu>
#include <QWidget>
#include <QTreeView>
#include <QStandardItemModel>
#include <dust3d/base/uuid.h>
#include <memory>
#include <map>
#include <vector>

class Document;

struct BoneNode {
    QString name;
    QString parent;
    float posX, posY, posZ;
    float endX, endY, endZ;
};

struct RigStructure {
    QString type;
    QString name;
    QString description;
    std::vector<BoneNode> bones;
};

class BoneManageWidget : public QWidget {
    Q_OBJECT

public slots:
    void showContextMenu(const QPoint& pos);
    void onRigTypeChanged(const QString& rigType);

public:
    BoneManageWidget(Document* document, QWidget* parent = nullptr);

private:
    Document* m_document = nullptr;
    QComboBox* m_rigTypeComboBox = nullptr;
    QTreeView* m_boneTreeView = nullptr;
    QStandardItemModel* m_boneTreeModel = nullptr;
    std::unique_ptr<QMenu> m_contextMenu;
    std::map<QString, RigStructure> m_rigStructures;
    
    void loadRigStructures();
    bool loadRigFromXml(const QString& filePath);
    void updateBoneTreeView(const QString& rigType);
};

#endif
