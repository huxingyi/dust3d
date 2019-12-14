#ifndef DUST3D_POSITION_KEY_H
#define DUST3D_POSITION_KEY_H
#include <QVector3D>

class PositionKey
{
public:
    PositionKey(const QVector3D &v);
    PositionKey(float x, float y, float z);
    const QVector3D &position() const;
    bool operator <(const PositionKey &right) const;
    bool operator ==(const PositionKey &right) const;

private:
    long m_intX = 0;
    long m_intY = 0;
    long m_intZ = 0;
    QVector3D m_position;

    static long m_toIntFactor;
};

#endif
