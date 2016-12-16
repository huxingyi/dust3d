#include "drawcommon.h"
#include "drawsphere.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/*
 * This function modified from [Jon Leech's implementation of sphere](ftp://ftp.ee.lbl.gov/sphere.c)  
 * Jeremy HU (huxingyi@msn.com) 2016/12/16
*/

#define XPLUS {  1,  0,  0 }  /*  X */
#define XMIN  { -1,  0,  0 }  /* -X */
#define YPLUS {  0,  1,  0 }  /*  Y */
#define YMIN  {  0, -1,  0 }  /* -Y */
#define ZPLUS {  0,  0,  1 }  /*  Z */
#define ZMIN  {  0,  0, -1 }  /* -Z */

static const triangle octahedron[] = {
  {{XPLUS, ZPLUS, YPLUS}},
  {{YPLUS, ZPLUS, XMIN}},
  {{XMIN,  ZPLUS, YMIN}},
  {{YMIN,  ZPLUS, XPLUS}},
  {{XPLUS, YPLUS, ZMIN}},
  {{YPLUS, XMIN,  ZMIN}},
  {{XMIN,  YMIN,  ZMIN}},
  {{YMIN,  XPLUS, ZMIN}},
};

static void subdivide(object *old, object *subdivided) {
  int i;
  for (i = 0; i < old->npoly; ++i) {
    triangle *oldt = &old->poly[i];
    triangle *newt = &subdivided->poly[i * 4];
    point a, b, c;

    midpoint(&oldt->pt[0], &oldt->pt[2], &a);
    midpoint(&oldt->pt[0], &oldt->pt[1], &b);
    midpoint(&oldt->pt[1], &oldt->pt[2], &c);

    normalize(&a);
    normalize(&b);
    normalize(&c);

    newt->pt[0] = oldt->pt[0];
    newt->pt[1] = b;
    newt->pt[2] = a;
    newt++;

    newt->pt[0] = b;
    newt->pt[1] = oldt->pt[1];
    newt->pt[2] = c;
    newt++;

    newt->pt[0] = a;
    newt->pt[1] = b;
    newt->pt[2] = c;
    newt++;

    newt->pt[0] = a;
    newt->pt[1] = c;
    newt->pt[2] = oldt->pt[2];
  }
}

int drawSphere(int level) {
  int lv, i;
  object oldObj, newObj;

  if (level < 1) {
    fprintf(stderr, "%s:level max greater than 0.\n", __FUNCTION__);
    return -1;
  }

  oldObj.npoly = sizeof(octahedron) / sizeof(octahedron[0]);
  oldObj.poly = (triangle *)malloc(oldObj.npoly * sizeof(triangle));
  if (!oldObj.poly) {
    fprintf(stderr, "%s:insufficient memory.\n", __FUNCTION__);
    return -1;
  }
  memcpy(oldObj.poly, octahedron, oldObj.npoly * sizeof(triangle));

  for (lv = 0; lv < level; lv++) {
    newObj.npoly = oldObj.npoly * 4;
    newObj.poly = (triangle *)malloc(newObj.npoly * sizeof(triangle));
    if (!newObj.poly) {
      fprintf(stderr, "%s:insufficient memory(levelLoop:%d).\n", 
        __FUNCTION__, lv);
      free(oldObj.poly);
      return -1;
    }

    subdivide(&oldObj, &newObj);

    free(oldObj.poly);
    oldObj = newObj;
  }

  for (i = 0; i < newObj.npoly; ++i) {
    drawTriangle(&newObj.poly[i]);
  }

  free(newObj.poly);

  return 0;
}
