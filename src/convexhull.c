#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "convexhull.h"
#include "array.h"
#include "draw.h"

//
// Implement Gift wrapping method which describled in http://dccg.upc.edu/people/vera/wp-content/uploads/2014/11/GA2014-ConvexHulls3D-Roger-Hernando.pdf
//
//
// Translate from Danielhst's lua version https://github.com/danielhst/3d-Hull-gift-wrap/blob/master/giftWrap.lua
//

typedef struct {
  vec3 pt;
  int plane;
  int orderOnPlane;
} converHullVertex;

struct convexHull {
  array *vertexArray;
  array *openEdgeArray;
  array *triangleArray;
  int nextEdgeIndex;
  unsigned int *openEdgeExistMap;
};

typedef struct {
  int firstVertex;
  int secondVertex;
} edge;

convexHull *convexHullCreate(void) {
  convexHull *hull = (convexHull *)calloc(1, sizeof(convexHull));
  if (!hull) {
    fprintf(stderr, "%s:Insufficient memory.\n", __FUNCTION__);
    return 0;
  }
  hull->vertexArray = arrayCreate(sizeof(converHullVertex));
  if (!hull->vertexArray) {
    fprintf(stderr, "%s:arrayCreate failed.\n", __FUNCTION__);
    return 0;
  }
  hull->openEdgeArray = arrayCreate(sizeof(edge));
  if (!hull->openEdgeArray) {
    fprintf(stderr, "%s:arrayCreate failed.\n", __FUNCTION__);
    return 0;
  }
  hull->triangleArray = arrayCreate(sizeof(triangle));
  if (!hull->triangleArray) {
    fprintf(stderr, "%s:arrayCreate failed.\n", __FUNCTION__);
    return 0;
  }
  return hull;
}

void convexHullMarkEdgeAsExsits(convexHull *hull, int firstVertex,
    int secondVertex) {
  int mapIndex = firstVertex * arrayGetLength(hull->vertexArray) + secondVertex;
  int unitIndex = mapIndex / (sizeof(unsigned int) * 8);
  int unitOffset = mapIndex % (sizeof(unsigned int) * 8);
  hull->openEdgeExistMap[unitIndex] |= (0x00000001 << unitOffset);
}

int convexHullEdgeExsits(convexHull *hull, int firstVertex,
    int secondVertex) {
  int mapIndex = firstVertex * arrayGetLength(hull->vertexArray) + secondVertex;
  int unitIndex = mapIndex / (sizeof(unsigned int) * 8);
  int unitOffset = mapIndex % (sizeof(unsigned int) * 8);
  return hull->openEdgeExistMap[unitIndex] & (0x00000001 << unitOffset);
}

int convexHullAddVertex(convexHull *hull, vec3 *vertex, int plane,
    int orderOnPlane) {
  converHullVertex *vtx;
  int newVertex = arrayGetLength(hull->vertexArray);
  if (0 != arraySetLength(hull->vertexArray, newVertex + 1)) {
    fprintf(stderr, "%s:arraySetLength failed.\n", __FUNCTION__);
    return -1;
  }
  vtx = (converHullVertex *)arrayGetItem(hull->vertexArray, newVertex);
  vtx->plane = plane;
  vtx->orderOnPlane = orderOnPlane;
  vtx->pt = *vertex;
  return newVertex;
}

int convexHullAddOpenEdgeNoCheck(convexHull *hull, int firstVertex,
    int secondVertex) {
  edge *e;
  int newEdge = arrayGetLength(hull->openEdgeArray);
  if (firstVertex < 0 || secondVertex < 0) {
    fprintf(stderr, "%s:Invalid params(firstVertex:%d secondVertex:%d).\n",
      __FUNCTION__, firstVertex, secondVertex);
    return -1;
  }
  if (0 != arraySetLength(hull->openEdgeArray, newEdge + 1)) {
    fprintf(stderr, "%s:arraySetLength failed.\n", __FUNCTION__);
    return -1;
  }
  e = (edge *)arrayGetItem(hull->openEdgeArray, newEdge);
  memset(e, 0, sizeof(edge));
  e->firstVertex = firstVertex;
  e->secondVertex = secondVertex;
  return 0;
}

int convexHullAddEdge(convexHull *hull, int p1, int p2) {
  convexHullMarkEdgeAsExsits(hull, p1, p2);
  if (!convexHullEdgeExsits(hull, p2, p1)) {
    return convexHullAddOpenEdgeNoCheck(hull, p2, p1);
  }
  return 0;
}

static int isInAdjacentOrder(int order1, int order2) {
  return ((order1 + 1) % 4 == order2 ||
    (order2 + 1) % 4 == order1);
}

int convexHullAddTriangle(convexHull *hull, int firstVertex, int secondVertex,
    int thirdVertex) {
  triangle *tri;
  converHullVertex *vtx1;
  converHullVertex *vtx2;
  converHullVertex *vtx3;
  int newTri;
  vtx1 = (converHullVertex *)arrayGetItem(hull->vertexArray, firstVertex);
  vtx2 = (converHullVertex *)arrayGetItem(hull->vertexArray, secondVertex);
  vtx3 = (converHullVertex *)arrayGetItem(hull->vertexArray, thirdVertex);
  if (vtx1->plane == vtx2->plane && vtx1->plane == vtx3->plane) {
    return 0;
  }
  if (vtx1->plane == vtx2->plane) {
    if (!isInAdjacentOrder(vtx1->orderOnPlane, vtx2->orderOnPlane)) {
      return 0;
    }
  }
  if (vtx1->plane == vtx3->plane) {
    if (!isInAdjacentOrder(vtx1->orderOnPlane, vtx3->orderOnPlane)) {
      return 0;
    }
  }
  if (vtx2->plane == vtx3->plane) {
    if (!isInAdjacentOrder(vtx2->orderOnPlane, vtx3->orderOnPlane)) {
      return 0;
    }
  }
  newTri = arrayGetLength(hull->triangleArray);
  if (0 != arraySetLength(hull->triangleArray, newTri + 1)) {
    fprintf(stderr, "%s:arraySetLength failed.\n", __FUNCTION__);
    return -1;
  }
  tri = (triangle *)arrayGetItem(hull->triangleArray, newTri);
  memset(tri, 0, sizeof(triangle));
  tri->pt[0] = vtx1->pt;
  tri->pt[1] = vtx2->pt;
  tri->pt[2] = vtx3->pt;
  return 0;
}

static void convexHullReleaseForGenerate(convexHull *hull) {
  free(hull->openEdgeExistMap);
  hull->openEdgeExistMap = 0;
}

void convexHullDestroy(convexHull *hull) {
  arrayDestroy(hull->vertexArray);
  arrayDestroy(hull->openEdgeArray);
  arrayDestroy(hull->triangleArray);
  convexHullReleaseForGenerate(hull);
  free(hull);
}

static int convexHullPrepareForGenerate(convexHull *hull) {
  free(hull->openEdgeExistMap);
  hull->openEdgeExistMap = (unsigned int *)calloc(
    arrayGetLength(hull->vertexArray) * arrayGetLength(hull->vertexArray) /
      (sizeof(unsigned int) * 8) + 1,
    sizeof(unsigned int));
  if (!hull->openEdgeExistMap) {
    fprintf(stderr, "%s:Insufficient memory.\n", __FUNCTION__);
    return -1;
  }
  hull->nextEdgeIndex = 0;
  return 0;
}

int convexHullGetLower(convexHull *hull) {
  int index = 0;
  int i;
  for (i = 1; i < arrayGetLength(hull->vertexArray); ++i) {
    vec3 *pI = (vec3 *)arrayGetItem(hull->vertexArray, i);
    vec3 *pIndex = (vec3 *)arrayGetItem(hull->vertexArray, index);
    if (pI->z < pIndex->z) {
      index = i;
    } else if (pI->z == pIndex->z) {
      if (pI->y < pIndex->y) {
        index = i;
      } else if (pI->x < pIndex->x) {
        index = i;
      }
    }
  }
  return index;
}

int convexHullGetNextVertex(convexHull *hull, int p1Index, int p2Index) {
  vec3 pCorner = {1, 1, 0};
  vec3 *p1 = (vec3 *)arrayGetItem(hull->vertexArray, p1Index);
  vec3 *p2 = p2Index < 0 ? &pCorner : (vec3 *)arrayGetItem(hull->vertexArray,
    p2Index);
  vec3 edge;
  int candidateIndex = -1;
  int i;
  vec3Sub(p2, p1, &edge);
  vec3Normalize(&edge);
  for (i = 0; i < arrayGetLength(hull->vertexArray); ++i) {
    if (i != p1Index && i != p2Index) {
      if (-1 == candidateIndex) {
        candidateIndex = 0;
      } else {
        vec3 v, proj, candidate, canProj, cross;
        vec3Sub((vec3 *)arrayGetItem(hull->vertexArray, i), p1, &v);
        vec3ProjectOver(&v, &edge, &proj);
        vec3Sub(&v, &proj, &v);
        vec3Sub((vec3 *)arrayGetItem(hull->vertexArray,
          candidateIndex), p1, &candidate);
        vec3ProjectOver(&candidate, &edge, &canProj);
        vec3Sub(&candidate, &canProj, &candidate);
        vec3CrossProduct(&candidate, &v, &cross);
        if (vec3DotProduct(&cross, &edge) > 0) {
          candidateIndex = i;
        }
      }
    }
  }
  return candidateIndex;
}

int convexHullGenerate(convexHull *hull) {
  int index1, index2, index3;
  convexHullReleaseForGenerate(hull);
  if (0 != convexHullPrepareForGenerate(hull)) {
    fprintf(stderr, "%s:convexHullPrepareForGenerate failed.\n", __FUNCTION__);
    return -1;
  }
  index1 = convexHullGetLower(hull);
  index2 = convexHullGetNextVertex(hull, index1, -1);
  convexHullAddEdge(hull, index2, index1);
  //glColor3f(0.0, 0.0, 0.0);
  //drawDebugPrintf("edgeLength:%d", arrayGetLength(hull->openEdgeArray));
  while (hull->nextEdgeIndex < arrayGetLength(hull->openEdgeArray)) {
    edge *e = (edge *)arrayGetItem(hull->openEdgeArray, hull->nextEdgeIndex++);
    index1 = e->firstVertex;
    index2 = e->secondVertex;
    if (convexHullEdgeExsits(hull, index1, index2)) {
      continue;
    }
    convexHullMarkEdgeAsExsits(hull, index1, index2);
    index3 = convexHullGetNextVertex(hull, index1, index2);
    //drawDebugPrintf("%d,%d,%d", index1, index2, index3);
    convexHullAddTriangle(hull, index1, index2, index3);
    convexHullAddEdge(hull, index1, index2);
    convexHullAddEdge(hull, index2, index3);
    convexHullAddEdge(hull, index3, index1);
  }
  return 0;
}

int convexHullGetTriangleNum(convexHull *hull) {
  return arrayGetLength(hull->triangleArray);
}

triangle *convexHullGetTriangle(convexHull *hull, int index) {
  triangle *tri = (triangle *)arrayGetItem(hull->triangleArray, index);
  return tri;
}
