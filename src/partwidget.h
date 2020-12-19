#ifndef DUST3D_PART_WIDGET_H
#define DUST3D_PART_WIDGET_H
#include <QLabel>
#include <QPushButton>
#include <QMouseEvent>
#include "modelwidget.h"
#include "cutface.h"

class Document;

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
    void setPartDeformUnified(QUuid partId, bool unified);
    void setPartDeformMapImageId(QUuid partId, QUuid imageId);
    void setPartDeformMapScale(QUuid partId, float scale);
    void setPartRoundState(QUuid partId, bool rounded);
    void setPartChamferState(QUuid partId, bool chamfered);
    void setPartColorState(QUuid partId, bool hasColor, QColor color);
    void setPartCutRotation(QUuid partId, float cutRotation);
    void setPartCutFace(QUuid partId, CutFace cutFace);
    void setPartCutFaceLinkedId(QUuid partId, QUuid linkedId);
    void setPartMaterialId(QUuid partId, QUuid materialId);
    void setPartColorSolubility(QUuid partId, float colorSolubility);
    void setPartMetalness(QUuid partId, float metalness);
    void setPartRoughness(QUuid partId, float roughness);
    void setPartHollowThickness(QUuid partId, float hollowThickness);
    void setPartCountershaded(QUuid partId, bool countershaded);
    void setPartSmoothState(QUuid partId, bool smooth);
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
    void updateSmoothButton();
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
    //void mouseDoubleClickEvent(QMouseEvent *event) override;
public slots:
    void showDeformSettingPopup(const QPoint &pos);
    void showCutRotationSettingPopup(const QPoint &pos);
    void showColorSettingPopup(const QPoint &pos);
private:
    const Document *m_document = nullptr;
    QUuid m_partId;
    bool m_unnormal = false;
private:
    ModelWidget *m_previewWidget = nullptr;
    QPushButton *m_visibleButton = nullptr;
    QPushButton *m_smoothButton = nullptr;
    QPushButton *m_lockButton = nullptr;
    QPushButton *m_subdivButton = nullptr;
    QPushButton *m_disableButton = nullptr;
    QPushButton *m_xMirrorButton = nullptr;
    QPushButton *m_zMirrorButton = nullptr;
    QPushButton *m_deformButton = nullptr;
    QPushButton *m_roundButton = nullptr;
    QPushButton *m_chamferButton = nullptr;
    QPushButton *m_colorButton = nullptr;
    QPushButton *m_cutRotationButton = nullptr;
    QWidget *m_backgroundWidget = nullptr;
    QLabel *m_previewLabel = nullptr;
private:
    void initToolButton(QPushButton *button);
    void initToolButtonWithoutFont(QPushButton *button);
    void initButton(QPushButton *button);
    void updateButton(QPushButton *button, QChar icon, bool highlighted, bool enabled=true);
    void updateAllButtons();
};

#endif
