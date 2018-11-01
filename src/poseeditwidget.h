#ifndef DUST3D_POSE_EDIT_WIDGET_H
#define DUST3D_POSE_EDIT_WIDGET_H
#include <QDialog>
#include <map>
#include <QCloseEvent>
#include <QLineEdit>
#include "posepreviewmanager.h"
#include "document.h"
#include "modelwidget.h"
#include "rigger.h"

typedef RiggerButtonParameterType PopupWidgetType;

class PoseEditWidget : public QDialog
{
    Q_OBJECT
signals:
    void addPose(QString name, std::map<QString, std::map<QString, QString>> parameters);
    void removePose(QUuid poseId);
    void setPoseParameters(QUuid poseId, std::map<QString, std::map<QString, QString>> parameters);
    void renamePose(QUuid poseId, QString name);
    void parametersAdjusted();
public:
    PoseEditWidget(const Document *document, QWidget *parent=nullptr);
    ~PoseEditWidget();
public slots:
    void updateButtons();
    void updatePreview();
    void showPopupAngleDialog(QString boneName, PopupWidgetType popupWidgetType, QPoint pos);
    void setEditPoseId(QUuid poseId);
    void setEditPoseName(QString name);
    void setEditParameters(std::map<QString, std::map<QString, QString>> parameters);
    void updateTitle();
    void save();
    void clearUnsaveState();
protected:
    QSize sizeHint() const override;
    void closeEvent(QCloseEvent *event) override;
    void reject() override;
private:
    const Document *m_document = nullptr;
    PosePreviewManager *m_posePreviewManager = nullptr;
    ModelWidget *m_previewWidget = nullptr;
    bool m_isPreviewDirty = false;
    bool m_closed = false;
    std::map<QString, std::map<QString, QString>> m_parameters;
    size_t m_openedMenuCount = 0;
    QUuid m_poseId;
    bool m_unsaved = false;
    QLineEdit *m_nameEdit = nullptr;
    QWidget *m_buttonsContainer = nullptr;
};

#endif
