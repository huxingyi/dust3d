#include "rigframe.h"

RigFrame::RigFrame()
{
}

RigFrame::RigFrame(int jointNum)
{
    rotations.clear();
    rotations.resize(jointNum);
    
    translations.clear();
    translations.resize(jointNum);
    
    scales.clear();
    scales.resize(jointNum, QVector3D(1.0, 1.0, 1.0));
}

void RigFrame::updateRotation(int index, const QQuaternion &rotation)
{
    rotations[index] = rotation;
    rotatedIndicies.insert(index);
}

void RigFrame::updateTranslation(int index, const QVector3D &translation)
{
    translations[index] = translation;
    translatedIndicies.insert(index);
}

void RigFrame::updateScale(int index, const QVector3D &scale)
{
    scales[index] = scale;
    scaledIndicies.insert(index);
}
