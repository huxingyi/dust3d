#include "geometry.h"

int geometryPointPlaneLocation(point3d *point, point3d *facePoint, vector3d *faceNormal) {
    vector3d v = *point;
    int result;
    vector3dSub(&v, facePoint);
    result = vector3dDotProduct(&v, faceNormal);
    if (result > 0) {
        return POINT_LOC_FRONT;
    } else if (result < 0) {
        return POINT_LOC_BACK;
    }
    return POINT_LOC_MIDDLE;
}

int geometryPointPlaneIntersection(point3d *p1, point3d *p2, point3d *facePoint, vector3d *faceNormal, point3d *result) {
    vector3d n = *p2;
    float t;
    float vpt;
    vector3d fp1 = *facePoint;
    vector3dSub(&fp1, p1);
    vector3dSub(&n, p1);
    vector3dNormalize(&n);
    if (0 == (vpt=vector3dDotProduct(faceNormal, &n))) {
        return -1;
    }
    t = vector3dDotProduct(&fp1, &n) / vpt;
    result->x = p1->x + faceNormal->x * t;
    result->y = p1->y + faceNormal->y * t;
    result->z = p1->z + faceNormal->z * t;
    return 0;
}
