#include <math.h>
#include "math3d.h"

int identifyPointOnWhichSideOfPlane(point3d *point, point3d *facePoint, vector3d *faceNormal) {
    vector3d v = *point;
    float result;
    vector3dSub(&v, facePoint);
    result = vector3dDotProduct(&v, faceNormal);
    if (result > 0) {
        return SIDE_FRONT;
    } else if (result < 0) {
        return SIDE_BACK;
    }
    return SIDE_COINCIDENCE;
}

int intersectionOfSegmentAndPlane(point3d *p1, point3d *p2, point3d *facePoint, vector3d *faceNormal, point3d *result) {
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

float distance(point3d *p1, point3d *p2) {
    double a = p1->x - p2->x;
    double b = p1->y - p2->y;
    double c = p1->z - p2->z;
    return (float)sqrt(a * a + b * b + c * c);
}

int isPointOnSegment(point3d *c, point3d *a, point3d *b) {
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
    if ((d=distance(c, &t)) > EPSILON_DISTANCE) {
        return 0;
    }
    return 1;
}

int compareTwoPoints(point3d *p1, point3d *p2) {
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

