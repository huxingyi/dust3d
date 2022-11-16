#include "document.h"
#include "mesh_generator.h"

Document::Node::Node(const dust3d::Uuid& withId)
{
    id = withId.isNull() ? dust3d::Uuid::createUuid() : withId;
}

void Document::Node::setRadius(float toRadius)
{
    if (toRadius < MeshGenerator::m_minimalRadius)
        toRadius = MeshGenerator::m_minimalRadius;
    else if (toRadius > 1)
        toRadius = 1;
    radius = toRadius;
}

void Document::Node::setCutRotation(float toRotation)
{
    if (toRotation < -1)
        toRotation = -1;
    else if (toRotation > 1)
        toRotation = 1;
    cutRotation = toRotation;
    hasCutFaceSettings = true;
}

void Document::Node::setCutFace(dust3d::CutFace face)
{
    cutFace = face;
    cutFaceLinkedId = dust3d::Uuid();
    hasCutFaceSettings = true;
}

void Document::Node::setCutFaceLinkedId(const dust3d::Uuid& linkedId)
{
    if (linkedId.isNull()) {
        clearCutFaceSettings();
        return;
    }
    cutFace = dust3d::CutFace::UserDefined;
    cutFaceLinkedId = linkedId;
    hasCutFaceSettings = true;
}

void Document::Node::clearCutFaceSettings()
{
    cutFace = dust3d::CutFace::Quad;
    cutFaceLinkedId = dust3d::Uuid();
    cutRotation = 0;
    hasCutFaceSettings = false;
}

float Document::Node::getX(bool rotated) const
{
    if (rotated)
        return m_y;
    return m_x;
}

float Document::Node::getY(bool rotated) const
{
    if (rotated)
        return m_x;
    return m_y;
}

float Document::Node::getZ(bool rotated) const
{
    (void)rotated;
    return m_z;
}

void Document::Node::setX(float x)
{
    m_x = x;
}

void Document::Node::setY(float y)
{
    m_y = y;
}

void Document::Node::setZ(float z)
{
    m_z = z;
}

void Document::Node::addX(float x)
{
    m_x += x;
}

void Document::Node::addY(float y)
{
    m_y += y;
}

void Document::Node::addZ(float z)
{
    m_z += z;
}
