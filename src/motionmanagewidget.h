#ifndef DUST3D_MOTION_MANAGE_WIDGET_H
#define DUST3D_MOTION_MANAGE_WIDGET_H
#include <QWidget>
#include "motionlistwidget.h"

class Document;

class MotionManageWidget : public QWidget
{
    Q_OBJECT
signals:
    void registerDialog(QWidget *widget);
    void unregisterDialog(QWidget *widget);
public:
    MotionManageWidget(const Document *document, QWidget *parent=nullptr);
protected:
    virtual QSize sizeHint() const;
public slots:
    void showAddMotionDialog();
    void showMotionDialog(QUuid motionId);
private:
    const Document *m_document = nullptr;
    MotionListWidget *m_motionListWidget = nullptr;
};

#endif
