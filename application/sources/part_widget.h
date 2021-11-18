#ifndef DUST3D_APPLICATION_PART_WIDGET_H_
#define DUST3D_APPLICATION_PART_WIDGET_H_

#include <QLabel>
#include <QPushButton>
#include <QMouseEvent>
#include <dust3d/base/cut_face.h>
#include <dust3d/base/uuid.h>

class Document;

class PartWidget : public QWidget
{
    Q_OBJECT
signals:
    void setPartLockState(dust3d::Uuid partId, bool locked);
    void setPartVisibleState(dust3d::Uuid partId, bool visible);
    void setPartSubdivState(dust3d::Uuid partId, bool subdived);
    void setPartDisableState(dust3d::Uuid partId, bool disabled);
    void setPartXmirrorState(dust3d::Uuid partId, bool mirrored);
    void setPartZmirrorState(dust3d::Uuid partId, bool mirrored);
    void setPartDeformThickness(dust3d::Uuid partId, float thickness);
    void setPartDeformWidth(dust3d::Uuid partId, float width);
    void setPartDeformUnified(dust3d::Uuid partId, bool unified);
    void setPartRoundState(dust3d::Uuid partId, bool rounded);
    void setPartChamferState(dust3d::Uuid partId, bool chamfered);
    void setPartColorState(dust3d::Uuid partId, bool hasColor, QColor color);
    void setPartCutRotation(dust3d::Uuid partId, float cutRotation);
    void setPartCutFace(dust3d::Uuid partId, dust3d::CutFace cutFace);
    void setPartCutFaceLinkedId(dust3d::Uuid partId, dust3d::Uuid linkedId);
    void setPartMaterialId(dust3d::Uuid partId, dust3d::Uuid materialId);
    void setPartColorSolubility(dust3d::Uuid partId, float colorSolubility);
    void setPartMetalness(dust3d::Uuid partId, float metalness);
    void setPartRoughness(dust3d::Uuid partId, float roughness);
    void setPartHollowThickness(dust3d::Uuid partId, float hollowThickness);
    void setPartCountershaded(dust3d::Uuid partId, bool countershaded);
    void setPartSmoothState(dust3d::Uuid partId, bool smooth);
    void movePartUp(dust3d::Uuid partId);
    void movePartDown(dust3d::Uuid partId);
    void movePartToTop(dust3d::Uuid partId);
    void movePartToBottom(dust3d::Uuid partId);
    void checkPart(dust3d::Uuid partId);
    void groupOperationAdded();
    void enableBackgroundBlur();
    void disableBackgroundBlur();
public:
    PartWidget(const Document *document, dust3d::Uuid partId);
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
protected:
    //void mouseDoubleClickEvent(QMouseEvent *event) override;
public slots:
    void showDeformSettingPopup(const QPoint &pos);
    void showCutRotationSettingPopup(const QPoint &pos);
    void showColorSettingPopup(const QPoint &pos);
private:
    const Document *m_document = nullptr;
    dust3d::Uuid m_partId;
    bool m_unnormal = false;
private:
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
