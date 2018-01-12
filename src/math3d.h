#ifndef DUST3D_MATH3D_H
#define DUST3D_MATH3D_H
#include "vector3d.h"

#define EPSILON_DISTANCE 0.01f

enum side {
    MATH3D_SIDE_COINCIDENCE,
    MATH3D_SIDE_FRONT,
    MATH3D_SIDE_BACK
};
int math3dIdentifyPointOnWhichSideOfPlane(point3d *point, point3d *facePoint, vector3d *faceNormal);
int math3dIntersectionOfSegmentAndPlane(point3d *p1, point3d *p2, point3d *facePoint, vector3d *faceNormal, point3d *result);
float math3dDistanceOfTwoPoints(point3d *p1, point3d *p2);
int math3dIsPointOnSegment(point3d *c, point3d *a, point3d *b);
int math3dCompareTwoPoints(point3d *p1, point3d *p2);
float math3dAngleOfTwoVectors(vector3d *v1, vector3d *v2);
int math3dMiddlePointOfTwoPoints(point3d *p1, point3d *p2, point3d *midPoint);
int math3dAveragePointOfPoints(point3d **points, int num, point3d *averagePoint);
float math3dDegreeToRadian(float degree);

#endif
