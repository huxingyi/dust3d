#ifndef MOTION_MANAGE_WIDGET_H
#define MOTION_MANAGE_WIDGET_H
#include <QWidget>
#include "skeletondocument.h"
#include "motionlistwidget.h"

class MotionManageWidget : public QWidget
{
    Q_OBJECT
signals:
    void registerDialog(QWidget *widget);
    void unregisterDialog(QWidget *widget);
public:
    MotionManageWidget(const SkeletonDocument *document, QWidget *parent=nullptr);
protected:
    virtual QSize sizeHint() const;
public slots:
    void showAddMotionDialog();
    void showMotionDialog(QUuid motionId);
private:
    const SkeletonDocument *m_document = nullptr;
    MotionListWidget *m_motionListWidget = nullptr;
};

#endif
