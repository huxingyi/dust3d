#ifndef MOTION_BUILDER_H
#define MOTION_BUILDER_H
#include <QVector3D>

class MotionBuilder
{
public:
    struct Node
    {
        QVector3D position;
        double radius;
        int boneIndex = -1;
        bool isTail = false;
    };

    enum class Side
    {
        Middle = 0,
        Left,
        Right,
    };
};

#endif
