#ifndef RIG_FRAME
#define RIG_FRAME
#include <QQuaternion>
#include <QVector3D>
#include <vector>
#include <set>

class RigFrame
{
public:
    RigFrame();
    RigFrame(int jointNum);
    void updateRotation(int index, const QQuaternion &rotation);
    void updateTranslation(int index, const QVector3D &translation);
    void updateScale(int index, const QVector3D &scale);
public:
    std::vector<QQuaternion> rotations;
    std::vector<QVector3D> translations;
    std::vector<QVector3D> scales;
    std::set<int> rotatedIndicies;
    std::set<int> translatedIndicies;
    std::set<int> scaledIndicies;
};

#endif
