#ifndef DUST3D_MOTION_TIMELINE_WIDGET_H
#define DUST3D_MOTION_TIMELINE_WIDGET_H
#include <QListWidget>
#include <QUuid>
#include <QMouseEvent>
#include "document.h"
#include "interpolationtype.h"
#include "proceduralanimation.h"

class MotionTimelineWidget : public QListWidget
{
    Q_OBJECT
signals:
    void clipsChanged();
public:
    MotionTimelineWidget(const Document *document, QWidget *parent=nullptr);
    const std::vector<MotionClip> &clips();
    
public slots:
    void setClips(std::vector<MotionClip> clips);
    void addPose(QUuid poseId);
    void addMotion(QUuid motionId);
    void addProceduralAnimation(ProceduralAnimation proceduralAnimation);
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
    void addClipAfterCurrentIndex(const MotionClip &clip);
    
    std::vector<MotionClip> m_clips;
    const Document *m_document = nullptr;
    int m_currentSelectedIndex = -1;
};

#endif
