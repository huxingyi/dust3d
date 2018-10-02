#ifndef MOTION_EDIT_WIDGET_H
#define MOTION_EDIT_WIDGET_H
#include <QDialog>
#include <QLineEdit>
#include <QCloseEvent>
#include "skeletondocument.h"
#include "interpolationgraphicswidget.h"
#include "modelwidget.h"
#include "curveutil.h"
#include "motionpreviewsgenerator.h"
#include "animationclipplayer.h"

class MotionEditWidget : public QDialog
{
    Q_OBJECT
signals:
    void addMotion(QString name, std::vector<HermiteControlNode> controlNodes, std::vector<std::pair<float, QUuid>> keyframes);
    void setMotionControlNodes(QUuid motionId, std::vector<HermiteControlNode> controlNodes);
    void setMotionKeyframes(QUuid motionId, std::vector<std::pair<float, QUuid>> keyframes);
    void renameMotion(QUuid motionId, QString name);
public:
    MotionEditWidget(const SkeletonDocument *document, QWidget *parent=nullptr);
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
    void setEditMotionControlNodes(std::vector<HermiteControlNode> controlNodes);
    void setEditMotionKeyframes(std::vector<std::pair<float, QUuid>> keyframes);
    void setUnsavedState();
    void addKeyframe(float knot, QUuid poseId);
    void syncKeyframesToGraphicsView();
    void generatePreviews();
    void previewsReady();
    void updateKeyframeKnot(int index, float knot);
private:
    const SkeletonDocument *m_document = nullptr;
    InterpolationGraphicsWidget *m_graphicsWidget = nullptr;
    ModelWidget *m_previewWidget = nullptr;
    QUuid m_motionId;
    QLineEdit *m_nameEdit = nullptr;
    std::vector<std::pair<float, QUuid>> m_keyframes;
    bool m_unsaved = false;
    bool m_closed = false;
    bool m_isPreviewsObsolete = false;
    MotionPreviewsGenerator *m_previewsGenerator = nullptr;
    AnimationClipPlayer *m_clipPlayer = nullptr;
};

#endif
