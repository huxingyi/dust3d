#include <math.h>
#include "geometry.h"

int geometryRelationBetweenPointAndPlane(point3d *point, point3d *facePoint, vector3d *faceNormal) {
    vector3d v = *point;
    float result;
    vector3dSub(&v, facePoint);
    result = vector3dDotProduct(&v, faceNormal);
    if (result > 0) {
        return GEOMETRY_FRONT;
    } else if (result < 0) {
        return GEOMETRY_BACK;
    }
    return GEOMETRY_COLLAPSE;
}

int geometryIntersectionOfSegmentAndPlane(point3d *p1, point3d *p2, point3d *facePoint, vector3d *faceNormal, point3d *result) {
    vector3d u, w;
    float d, n, i;
    vector3dSegment(p2, p1, &u);
    vector3dSegment(p1, facePoint, &w);
    d = vector3dDotProduct(faceNormal, &u);
    n = -vector3dDotProduct(faceNormal, &w);
    if (fabs(d) < 0.00000001) {
        if (0 == n) {
            return GEOMETRY_COLLAPSE;
        }
        return GEOMETRY_NO_INTERSECT;
    }
    i = n / d;
    if (i < 0 || i > 1) {
        return GEOMETRY_NO_INTERSECT;
    }
    if (i < 0) {
        i = 0;
    } else if (i > 1) {
        i = 1;
    }
    *result = u;
    vector3dMultiply(result, i);
    vector3dAdd(result, p1);
    return GEOMETRY_INTERSECT;
}
