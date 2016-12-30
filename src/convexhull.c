#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "convexhull.h"
#include "array.h"
#include "hashtable.h"

//
// Implement Gift wrapping method which describled in http://dccg.upc.edu/people/vera/wp-content/uploads/2014/11/GA2014-ConvexHulls3D-Roger-Hernando.pdf
//

#define TRIANGLE_INDEX_HASHTABLE_SIZE 100

typedef struct {
  vec3 pt;
  int plane;
  int orderOnPlane;
} converHullVertex;

struct convexHull {
  array *vertexArray;
  array *todoArray;
  array *face3Array;
  int nextTodoIndex;
  unsigned int *openEdgeExistMap;
  hashtable *face3Hashtable;
  face3 findFace3;
};

typedef struct {
  int firstVertex;
  int secondVertex;
  int thirdVertex;
} todo;

static int cantorPair(int k1, int k2) {
  return (k1 + k2) * (k1 + k2 + 1) / 2 + k2;
}

face3 *convexHullGetFaceByHashtableParam(convexHull *hull,
    const void *param) {
  int index = (char *)param - (char *)0;
  if (0 == index) {
    return &hull->findFace3;
  }
  return (face3 *)arrayGetItem(hull->face3Array, index - 1);
}

static int faceHash(void *userData, const void *node) {
  face3 *triIdx = convexHullGetFaceByHashtableParam(
    (convexHull *)userData, node);
  return cantorPair(cantorPair(triIdx->indices[0], triIdx->indices[1]),
    triIdx->indices[2]);
}

static int faceCompare(void *userData, const void *node1,
    const void *node2) {
  face3 *triIdx1 = convexHullGetFaceByHashtableParam(
    (convexHull *)userData, node1);
  face3 *triIdx2 = convexHullGetFaceByHashtableParam(
    (convexHull *)userData, node2);
  return memcmp(triIdx1, triIdx2, sizeof(face3));
}

convexHull *convexHullCreate(void) {
  convexHull *hull = (convexHull *)calloc(1, sizeof(convexHull));
  if (!hull) {
    fprintf(stderr, "%s:Insufficient memory.\n", __FUNCTION__);
    return 0;
  }
  hull->vertexArray = arrayCreate(sizeof(converHullVertex));
  if (!hull->vertexArray) {
    fprintf(stderr, "%s:arrayCreate failed.\n", __FUNCTION__);
    convexHullDestroy(hull);
    return 0;
  }
  hull->todoArray = arrayCreate(sizeof(todo));
  if (!hull->todoArray) {
    fprintf(stderr, "%s:arrayCreate failed.\n", __FUNCTION__);
    convexHullDestroy(hull);
    return 0;
  }
  hull->face3Array = arrayCreate(sizeof(face3));
  if (!hull->face3Array) {
    fprintf(stderr, "%s:arrayCreate failed.\n", __FUNCTION__);
    convexHullDestroy(hull);
    return 0;
  }
  hull->face3Hashtable = hashtableCreate(TRIANGLE_INDEX_HASHTABLE_SIZE,
    faceHash, faceCompare, hull);
  if (!hull->face3Hashtable) {
    fprintf(stderr, "%s:hashtableCreate failed.\n", __FUNCTION__);
    convexHullDestroy(hull);
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
  if (hull->openEdgeExistMap) {
    int mapIndex = firstVertex * arrayGetLength(hull->vertexArray) + secondVertex;
    int unitIndex = mapIndex / (sizeof(unsigned int) * 8);
    int unitOffset = mapIndex % (sizeof(unsigned int) * 8);
    return hull->openEdgeExistMap[unitIndex] & (0x00000001 << unitOffset);
  }
  return 0;
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

int convexHullAddTodoNoCheck(convexHull *hull, int firstVertex,
    int secondVertex, int thirdVertex) {
  todo *t;
  int newEdge = arrayGetLength(hull->todoArray);
  if (firstVertex < 0 || secondVertex < 0) {
    fprintf(stderr, "%s:Invalid params(firstVertex:%d secondVertex:%d).\n",
      __FUNCTION__, firstVertex, secondVertex);
    return -1;
  }
  if (0 != arraySetLength(hull->todoArray, newEdge + 1)) {
    fprintf(stderr, "%s:arraySetLength failed.\n", __FUNCTION__);
    return -1;
  }
  t = (todo *)arrayGetItem(hull->todoArray, newEdge);
  memset(t, 0, sizeof(todo));
  t->firstVertex = firstVertex;
  t->secondVertex = secondVertex;
  t->thirdVertex = thirdVertex;
  return 0;
}

static int isInAdjacentOrder(int order1, int order2) {
  return ((order1 + 1) % 4 == order2 ||
    (order2 + 1) % 4 == order1);
}

static int sortface(const void *first, const void *second) {
  const int *firstIndex = (const void *)first;
  const int *secondIndex = (const void *)second;
  return *firstIndex - *secondIndex;
}

int convexHullAddFace3(convexHull *hull, int firstVertex, int secondVertex,
    int thirdVertex) {
  face3 *tri;
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
  memset(&hull->findFace3, 0, sizeof(hull->findFace3));
  hull->findFace3.indices[0] = firstVertex;
  hull->findFace3.indices[1] = secondVertex;
  hull->findFace3.indices[2] = thirdVertex;
  qsort(hull->findFace3.indices, 3,
    sizeof(hull->findFace3.indices[0]), sortface);
  if (0 == hashtableGet(hull->face3Hashtable, 0)) {
    newTri = arrayGetLength(hull->face3Array);
    if (0 != arraySetLength(hull->face3Array, newTri + 1)) {
      fprintf(stderr, "%s:arraySetLength failed.\n", __FUNCTION__);
      return -1;
    }
    tri = (face3 *)arrayGetItem(hull->face3Array, newTri);
    *tri = hull->findFace3;
    if (0 != hashtableInsert(hull->face3Hashtable,
        (char *)0 + newTri + 1)) {
      fprintf(stderr, "%s:hashtableInsert failed.\n", __FUNCTION__);
      return -1;
    }
  }
  return 0;
}

static void convexHullReleaseForGenerate(convexHull *hull) {
  free(hull->openEdgeExistMap);
  hull->openEdgeExistMap = 0;
}

void convexHullDestroy(convexHull *hull) {
  arrayDestroy(hull->vertexArray);
  arrayDestroy(hull->todoArray);
  arrayDestroy(hull->face3Array);
  hashtableDestroy(hull->face3Hashtable);
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
  hull->nextTodoIndex = 0;
  return 0;
}

int convexHullGetNextVertex(convexHull *hull, int p1Index, int p2Index,
    int p3Index) {
  vec3 *p1 = (vec3 *)arrayGetItem(hull->vertexArray, p1Index);
  vec3 *p2 = (vec3 *)arrayGetItem(hull->vertexArray, p2Index);
  vec3 *p3 = (vec3 *)arrayGetItem(hull->vertexArray, p3Index);
  vec3 beginNormal;
  vec3 endNormal;
  int i;
  float angle;
  float maxAngle = 0;
  int candidateIndex = -1;

  vec3Normal(p1, p2, p3, &beginNormal);
  
  for (i = 0; i < arrayGetLength(hull->vertexArray); ++i) {
    if (i != p1Index && i != p2Index && i != p3Index) {
      vec3Normal(p1, p2, (vec3 *)arrayGetItem(hull->vertexArray, i),
        &endNormal);
      angle = vec3Angle(&beginNormal, &endNormal);
      if (angle > maxAngle) {
        candidateIndex = i;
        maxAngle = angle;
      }
    }
  }
  
  return candidateIndex;
}

int convexHullAddTodo(convexHull *hull, int vertex1, int vertex2,
    int vertex3) {
  if (!convexHullEdgeExsits(hull, vertex1, vertex2)) {
    return convexHullAddTodoNoCheck(hull, vertex1, vertex2, vertex3);
  }
  return 0;
}

int convexHullGenerate(convexHull *hull) {
  int index1, index2, index3;
  convexHullReleaseForGenerate(hull);
  if (0 != convexHullPrepareForGenerate(hull)) {
    fprintf(stderr, "%s:convexHullPrepareForGenerate failed.\n", __FUNCTION__);
    return -1;
  }
  while (hull->nextTodoIndex < arrayGetLength(hull->todoArray)) {
    todo *t = (todo *)arrayGetItem(hull->todoArray, hull->nextTodoIndex++);
    index1 = t->firstVertex;
    index2 = t->secondVertex;
    if (convexHullEdgeExsits(hull, index1, index2) ||
        convexHullEdgeExsits(hull, index2, index1)) {
      continue;
    }
    convexHullMarkEdgeAsExsits(hull, index1, index2);
    index3 = convexHullGetNextVertex(hull, index1, index2, t->thirdVertex);
    if (-1 == index3) {
      continue;
    }
    convexHullAddFace3(hull, index1, index2, index3);
    convexHullAddTodo(hull, index2, index3, index1);
    convexHullAddTodo(hull, index3, index1, index2);
  }
  return 0;
}

int convexHullUnifyNormals(convexHull *hull, vec3 *origin) {
  int i;
  for (i = 0; i < arrayGetLength(hull->face3Array); ++i) {
    face3 *triIdx = (face3 *)arrayGetItem(
      hull->face3Array, i);
    converHullVertex *p1 = (converHullVertex *)arrayGetItem(
      hull->vertexArray, triIdx->indices[0]);
    converHullVertex *p2 = (converHullVertex *)arrayGetItem(
      hull->vertexArray, triIdx->indices[1]);
    converHullVertex *p3 = (converHullVertex *)arrayGetItem(
      hull->vertexArray, triIdx->indices[2]);
    vec3 normal;
    vec3 o2v;
    vec3Normal(&p1->pt, &p2->pt, &p3->pt, &normal);
    vec3Sub(&p1->pt, origin, &o2v);
    if (vec3DotProduct(&o2v, &normal) < 0) {
      int index = triIdx->indices[0];
      triIdx->indices[0] = triIdx->indices[2];
      triIdx->indices[2] = index;
    }
  }
  return 0;
}

face3 *convexHullGetFace3(convexHull *hull, int faceIndex) {
  return (face3 *)arrayGetItem(hull->face3Array, faceIndex);
}

vec3 *convexHullGetVertex(convexHull *hull, int vertexIndex) {
  converHullVertex *vertex = (converHullVertex *)arrayGetItem(
    hull->vertexArray, vertexIndex);
  return &vertex->pt;
}

int convexHullGetFace3Num(convexHull *hull) {
  return arrayGetLength(hull->face3Array);
}
