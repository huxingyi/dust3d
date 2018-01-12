#include <math.h>
#include "math3d.h"

#define CONST_180_DIV_PI    57.29578f
#define CONST_PI_DIV_180    0.01745329252f

int math3dIdentifyPointOnWhichSideOfPlane(point3d *point, point3d *facePoint, vector3d *faceNormal) {
    vector3d v = *point;
    float result;
    vector3dSub(&v, facePoint);
    result = vector3dDotProduct(&v, faceNormal);
    if (result > 0) {
        return MATH3D_SIDE_FRONT;
    } else if (result < 0) {
        return MATH3D_SIDE_BACK;
    }
    return MATH3D_SIDE_COINCIDENCE;
}

int math3dIntersectionOfSegmentAndPlane(point3d *p1, point3d *p2, point3d *facePoint, vector3d *faceNormal, point3d *result) {
    vector3d u, w;
    float d, n, i;
    vector3dSegment(p1, p2, &u);
    vector3dSegment(facePoint, p1, &w);
    d = vector3dDotProduct(faceNormal, &u);
    n = -vector3dDotProduct(faceNormal, &w);
    if (fabs(d) < EPSILON) {
        if (0 == n) {
            return -1;
        }
        return -1;
    }
    i = n / d;
    if (i < 0 || i > 1) {
        return -1;
    }
    *result = u;
    vector3dMultiply(result, i);
    vector3dAdd(result, p1);
    return 0;
}

float math3dDistanceOfTwoPoints(point3d *p1, point3d *p2) {
    double a = p1->x - p2->x;
    double b = p1->y - p2->y;
    double c = p1->z - p2->z;
    return (float)sqrt(a * a + b * b + c * c);
}

int math3dIsPointOnSegment(point3d *c, point3d *a, point3d *b) {
    vector3d t, v, w;
    double dot1, dot2;
    float d;
    vector3dSegment(a, b, &v);
    vector3dSegment(a, c, &w);
    dot1 = vector3dDotProduct(&w, &v);
    if (dot1 <= 0) {
        return 0;
    }
    dot2 = vector3dDotProduct(&v, &v);
    if (dot2 <= dot1) {
        return 0;
    }
    t = v;
    vector3dMultiply(&t, dot1 / dot2);
    vector3dAdd(&t, a);
    if ((d=math3dDistanceOfTwoPoints(c, &t)) > EPSILON_DISTANCE) {
        return 0;
    }
    return 1;
}

int math3dCompareTwoPoints(point3d *p1, point3d *p2) {
    float offsetX = p1->x - p2->x;
    float offsetY = p1->y - p2->y;
    float offsetZ = p1->z - p2->z;
    if (fabs(offsetZ) > EPSILON_DISTANCE) {
        if (offsetZ < 0) {
            return -1;
        } else {
            return 1;
        }
    }
    if (fabs(offsetY) > EPSILON_DISTANCE) {
        if (offsetY < 0) {
            return -1;
        } else {
            return 1;
        }
    }
    if (fabs(offsetX) > EPSILON_DISTANCE) {
        if (offsetX < 0) {
            return -1;
        } else {
            return 1;
        }
    }
    return 0;
}

float math3dAngleOfTwoVectors(vector3d *v1, vector3d *v2) {
    float dot = vector3dDotProduct(v1, v2);
    float mag1 = vector3dMagnitude(v1);
    float mag2 = vector3dMagnitude(v2);
    double mag1Xmag2 = mag1 * mag2;
    double dotDivMags;
    if (0 == mag1Xmag2) {
        return 0;
    }
    dotDivMags = dot / mag1Xmag2;
    if (dotDivMags < -1) {
        dotDivMags = -1;
    }
    if (dotDivMags > 1) {
        dotDivMags = 1;
    }
    return CONST_180_DIV_PI * acos(dotDivMags);
}

int math3dMiddlePointOfTwoPoints(point3d *p1, point3d *p2, point3d *midPoint) {
    point3d *points[] = {p1, p2};
    math3dAveragePointOfPoints(points, 2, midPoint);
    return 0;
}

int math3dAveragePointOfPoints(point3d **points, int num, point3d *averagePoint) {
    int i;
    averagePoint->x = averagePoint->y = averagePoint->z = 0;
    for (i = 0; i < num; i++) {
        vector3dAdd(averagePoint, points[i]);
    }
    vector3dDivide(averagePoint, num);
    return 0;
}

float math3dDegreeToRadian(float degree) {
    return degree * CONST_PI_DIV_180;
}

