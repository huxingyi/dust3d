#ifndef DUST3D_APPLICATION_COMPONENT_PROPERTY_WIDGET_H_
#define DUST3D_APPLICATION_COMPONENT_PROPERTY_WIDGET_H_

#include "document.h"
#include <QColorDialog>
#include <QWidget>
#include <dust3d/base/uuid.h>

class QPushButton;

class ComponentPropertyWidget : public QWidget {
    Q_OBJECT
signals:
    void beginColorPicking();
    void endColorPicking();
    void setComponentColorState(const dust3d::Uuid& componentId, bool hasColor, const QColor& color);
    void setPartDeformThickness(const dust3d::Uuid& partId, float thickness);
    void setPartDeformWidth(const dust3d::Uuid& partId, float width);
    void setPartDeformUnified(const dust3d::Uuid& partId, bool unified);
    void setPartSubdivState(const dust3d::Uuid& partId, bool subdived);
    void setPartChamferState(const dust3d::Uuid& partId, bool chamfered);
    void setPartRoundState(const dust3d::Uuid& partId, bool rounded);
    void setPartCutRotation(const dust3d::Uuid& partId, float cutRotation);
    void setComponentColorImage(const dust3d::Uuid& componentId, const dust3d::Uuid& imageId);
    void setComponentSideCloseState(const dust3d::Uuid& componentId, bool closed);
    void setComponentFrontCloseState(const dust3d::Uuid& componentId, bool closed);
    void setComponentBackCloseState(const dust3d::Uuid& componentId, bool closed);
    void setComponentTargetSegments(const dust3d::Uuid& componentId, size_t targetSegments);
    void setComponentSmoothCutoffDegrees(const dust3d::Uuid& partId, float degrees);
    void setPartCutFace(const dust3d::Uuid& partId, dust3d::CutFace cutFace);
    void setPartCutFaceLinkedId(const dust3d::Uuid& partId, dust3d::Uuid linkedId);
    void setPartXmirrorState(dust3d::Uuid partId, bool mirrored);
    void setComponentCombineMode(dust3d::Uuid componentId, dust3d::CombineMode combineMode);
    void groupOperationAdded();

public:
    ComponentPropertyWidget(Document* document,
        const std::vector<dust3d::Uuid>& componentIds,
        QWidget* parent = nullptr);
public slots:
    void showColorDialog();

private:
    Document* m_document = nullptr;
    std::vector<dust3d::Uuid> m_componentIds;
    std::vector<dust3d::Uuid> m_partIds;
    dust3d::Uuid m_partId;
    const Document::Part* m_part = nullptr;
    QColor m_color;
    QColor lastColor();
    dust3d::Uuid lastColorImageId();
    bool lastSideClosed();
    bool lastFrontClosed();
    bool lastBackClosed();
    size_t lastTargetSegments();
    float lastSmoothCutoffDegrees();
    void preparePartIds();
    void pickColorImageForComponents(const std::vector<dust3d::Uuid>& componentIds);
    std::vector<QPushButton*> m_cutFaceButtons;
    std::vector<QString> m_cutFaceList;
    std::unique_ptr<QColorDialog> m_colorDialog;
    std::map<dust3d::Uuid, QColor> m_componentsColorsBeforeColorPicking;

    void updateCutFaceButtonState(size_t index);
    bool hasStitchingLineConfigure();
};

#endif
