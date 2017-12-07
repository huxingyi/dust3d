#ifndef DUST3D_GEOMETRY_H
#define DUST3D_GEOMETRY_H
#include "vector3d.h"

enum POINT_LOC {
    POINT_LOC_MIDDLE,
    POINT_LOC_FRONT,
    POINT_LOC_BACK
};
int geometryPointPlaneLocation(point3d *point, point3d *facePoint, vector3d *faceNormal);
int geometryPointPlaneIntersection(point3d *p1, point3d *p2, point3d *facePoint, vector3d *faceNormal, point3d *result);

#endif
