#ifndef DUST3D_STANDARD_H
#define DUST3D_STANDARD_H
#include "halfedge.h"

typedef struct standardBox {
    float length;
    float width;
    float height;
    int lengthSegs;
    int widthSegs;
    int heightSegs;
} standardBox;

#define STANDARD_BOX_DEFAULT {              \
    1.0f,    /*float length;*/              \
    1.0f,    /*float width;*/               \
    1.0f,    /*float height;*/              \
    1,       /*int lengthSegs;*/            \
    1,      /*int widthSegs;*/              \
    1,      /*int heightSegs;*/             \
}

mesh *standardCreateBoxMesh(standardBox *box);

#endif

