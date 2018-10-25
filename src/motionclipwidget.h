#ifndef DUST3D_MOTION_CLIP_WIDGET_H
#define DUST3D_MOTION_CLIP_WIDGET_H
#include <QFrame>
#include "document.h"

class MotionClipWidget : public QFrame
{
    Q_OBJECT
signals:
    void modifyInterpolation();
    
public:
    MotionClipWidget(const Document *document, QWidget *parent=nullptr);
    QSize preferredSize();
    static QSize maxSize();
    
public slots:
    void setClip(MotionClip clip);
    void reload();
    void updateCheckedState(bool checked);
    
private:
    const Document *m_document = nullptr;
    MotionClip m_clip;
    QWidget *m_reloadToWidget = nullptr;
};

#endif
