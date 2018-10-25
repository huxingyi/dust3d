#ifndef DUST3D_MOTION_LIST_WIDGET_H
#define DUST3D_MOTION_LIST_WIDGET_H
#include <QTreeWidget>
#include <map>
#include <QMouseEvent>
#include "document.h"
#include "motionwidget.h"

class MotionListWidget : public QTreeWidget
{
    Q_OBJECT
signals:
    void removeMotion(QUuid motionId);
    void modifyMotion(QUuid motionId);
    void cornerButtonClicked(QUuid motionId);
public:
    MotionListWidget(const Document *document, QWidget *parent=nullptr);
    bool isMotionSelected(QUuid motionId);
public slots:
    void reload();
    void removeAllContent();
    void motionRemoved(QUuid motionId);
    void showContextMenu(const QPoint &pos);
    void selectMotion(QUuid motionId, bool multiple=false);
    void copy();
    void setCornerButtonVisible(bool visible);
    void setHasContextMenu(bool hasContextMenu);
protected:
    void resizeEvent(QResizeEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
private:
    int calculateColumnCount();
    void updateMotionSelectState(QUuid motionId, bool selected);
    const Document *m_document = nullptr;
    std::map<QUuid, std::pair<QTreeWidgetItem *, int>> m_itemMap;
    std::set<QUuid> m_selectedMotionIds;
    QUuid m_currentSelectedMotionId;
    QUuid m_shiftStartMotionId;
    bool m_cornerButtonVisible = false;
    bool m_hasContextMenu = true;
};

#endif
