#ifndef MOTION_TIMELINE_WIDGET_H
#define MOTION_TIMELINE_WIDGET_H
#include <QListWidget>
#include <QUuid>
#include <QMouseEvent>
#include "skeletondocument.h"
#include "interpolationtype.h"

class MotionTimelineWidget : public QListWidget
{
    Q_OBJECT
signals:
    void clipsChanged();
public:
    MotionTimelineWidget(const SkeletonDocument *document, QWidget *parent=nullptr);
    const std::vector<SkeletonMotionClip> &clips();
    
public slots:
    void setClips(std::vector<SkeletonMotionClip> clips);
    void addPose(QUuid poseId);
    void addMotion(QUuid motionId);
    void reload();
    void setClipInterpolationType(int index, InterpolationType type);
    void setClipDuration(int index, float duration);
    void showInterpolationSettingPopup(int clipIndex, const QPoint &pos);
    void showContextMenu(const QPoint &pos);
    void setCurrentIndex(int index);
    void removeClip(int index);
    
protected:
    void mousePressEvent(QMouseEvent *event) override;
    QSize sizeHint() const override;
    
private:
    void addClipAfterCurrentIndex(const SkeletonMotionClip &clip);
    
    std::vector<SkeletonMotionClip> m_clips;
    const SkeletonDocument *m_document = nullptr;
    int m_currentSelectedIndex = -1;
};

#endif
