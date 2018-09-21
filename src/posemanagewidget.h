#ifndef POSE_MANAGE_WIDGET_H
#define POSE_MANAGE_WIDGET_H
#include <QWidget>
#include "skeletondocument.h"
#include "poselistwidget.h"

class PoseManageWidget : public QWidget
{
    Q_OBJECT
signals:
    void registerDialog(QWidget *widget);
    void unregisterDialog(QWidget *widget);
public:
    PoseManageWidget(const SkeletonDocument *document, QWidget *parent=nullptr);
    PoseListWidget *poseListWidget();
protected:
    virtual QSize sizeHint() const;
public slots:
    void showAddPoseDialog();
    void showPoseDialog(QUuid poseId);
private:
    const SkeletonDocument *m_document = nullptr;
    PoseListWidget *m_poseListWidget = nullptr;
};

#endif
