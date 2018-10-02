#ifndef MOTION_LIST_WIDGET_H
#define MOTION_LIST_WIDGET_H
#include <QTreeWidget>
#include <map>
#include "skeletondocument.h"
#include "motionwidget.h"
#include "skeletongraphicswidget.h"

class MotionListWidget : public QTreeWidget, public SkeletonGraphicsFunctions
{
    Q_OBJECT
signals:
    void removeMotion(QUuid motionId);
    void modifyMotion(QUuid motionId);
public:
    MotionListWidget(const SkeletonDocument *document, QWidget *parent=nullptr);
    bool isMotionSelected(QUuid motionId);
public slots:
    void reload();
    void removeAllContent();
    void motionRemoved(QUuid motionId);
    void showContextMenu(const QPoint &pos);
    void selectMotion(QUuid motionId, bool multiple=false);
    void copy();
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
    void updateMotionSelectState(QUuid motionId, bool selected);
    const SkeletonDocument *m_document = nullptr;
    std::map<QUuid, std::pair<QTreeWidgetItem *, int>> m_itemMap;
    std::set<QUuid> m_selectedMotionIds;
    QUuid m_currentSelectedMotionId;
    QUuid m_shiftStartMotionId;
};

#endif
