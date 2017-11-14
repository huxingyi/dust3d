#ifndef DUST3D_VECTOR3D_H
#define DUST3D_VECTOR3D_H

typedef struct vector3d {
    float x;
    float y;
    float z;
} vector3d;

typedef struct point3d {
    float x;
    float y;
    float z;
} point3d;

void vector3dNormal(point3d *p1, point3d *p2, point3d *p3, vector3d *v);
void vector3dAdd(vector3d *v, vector3d *add);
void vector3dDivide(vector3d *v, float div);
void vector3dMultiply(vector3d *v, float mul);

#endif
