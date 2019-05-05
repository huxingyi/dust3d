#ifndef DUST3D_PART_WIDGET_H
#define DUST3D_PART_WIDGET_H
#include <QLabel>
#include <QPushButton>
#include <QMouseEvent>
#include "document.h"
#include "modelwidget.h"

class PartWidget : public QWidget
{
    Q_OBJECT
signals:
    void setPartLockState(QUuid partId, bool locked);
    void setPartVisibleState(QUuid partId, bool visible);
    void setPartSubdivState(QUuid partId, bool subdived);
    void setPartDisableState(QUuid partId, bool disabled);
    void setPartXmirrorState(QUuid partId, bool mirrored);
    void setPartZmirrorState(QUuid partId, bool mirrored);
    void setPartDeformThickness(QUuid partId, float thickness);
    void setPartDeformWidth(QUuid partId, float width);
    void setPartRoundState(QUuid partId, bool rounded);
    void setPartChamferState(QUuid partId, bool chamfered);
    void setPartColorState(QUuid partId, bool hasColor, QColor color);
    void setPartCutRotation(QUuid partId, float cutRotation);
    void setPartCutFace(QUuid partId, CutFace cutFace);
    void setPartMaterialId(QUuid partId, QUuid materialId);
    void movePartUp(QUuid partId);
    void movePartDown(QUuid partId);
    void movePartToTop(QUuid partId);
    void movePartToBottom(QUuid partId);
    void checkPart(QUuid partId);
    void groupOperationAdded();
    void enableBackgroundBlur();
    void disableBackgroundBlur();
public:
    PartWidget(const Document *document, QUuid partId);
    void reload();
    void updatePreview();
    void updateLockButton();
    void updateVisibleButton();
    void updateSubdivButton();
    void updateDisableButton();
    void updateXmirrorButton();
    void updateZmirrorButton();
    void updateDeformButton();
    void updateRoundButton();
    void updateChamferButton();
    void updateColorButton();
    void updateCutRotationButton();
    void updateCheckedState(bool checked);
    void updateUnnormalState(bool unnormal);
    static QSize preferredSize();
    ModelWidget *previewWidget();
protected:
    void mouseDoubleClickEvent(QMouseEvent *event) override;
public slots:
    void showDeformSettingPopup(const QPoint &pos);
    void showCutRotationSettingPopup(const QPoint &pos);
    void showColorSettingPopup(const QPoint &pos);
private: // need initialize
    const Document *m_document;
    QUuid m_partId;
    bool m_unnormal;
private:
    ModelWidget *m_previewWidget;
    QPushButton *m_visibleButton;
    QPushButton *m_lockButton;
    QPushButton *m_subdivButton;
    QPushButton *m_disableButton;
    QPushButton *m_xMirrorButton;
    QPushButton *m_zMirrorButton;
    QPushButton *m_deformButton;
    QPushButton *m_roundButton;
    QPushButton *m_chamferButton;
    QPushButton *m_colorButton;
    QPushButton *m_cutRotationButton;
    QWidget *m_backgroundWidget;
private:
    void initToolButton(QPushButton *button);
    void initToolButtonWithoutFont(QPushButton *button);
    void initButton(QPushButton *button);
    void updateButton(QPushButton *button, QChar icon, bool highlighted);
    void updateAllButtons();
};

#endif
