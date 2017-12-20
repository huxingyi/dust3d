#ifndef DUST3D_GEOMETRY_H
#define DUST3D_GEOMETRY_H
#include "vector3d.h"

enum GEOMETRY {
    GEOMETRY_NO_INTERSECT,
    GEOMETRY_PARALLEL,
    GEOMETRY_INTERSECT,
    GEOMETRY_FRONT,
    GEOMETRY_BACK,
    GEOMETRY_COLLAPSE
};
int geometryRelationBetweenPointAndPlane(point3d *point, point3d *facePoint, vector3d *faceNormal);
int geometryIntersectionOfSegmentAndPlane(point3d *p1, point3d *p2, point3d *facePoint, vector3d *faceNormal, point3d *result);

#endif
