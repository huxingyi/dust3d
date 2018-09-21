#ifndef POSE_LIST_WIDGET_H
#define POSE_LIST_WIDGET_H
#include <QTreeWidget>
#include <map>
#include "skeletondocument.h"
#include "posewidget.h"
#include "skeletongraphicswidget.h"

class PoseListWidget : public QTreeWidget, public SkeletonGraphicsFunctions
{
    Q_OBJECT
signals:
    void removePose(QUuid poseId);
    void modifyPose(QUuid poseId);
public:
    PoseListWidget(const SkeletonDocument *document, QWidget *parent=nullptr);
    bool isPoseSelected(QUuid poseId);
public slots:
    void reload();
    void removeAllContent();
    void poseRemoved(QUuid poseId);
    void showContextMenu(const QPoint &pos);
    void selectPose(QUuid poseId, bool multiple=false);
    void copy();
protected:
    void resizeEvent(QResizeEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    bool mouseMove(QMouseEvent *event);
    bool wheel(QWheelEvent *event);
    bool mouseRelease(QMouseEvent *event);
    bool mousePress(QMouseEvent *event);
    bool mouseDoubleClick(QMouseEvent *event);
    bool keyPress(QKeyEvent *event);
private:
    int calculateColumnCount();
    void updatePoseSelectState(QUuid poseId, bool selected);
    const SkeletonDocument *m_document = nullptr;
    std::map<QUuid, std::pair<QTreeWidgetItem *, int>> m_itemMap;
    std::set<QUuid> m_selectedPoseIds;
    QUuid m_currentSelectedPoseId;
    QUuid m_shiftStartPoseId;
};

#endif
