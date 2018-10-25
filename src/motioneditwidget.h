#ifndef DUST3D_MOTION_EDIT_WIDGET_H
#define DUST3D_MOTION_EDIT_WIDGET_H
#include <QDialog>
#include <QLineEdit>
#include <QCloseEvent>
#include "document.h"
#include "motiontimelinewidget.h"
#include "modelwidget.h"
#include "motionsgenerator.h"
#include "animationclipplayer.h"

class MotionEditWidget : public QDialog
{
    Q_OBJECT
signals:
    void addMotion(QString name, std::vector<MotionClip> clips);
    void setMotionClips(QUuid motionId, std::vector<MotionClip> clips);
    void renameMotion(QUuid motionId, QString name);
public:
    MotionEditWidget(const Document *document, QWidget *parent=nullptr);
    ~MotionEditWidget();
protected:
    void closeEvent(QCloseEvent *event) override;
    void reject() override;
    QSize sizeHint() const override;
public slots:
    void updateTitle();
    void save();
    void clearUnsaveState();
    void setEditMotionId(QUuid poseId);
    void setEditMotionName(QString name);
    void setEditMotionClips(std::vector<MotionClip> clips);
    void setUnsavedState();
    void generatePreviews();
    void previewsReady();
private:
    const Document *m_document = nullptr;
    MotionTimelineWidget *m_timelineWidget = nullptr;
    ModelWidget *m_previewWidget = nullptr;
    QUuid m_motionId;
    QLineEdit *m_nameEdit = nullptr;
    std::vector<std::pair<float, QUuid>> m_keyframes;
    bool m_unsaved = false;
    bool m_closed = false;
    bool m_isPreviewsObsolete = false;
    MotionsGenerator *m_previewsGenerator = nullptr;
    AnimationClipPlayer *m_clipPlayer = nullptr;
};

#endif
