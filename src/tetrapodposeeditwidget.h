#ifndef TETRAPOD_POSE_EDIT_WIDGET_H
#define TETRAPOD_POSE_EDIT_WIDGET_H
#include <QWidget>
#include <map>
#include <QPointF>
#include "posepreviewmanager.h"
#include "tetrapodposer.h"
#include "skeletondocument.h"

enum class PopupWidgetType
{
    PitchYawRoll,
    Intersection
};

class TetrapodPoseEditWidget : public QWidget
{
    Q_OBJECT
public:
    TetrapodPoseEditWidget(const SkeletonDocument *document, QWidget *parent=nullptr);
    ~TetrapodPoseEditWidget();
public slots:
    void updatePreview();
    void showPopupAngleDialog(QString boneName, PopupWidgetType popupWidgetType, QPoint pos);
private:
    const SkeletonDocument *m_document = nullptr;
    PosePreviewManager *m_posePreviewManager = nullptr;
    TetrapodPoser *m_poser = nullptr;
    bool m_isPreviewDirty = false;
    std::map<QString, std::map<QString, QString>> m_parameters;
};

#endif
