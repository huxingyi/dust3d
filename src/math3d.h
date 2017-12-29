#ifndef DUST3D_MATH3D_H
#define DUST3D_MATH3D_H
#include "vector3d.h"

#define EPSILON_DISTANCE 0.01f

enum side {
    SIDE_COINCIDENCE,
    SIDE_FRONT,
    SIDE_BACK
};
int identifyPointOnWhichSideOfPlane(point3d *point, point3d *facePoint, vector3d *faceNormal);
int intersectionOfSegmentAndPlane(point3d *p1, point3d *p2, point3d *facePoint, vector3d *faceNormal, point3d *result);
float distance(point3d *p1, point3d *p2);
int isPointOnSegment(point3d *c, point3d *a, point3d *b);

#endif
