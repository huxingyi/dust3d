#include <QDebug>
#include "skeletonsnapshot.h"
#include "util.h"

void SkeletonSnapshot::resolveBoundingBox(QRectF *mainProfile, QRectF *sideProfile, const QString &partId)
{
    float left = 0;
    bool leftFirstTime = true;
    float right = 0;
    bool rightFirstTime = true;
    float top = 0;
    bool topFirstTime = true;
    float bottom = 0;
    bool bottomFirstTime = true;
    float zLeft = 0;
    bool zLeftFirstTime = true;
    float zRight = 0;
    bool zRightFirstTime = true;
    for (const auto &nodeIt: nodes) {
        if (!partId.isEmpty() && partId != valueOfKeyInMapOrEmpty(nodeIt.second, "partId"))
            continue;
        float radius = valueOfKeyInMapOrEmpty(nodeIt.second, "radius").toFloat();
        float x = valueOfKeyInMapOrEmpty(nodeIt.second, "x").toFloat();
        float y = valueOfKeyInMapOrEmpty(nodeIt.second, "y").toFloat();
        float z = valueOfKeyInMapOrEmpty(nodeIt.second, "z").toFloat();
        if (leftFirstTime || x - radius < left) {
            left = x - radius;
            leftFirstTime = false;
        }
        if (topFirstTime || y - radius < top) {
            top = y - radius;
            topFirstTime = false;
        }
        if (rightFirstTime || x + radius > right) {
            right = x + radius;
            rightFirstTime = false;
        }
        if (bottomFirstTime || y + radius > bottom) {
            bottom = y + radius;
            bottomFirstTime = false;
        }
        if (zLeftFirstTime || z - radius < zLeft) {
            zLeft = z - radius;
            zLeftFirstTime = false;
        }
        if (zRightFirstTime || z + radius > zRight) {
            zRight = z + radius;
            zRightFirstTime = false;
        }
    }
    *mainProfile = QRectF(QPointF(left, top), QPointF(right, bottom));
    *sideProfile = QRectF(QPointF(zLeft, top), QPointF(zRight, bottom));
    //qDebug() << "resolveBoundingBox left:" << left << "top:" << top << "right:" << right << "bottom:" << bottom << " zLeft:" << zLeft << "zRight:" << zRight;
    //qDebug() << "mainHeight:" << mainProfile->height() << "mainWidth:" << mainProfile->width();
    //qDebug() << "sideHeight:" << sideProfile->height() << "sideWidth:" << sideProfile->width();
}


