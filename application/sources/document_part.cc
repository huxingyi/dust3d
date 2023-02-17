#include "document.h"

Document::Part::Part(const dust3d::Uuid& withId)
    : visible(true)
    , locked(false)
    , subdived(false)
    , disabled(false)
    , xMirrored(false)
    , deformThickness(1.0)
    , deformWidth(1.0)
    , deformUnified(false)
    , rounded(false)
    , chamfered(false)
    , dirty(true)
    , cutRotation(0.0)
    , cutFace(dust3d::CutFace::Quad)
    , target(dust3d::PartTarget::Model)
    , metalness(0.0)
    , roughness(1.0)
    , hollowThickness(0.0)
    , countershaded(false)
{
    id = withId.isNull() ? dust3d::Uuid::createUuid() : withId;
}

void Document::Part::setDeformThickness(float toThickness)
{
    if (toThickness < 0)
        toThickness = 0;
    else if (toThickness > 2)
        toThickness = 2;
    deformThickness = toThickness;
}

void Document::Part::setDeformWidth(float toWidth)
{
    if (toWidth < 0)
        toWidth = 0;
    else if (toWidth > 2)
        toWidth = 2;
    deformWidth = toWidth;
}

void Document::Part::setCutRotation(float toRotation)
{
    if (toRotation < -1)
        toRotation = -1;
    else if (toRotation > 1)
        toRotation = 1;
    cutRotation = toRotation;
}

void Document::Part::setCutFace(dust3d::CutFace face)
{
    cutFace = face;
    cutFaceLinkedId = dust3d::Uuid();
}

void Document::Part::setCutFaceLinkedId(const dust3d::Uuid& linkedId)
{
    if (linkedId.isNull()) {
        setCutFace(dust3d::CutFace::Quad);
        return;
    }
    cutFace = dust3d::CutFace::UserDefined;
    cutFaceLinkedId = linkedId;
}

bool Document::Part::deformThicknessAdjusted() const
{
    return fabs(deformThickness - 1.0) >= 0.01;
}

bool Document::Part::deformWidthAdjusted() const
{
    return fabs(deformWidth - 1.0) >= 0.01;
}

bool Document::Part::deformAdjusted() const
{
    return deformThicknessAdjusted() || deformWidthAdjusted() || deformUnified;
}

bool Document::Part::metalnessAdjusted() const
{
    return fabs(metalness - 0.0) >= 0.01;
}

bool Document::Part::roughnessAdjusted() const
{
    return fabs(roughness - 1.0) >= 0.01;
}

bool Document::Part::cutRotationAdjusted() const
{
    return fabs(cutRotation - 0.0) >= 0.01;
}

bool Document::Part::hollowThicknessAdjusted() const
{
    return fabs(hollowThickness - 0.0) >= 0.01;
}

bool Document::Part::cutFaceAdjusted() const
{
    return cutFace != dust3d::CutFace::Quad;
}

bool Document::Part::cutAdjusted() const
{
    return cutRotationAdjusted() || cutFaceAdjusted() || hollowThicknessAdjusted();
}

bool Document::Part::isEditVisible() const
{
    return visible && !disabled;
}

void Document::Part::copyAttributes(const Part& other)
{
    visible = other.visible;
    locked = other.locked;
    subdived = other.subdived;
    disabled = other.disabled;
    xMirrored = other.xMirrored;
    deformThickness = other.deformThickness;
    deformWidth = other.deformWidth;
    rounded = other.rounded;
    chamfered = other.chamfered;
    cutRotation = other.cutRotation;
    cutFace = other.cutFace;
    cutFaceLinkedId = other.cutFaceLinkedId;
    componentId = other.componentId;
    dirty = other.dirty;
    target = other.target;
    countershaded = other.countershaded;
    metalness = other.metalness;
    roughness = other.roughness;
    deformUnified = other.deformUnified;
    hollowThickness = other.hollowThickness;
}
