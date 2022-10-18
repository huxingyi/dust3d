#ifndef DUST3D_APPLICATION_MATERIAL_LIST_WIDGET_H_
#define DUST3D_APPLICATION_MATERIAL_LIST_WIDGET_H_

#include "material_widget.h"
#include <QMouseEvent>
#include <QTreeWidget>
#include <map>

class Document;

class MaterialListWidget : public QTreeWidget {
    Q_OBJECT
signals:
    void removeMaterial(dust3d::Uuid materialId);
    void modifyMaterial(dust3d::Uuid materialId);
    void cornerButtonClicked(dust3d::Uuid materialId);
    void currentSelectedMaterialChanged(dust3d::Uuid materialId);

public:
    MaterialListWidget(const Document* document, QWidget* parent = nullptr);
    bool isMaterialSelected(dust3d::Uuid materialId);
    void enableMultipleSelection(bool enabled);
public slots:
    void reload();
    void removeAllContent();
    void materialRemoved(dust3d::Uuid materialId);
    void showContextMenu(const QPoint& pos);
    void selectMaterial(dust3d::Uuid materialId, bool multiple = false);
    void copy();
    void setHasContextMenu(bool hasContextMenu);

protected:
    void resizeEvent(QResizeEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;

private:
    int calculateColumnCount();
    void updateMaterialSelectState(dust3d::Uuid materialId, bool selected);
    const Document* m_document = nullptr;
    std::map<dust3d::Uuid, std::pair<QTreeWidgetItem*, int>> m_itemMap;
    std::set<dust3d::Uuid> m_selectedMaterialIds;
    dust3d::Uuid m_currentSelectedMaterialId;
    dust3d::Uuid m_shiftStartMaterialId;
    bool m_hasContextMenu = true;
    bool m_multipleSelectionEnabled = true;
};

#endif
