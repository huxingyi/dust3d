#ifndef MATERIAL_LIST_WIDGET_H
#define MATERIAL_LIST_WIDGET_H
#include <QTreeWidget>
#include <map>
#include "skeletondocument.h"
#include "materialwidget.h"
#include "skeletongraphicswidget.h"

class MaterialListWidget : public QTreeWidget, public SkeletonGraphicsFunctions
{
    Q_OBJECT
signals:
    void removeMaterial(QUuid materialId);
    void modifyMaterial(QUuid materialId);
    void cornerButtonClicked(QUuid materialId);
    void currentSelectedMaterialChanged(QUuid materialId);
public:
    MaterialListWidget(const SkeletonDocument *document, QWidget *parent=nullptr);
    bool isMaterialSelected(QUuid materialId);
    void enableMultipleSelection(bool enabled);
public slots:
    void reload();
    void removeAllContent();
    void materialRemoved(QUuid materialId);
    void showContextMenu(const QPoint &pos);
    void selectMaterial(QUuid materialId, bool multiple=false);
    void copy();
    void setCornerButtonVisible(bool visible);
    void setHasContextMenu(bool hasContextMenu);
protected:
    void resizeEvent(QResizeEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    bool mouseMove(QMouseEvent *event) override;
    bool wheel(QWheelEvent *event) override;
    bool mouseRelease(QMouseEvent *event) override;
    bool mousePress(QMouseEvent *event) override;
    bool mouseDoubleClick(QMouseEvent *event) override;
    bool keyPress(QKeyEvent *event) override;
private:
    int calculateColumnCount();
    void updateMaterialSelectState(QUuid materialId, bool selected);
    const SkeletonDocument *m_document = nullptr;
    std::map<QUuid, std::pair<QTreeWidgetItem *, int>> m_itemMap;
    std::set<QUuid> m_selectedMaterialIds;
    QUuid m_currentSelectedMaterialId;
    QUuid m_shiftStartMaterialId;
    bool m_cornerButtonVisible = false;
    bool m_hasContextMenu = true;
    bool m_multipleSelectionEnabled = true;
};

#endif
