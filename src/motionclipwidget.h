#ifndef MOTION_CLIP_WIDGET_H
#define MOTION_CLIP_WIDGET_H
#include <QFrame>
#include "skeletondocument.h"

class MotionClipWidget : public QFrame
{
    Q_OBJECT
signals:
    void modifyInterpolation();
    
public:
    MotionClipWidget(const SkeletonDocument *document, QWidget *parent=nullptr);
    QSize preferredSize();
    static QSize maxSize();
    
public slots:
    void setClip(SkeletonMotionClip clip);
    void reload();
    void updateCheckedState(bool checked);
    
private:
    const SkeletonDocument *m_document = nullptr;
    SkeletonMotionClip m_clip;
    QWidget *m_reloadToWidget = nullptr;
};

#endif
