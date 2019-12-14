#include "positionkey.h"

long PositionKey::m_toIntFactor = 100000;

PositionKey::PositionKey(const QVector3D &v) :
    PositionKey(v.x(), v.y(), v.z())
{
}

PositionKey::PositionKey(float x, float y, float z)
{
    m_position.setX(x);
    m_position.setY(y);
    m_position.setZ(z);
    
    m_intX = x * m_toIntFactor;
    m_intY = y * m_toIntFactor;
    m_intZ = z * m_toIntFactor;
}

const QVector3D &PositionKey::position() const
{
    return m_position;
}

bool PositionKey::operator <(const PositionKey &right) const
{
    if (m_intX < right.m_intX)
        return true;
    if (m_intX > right.m_intX)
        return false;
    if (m_intY < right.m_intY)
        return true;
    if (m_intY > right.m_intY)
        return false;
    if (m_intZ < right.m_intZ)
        return true;
    if (m_intZ > right.m_intZ)
        return false;
    return false;
}

bool PositionKey::operator ==(const PositionKey &right) const
{
    return m_intX == right.m_intX &&
        m_intY == right.m_intY &&
        m_intZ == right.m_intZ;
}
