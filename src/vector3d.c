#include <math.h>
#include "vector3d.h"

void vector3dNormal(point3d *p1, point3d *p2, point3d *p3, vector3d *v) {
    float v1[3], v2[3], vr[3], val;

    v1[0] = p1->x - p2->x;
    v1[1] = p1->y - p2->y;
    v1[2] = p1->z - p2->z;

    v2[0] = p1->x - p3->x;
    v2[1] = p1->y - p3->y;
    v2[2] = p1->z - p3->z;

    vr[0] = v1[1] * v2[2] - v2[1] * v1[2];
    vr[1] = v2[0] * v1[2] - v1[0] * v2[2];
    vr[2] = v1[0] * v2[1] - v2[0] * v1[1];

    val = sqrt(vr[0]*vr[0] + vr[1]*vr[1] + vr[2]*vr[2]);

    v->x = vr[0] / val;
    v->y = vr[1] / val;
    v->z = vr[2] / val;
}

void vector3dAdd(vector3d *v, vector3d *add) {
    v->x += add->x;
    v->y += add->y;
    v->z += add->z;
}

void vector3dSub(vector3d *v, vector3d *sub) {
    v->x -= sub->x;
    v->y -= sub->y;
    v->z -= sub->z;
}

void vector3dDivide(vector3d *v, float div) {
    v->x /= div;
    v->y /= div;
    v->z /= div;
}

void vector3dMultiply(vector3d *v, float mul) {
    v->x *= mul;
    v->y *= mul;
    v->z *= mul;
}

void vector3dLerp(vector3d *a, vector3d *b, float frac, vector3d *result) {
    result->x = a->x + (b->x - a->x) * frac;
    result->y = a->y + (b->y - a->y) * frac;
    result->z = a->z + (b->z - a->z) * frac;
}

float vector3dDotProduct(vector3d *a, vector3d *b) {
    return a->x * b->x + a->y * b->y + a->z * b->z;
}

void vector3dCrossProduct(vector3d *a, vector3d *b, vector3d *result) {
    result->x = a->y * b->z - a->z * b->y;
    result->y = a->z * b->x - a->x * b->z;
    result->z = a->x * b->y - a->y * b->x;
}

float vector3dMagnitude(vector3d *v) {
    double mag;
    mag = v->x * v->x + v->y * v->y + v->z * v->z;
    return sqrt(mag);
}

float vector3dAngle(vector3d *a, vector3d *b) {
    float angle;
    float da, db;
    float acosParam;
    da = vector3dMagnitude(a);
    db = vector3dMagnitude(b);
    if (0 == da || 0 == db) {
        return 0;
    }
    acosParam = vector3dDotProduct(a, b) / (da * db);
    if (acosParam < -1) {
        acosParam = -1;
    }
    if (acosParam > 1) {
        acosParam = 1;
    }
    /* 57.29578f = 180 / M_PI */
    angle = acos(acosParam) * 57.29578f;
    return angle;
}

void vector3dNormalize(vector3d *v) {
    double mag;
    mag = v->x * v->x + v->y * v->y + v->z * v->z;
    if (mag != 0.0) {
        mag = 1.0 / sqrt(mag);
        v->x *= mag;
        v->y *= mag;
        v->z *= mag;
    }
}

void vector3dSegment(point3d *p1, point3d *p2, vector3d *v) {
    *v = *p1;
    vector3dSub(v, p2);
}
