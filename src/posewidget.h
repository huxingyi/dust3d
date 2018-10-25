#ifndef DUST3D_POSE_WIDGET_H
#define DUST3D_POSE_WIDGET_H
#include <QFrame>
#include <QLabel>
#include <QIcon>
#include "document.h"
#include "modelwidget.h"

class PoseWidget : public QFrame
{
    Q_OBJECT
signals:
    void modifyPose(QUuid poseId);
    void cornerButtonClicked(QUuid poseId);
public:
    PoseWidget(const Document *document, QUuid poseId);
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
    QUuid m_poseId;
    const Document *m_document = nullptr;
    ModelWidget *m_previewWidget = nullptr;
    QLabel *m_nameLabel = nullptr;
    QPushButton *m_cornerButton = nullptr;
};

#endif
