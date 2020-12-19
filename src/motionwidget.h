#ifndef DUST3D_MOTION_WIDGET_H
#define DUST3D_MOTION_WIDGET_H
#include <QFrame>
#include <QLabel>
#include <QIcon>
#include <QPushButton>
#include "modelwidget.h"

class Document;

class MotionWidget : public QFrame
{
    Q_OBJECT
signals:
    void modifyMotion(QUuid motionId);
    void cornerButtonClicked(QUuid motionId);
public:
    MotionWidget(const Document *document, QUuid motionId);
    static int preferredHeight();
    ModelWidget *previewWidget();
protected:
    void mouseDoubleClickEvent(QMouseEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
public slots:
    void reload();
    void updatePreview();
    void updateName();
    void updateCheckedState(bool checked);
    void setCornerButtonVisible(bool visible);
private:
    QUuid m_motionId;
    const Document *m_document = nullptr;
    ModelWidget *m_previewWidget = nullptr;
    QLabel *m_nameLabel = nullptr;
    QPushButton *m_cornerButton = nullptr;
};

#endif
