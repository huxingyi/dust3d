#ifndef MOTION_WIDGET_H
#define MOTION_WIDGET_H
#include <QFrame>
#include <QLabel>
#include <QIcon>
#include "skeletondocument.h"
#include "interpolationgraphicswidget.h"

class MotionWidget : public QFrame
{
    Q_OBJECT
signals:
    void modifyMotion(QUuid motionId);
public:
    MotionWidget(const SkeletonDocument *document, QUuid motionId);
    static int preferredHeight();
    InterpolationGraphicsWidget *previewWidget();
protected:
    void mouseDoubleClickEvent(QMouseEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
public slots:
    void reload();
    void updatePreview();
    void updateName();
    void updateCheckedState(bool checked);
private:
    QUuid m_motionId;
    const SkeletonDocument *m_document = nullptr;
    InterpolationGraphicsWidget *m_previewWidget = nullptr;
    QLabel *m_nameLabel = nullptr;
};

#endif
