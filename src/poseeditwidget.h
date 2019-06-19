#ifndef DUST3D_POSE_EDIT_WIDGET_H
#define DUST3D_POSE_EDIT_WIDGET_H
#include <QDialog>
#include <map>
#include <QCloseEvent>
#include <QLineEdit>
#include <QSlider>
#include <QDoubleSpinBox>
#include "posepreviewmanager.h"
#include "document.h"
#include "modelwidget.h"
#include "rigger.h"
#include "skeletongraphicswidget.h"
#include "posedocument.h"
#include "floatnumberwidget.h"
#include "skeletonside.h"

class PoseEditWidget : public QDialog
{
    Q_OBJECT
signals:
    void addPose(QUuid poseId, QString name, std::vector<std::pair<std::map<QString, QString>, std::map<QString, std::map<QString, QString>>>> frames, QUuid turnaroundImageId, float yTranslationScale);
    void removePose(QUuid poseId);
    void setPoseFrames(QUuid poseId, std::vector<std::pair<std::map<QString, QString>, std::map<QString, std::map<QString, QString>>>> frames);
    void setPoseTurnaroundImageId(QUuid poseId, QUuid imageId);
    void setPoseYtranslationScale(QUuid poseId, float scale);
    void renamePose(QUuid poseId, QString name);
    void parametersAdjusted();
public:
    PoseEditWidget(const Document *document, QWidget *parent=nullptr);
    ~PoseEditWidget();
    
    float m_duration = 1.0;
public slots:
    void updatePoseDocument();
    void updatePreview();
    void syncFrameFromCurrent();
    void setEditPoseId(QUuid poseId);
    void setEditPoseName(QString name);
    void setEditPoseFrames(std::vector<std::pair<std::map<QString, QString>, std::map<QString, std::map<QString, QString>>>> frames);
    void setEditPoseTurnaroundImageId(QUuid imageId);
    void setEditPoseYtranslationScale(float yTranslationScale);
    void setCurrentFrame(int frame);
    void insertFrameAfterCurrentFrame();
    void removeCurrentFrame();
    void setDuration(float duration);
    void updateTitle();
    void save();
    void clearUnsaveState();
    void setUnsaveState();
    void changeTurnaround();
private slots:
    void updateFramesSettingButton();
    void showFramesSettingPopup(const QPoint &pos);
    void showPoseSettingPopup(const QPoint &pos);
    void updateFramesDurations();
protected:
    QSize sizeHint() const override;
    void closeEvent(QCloseEvent *event) override;
    void reject() override;
private:
    void ensureEnoughFrames();
    const Document *m_document = nullptr;
    PosePreviewManager *m_posePreviewManager = nullptr;
    ModelWidget *m_previewWidget = nullptr;
    bool m_isPreviewDirty = false;
    bool m_closed = false;
    std::vector<std::pair<std::map<QString, QString>, std::map<QString, std::map<QString, QString>>>> m_frames;
    std::map<QString, QString> m_currentAttributes;
    std::map<QString, std::map<QString, QString>> m_currentParameters;
    std::vector<std::map<QString, std::map<QString, QString>>> m_otherFramesParameters;
    int m_currentFrame = 0;
    QUuid m_poseId;
    bool m_unsaved = false;
    QUuid m_imageId;
    float m_yTranslationScale = 1.0;
    QLineEdit *m_nameEdit = nullptr;
    QDoubleSpinBox *m_durationEdit = nullptr;
    PoseDocument *m_poseDocument = nullptr;
    SkeletonGraphicsWidget *m_poseGraphicsWidget = nullptr;
    QPushButton *m_framesSettingButton = nullptr;
    QSlider *m_currentFrameSlider = nullptr;
    static float m_defaultBlur;
    void initSideButton(QPushButton *button);
    void updateSideButtonState(QPushButton *button, bool visible);
};

#endif
