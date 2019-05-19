#ifndef DUST3D_CUT_FACE_LIST_WIDGET_H
#define DUST3D_CUT_FACE_LIST_WIDGET_H
#include <QTreeWidget>
#include <map>
#include <QMouseEvent>
#include "document.h"
#include "cutfacewidget.h"

class CutFaceListWidget : public QTreeWidget
{
    Q_OBJECT
signals:
    void currentSelectedCutFaceChanged(QUuid partId);
public:
    CutFaceListWidget(const Document *document, QWidget *parent=nullptr);
    bool isCutFaceSelected(QUuid partId);
    void enableMultipleSelection(bool enabled);
    bool isEmpty();
public slots:
    void reload();
    void removeAllContent();
    void showContextMenu(const QPoint &pos);
    void selectCutFace(QUuid partId, bool multiple=false);
    void setHasContextMenu(bool hasContextMenu);
protected:
    void resizeEvent(QResizeEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
private:
    int calculateColumnCount();
    void updateCutFaceSelectState(QUuid partId, bool selected);
    const Document *m_document = nullptr;
    std::map<QUuid, std::pair<QTreeWidgetItem *, int>> m_itemMap;
    std::set<QUuid> m_selectedPartIds;
    QUuid m_currentSelectedPartId;
    QUuid m_shiftStartPartId;
    bool m_hasContextMenu = false;
    bool m_multipleSelectionEnabled = false;
    std::vector<QUuid> m_cutFacePartIdList;
};

#endif
