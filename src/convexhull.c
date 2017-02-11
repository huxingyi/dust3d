#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "dmemory.h"
#include "convexhull.h"
#include "array.h"
#include "dict.h"

//
// Implement Gift wrapping method which describled in http://dccg.upc.edu/people/vera/wp-content/uploads/2014/11/GA2014-ConvexHulls3D-Roger-Hernando.pdf
//

#define FACE3_HASHTABLE_SIZE  100
#define EDGE_HASHTABLE_SIZE   100

typedef struct {
  int p1;
  int p2;
  int hill1;
  int hill2;
  vec3 hill1Normal;
  int angleBetweenFaces;
  int angleWithY;
  int face1;
  int face2;
} edge;

struct convexHull {
  array *vertexArray;
  array *todoArray;
  array *faceArray;
  int nextTodoIndex;
  unsigned int *openEdgeProcessedMap;
  dict *face3Dict;
  array *edgeArray;
  dict *edgeDict;
};

typedef struct {
  int firstVertex;
  int secondVertex;
  int thirdVertex;
} todo;

static int cantorPair(int k1, int k2) {
  return (k1 + k2) * (k1 + k2 + 1) / 2 + k2;
}

static int face3Hash(void *userData, const void *node) {
  const face3 *face = node;
  return cantorPair(cantorPair(face->indices[0], face->indices[1]),
    face->indices[2]);
}

static int edgeHash(void *userData, const void *node) {
  const edge *e = node;
  return e->p1 < e->p2 ? cantorPair(e->p1, e->p2) : cantorPair(e->p2, e->p1);
}

static int face3Compare(void *userData, const void *node1,
    const void *node2) {
  return memcmp(node1, node2, sizeof(face3));
}

static int edgeCompare(void *userData, const void *node1,
    const void *node2) {
  const edge *e1 = node1;
  const edge *e2 = node2;
  if ((e1->p1 == e2->p1 && e1->p2 == e2->p2) ||
      (e1->p2 == e2->p1 && e1->p1 == e2->p2)) {
    return 0;
  }
  return -1;
}

convexHull *convexHullCreate(void) {
  convexHull *hull = dcalloc(1, sizeof(convexHull));
  hull->vertexArray = arrayCreate(sizeof(convexHullVertex));
  hull->todoArray = arrayCreate(sizeof(todo));
  hull->faceArray = arrayCreate(sizeof(convexHullFace));
  hull->face3Dict = dictCreate(hull->faceArray, FACE3_HASHTABLE_SIZE,
    face3Hash, face3Compare, hull);
  hull->edgeArray = arrayCreate(sizeof(edge));
  hull->edgeDict = dictCreate(hull->edgeArray, EDGE_HASHTABLE_SIZE,
    edgeHash, edgeCompare, hull);
  return hull;
}

edge *convexHullFindEdge(convexHull *hull, int p1, int p2) {
  edge findEdge;
  findEdge.p1 = p1;
  findEdge.p2 = p2;
  return dictFind(hull->edgeDict, &findEdge);
}

void convexHullAddEdge(convexHull *hull, int p1, int p2, int hill,
    int face) {
  edge findEdge;
  edge *e;
  findEdge.p1 = p1;
  findEdge.p2 = p2;
  e = dictFind(hull->edgeDict, &findEdge);
  if (!e) {
    e = dictGetClear(hull->edgeDict, &findEdge);
    e->p1 = p1;
    e->p2 = p2;
    e->hill1 = hill;
    e->hill2 = -1;
    e->face1 = face;
    e->face2 = -1;
    vec3Normal((vec3 *)arrayGetItem(hull->vertexArray, e->p1),
      (vec3 *)arrayGetItem(hull->vertexArray, e->p2),
      (vec3 *)arrayGetItem(hull->vertexArray, e->hill1), &e->hill1Normal);
    return;
  }
  assert(-1 == e->hill2);
  assert(-1 == e->face2);
  e->hill2 = hill;
  e->face2 = face;
}

void convexHullMarkEdgeAsProcessed(convexHull *hull, int firstVertex,
    int secondVertex) {
  int mapIndex = firstVertex * arrayGetLength(hull->vertexArray) +
    secondVertex;
  int unitIndex = mapIndex / (sizeof(unsigned int) * 8);
  int unitOffset = mapIndex % (sizeof(unsigned int) * 8);
  hull->openEdgeProcessedMap[unitIndex] |= (0x00000001 << unitOffset);
}

int convexHullOpenEdgeProcessed(convexHull *hull, int firstVertex,
    int secondVertex) {
  if (hull->openEdgeProcessedMap) {
    int mapIndex = firstVertex * arrayGetLength(hull->vertexArray) +
      secondVertex;
    int unitIndex = mapIndex / (sizeof(unsigned int) * 8);
    int unitOffset = mapIndex % (sizeof(unsigned int) * 8);
    return hull->openEdgeProcessedMap[unitIndex] & (0x00000001 << unitOffset);
  }
  return 0;
}

int convexHullAddVertex(convexHull *hull, vec3 *vertex, int plane,
    int orderOnPlane) {
  convexHullVertex *vtx;
  int newVertex = arrayGetLength(hull->vertexArray);
  arraySetLength(hull->vertexArray, newVertex + 1);
  vtx = arrayGetItem(hull->vertexArray, newVertex);
  vtx->plane = plane;
  vtx->orderOnPlane = orderOnPlane;
  vtx->pt = *vertex;
  return newVertex;
}

void convexHullAddTodoNoCheck(convexHull *hull, int firstVertex,
    int secondVertex, int thirdVertex) {
  todo *t = arrayNewItemClear(hull->todoArray);
  t->firstVertex = firstVertex;
  t->secondVertex = secondVertex;
  t->thirdVertex = thirdVertex;
}

static int sortface(const void *first, const void *second) {
  const int *firstIndex = (const void *)first;
  const int *secondIndex = (const void *)second;
  return *firstIndex - *secondIndex;
}

void convexHullAddFace3(convexHull *hull, int firstVertex, int secondVertex,
    int thirdVertex) {
  face3 *tri;
  convexHullVertex *vtx1;
  convexHullVertex *vtx2;
  convexHullVertex *vtx3;
  int newTri;
  int facePlane = -1;
  face3 findFace3;
  
  vtx1 = arrayGetItem(hull->vertexArray, firstVertex);
  vtx2 = arrayGetItem(hull->vertexArray, secondVertex);
  vtx3 = arrayGetItem(hull->vertexArray, thirdVertex);
  
  if (vtx1->plane == vtx2->plane && vtx1->plane == vtx3->plane) {
    facePlane = vtx1->plane;
  }
  
  memset(&findFace3, 0, sizeof(findFace3));
  findFace3.indices[0] = firstVertex;
  findFace3.indices[1] = secondVertex;
  findFace3.indices[2] = thirdVertex;
  qsort(&findFace3.indices, 3, sizeof(findFace3.indices[0]), sortface);
  if (!dictFind(hull->face3Dict, &findFace3)) {
    newTri = arrayGetLength(hull->faceArray);
    tri = dictGetClear(hull->face3Dict, &findFace3);
    *tri = findFace3;
    ((convexHullFace *)tri)->vertexNum = 3;
    ((convexHullFace *)tri)->plane = facePlane;
    convexHullAddEdge(hull, firstVertex, secondVertex, thirdVertex, newTri);
    convexHullAddEdge(hull, secondVertex, thirdVertex, firstVertex, newTri);
    convexHullAddEdge(hull, thirdVertex, firstVertex, secondVertex, newTri);
  }
}

static void convexHullReleaseForGenerate(convexHull *hull) {
  dfree(hull->openEdgeProcessedMap);
  hull->openEdgeProcessedMap = 0;
}

void convexHullDestroy(convexHull *hull) {
  arrayDestroy(hull->vertexArray);
  arrayDestroy(hull->todoArray);
  arrayDestroy(hull->faceArray);
  arrayDestroy(hull->edgeArray);
  dictDestroy(hull->edgeDict);
  dictDestroy(hull->face3Dict);
  convexHullReleaseForGenerate(hull);
  dfree(hull);
}

static void convexHullPrepareForGenerate(convexHull *hull) {
  dfree(hull->openEdgeProcessedMap);
  hull->openEdgeProcessedMap = dcalloc(
    arrayGetLength(hull->vertexArray) * arrayGetLength(hull->vertexArray) /
      (sizeof(unsigned int) * 8) + 1,
    sizeof(unsigned int));
  hull->nextTodoIndex = 0;
}

int convexHullGetNextVertex(convexHull *hull, int p1Index, int p2Index,
    int p3Index) {
  vec3 *p1 = arrayGetItem(hull->vertexArray, p1Index);
  vec3 *p2 = arrayGetItem(hull->vertexArray, p2Index);
  vec3 *p3 = arrayGetItem(hull->vertexArray, p3Index);
  vec3 beginNormal;
  vec3 endNormal;
  int i;
  float angle;
  float maxAngle = 0;
  int candidateIndex = -1;

  vec3Normal(p1, p2, p3, &beginNormal);
  
  for (i = 0; i < arrayGetLength(hull->vertexArray); ++i) {
    if (i != p1Index && i != p2Index && i != p3Index) {
      vec3Normal(p1, p2, arrayGetItem(hull->vertexArray, i), &endNormal);
      angle = vec3Angle(&beginNormal, &endNormal);
      if (angle > maxAngle) {
        candidateIndex = i;
        maxAngle = angle;
      }
    }
  }
  
  return candidateIndex;
}

void convexHullAddTodo(convexHull *hull, int vertex1, int vertex2,
    int vertex3) {
  if (!convexHullOpenEdgeProcessed(hull, vertex1, vertex2)) {
    convexHullAddTodoNoCheck(hull, vertex1, vertex2, vertex3);
  }
}

static int convexHullCanAddFace3(convexHull *hull, int index1, int index2,
    int index3) {
  int i;
  int indices[] = {index1, index2, index3};
  
  for (i = 0; i < 3; ++i) {
    int p1 = indices[i];
    int p2 = indices[(i + 1) % 3];
    int hill = indices[(i + 2) % 3];
    float angle;
    edge *e = convexHullFindEdge(hull, p1, p2);
    if (e) {
      vec3 normal;
      if (-1 != e->hill2) {
        return 0;
      }
      vec3Normal((vec3 *)arrayGetItem(hull->vertexArray, e->p1),
        (vec3 *)arrayGetItem(hull->vertexArray, e->p2),
        (vec3 *)arrayGetItem(hull->vertexArray, hill), &normal);
      angle = vec3Angle(&e->hill1Normal, &normal);
      
      if (angle < 1) {
        return 0;
      }
    }
  }
  return 1;
}

void convexHullGenerate(convexHull *hull) {
  int index1, index2, index3;
  convexHullReleaseForGenerate(hull);
  convexHullPrepareForGenerate(hull);
  while (hull->nextTodoIndex < arrayGetLength(hull->todoArray)) {
    todo *t = arrayGetItem(hull->todoArray, hull->nextTodoIndex++);
    index1 = t->firstVertex;
    index2 = t->secondVertex;
    if (convexHullOpenEdgeProcessed(hull, index1, index2) ||
        convexHullOpenEdgeProcessed(hull, index2, index1)) {
      continue;
    }
    convexHullMarkEdgeAsProcessed(hull, index1, index2);
    index3 = convexHullGetNextVertex(hull, index1, index2, t->thirdVertex);
    if (-1 == index3) {
      continue;
    }
    if (!convexHullCanAddFace3(hull, index1, index2, index3)) {
      continue;
    }
    convexHullAddFace3(hull, index1, index2, index3);
    convexHullAddTodo(hull, index2, index3, index1);
    convexHullAddTodo(hull, index3, index1, index2);
  }
}

void convexHullUnifyNormals(convexHull *hull, vec3 *origin) {
  int i;
  for (i = 0; i < arrayGetLength(hull->faceArray); ++i) {
    face3 *triIdx = arrayGetItem(hull->faceArray, i);
    convexHullVertex *p1 = arrayGetItem(hull->vertexArray, triIdx->indices[0]);
    convexHullVertex *p2 = arrayGetItem(hull->vertexArray, triIdx->indices[1]);
    convexHullVertex *p3 = arrayGetItem(hull->vertexArray, triIdx->indices[2]);
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
}

static int sortEdgeByScore(const void *first, const void *second) {
    edge *e1 = (edge *)first;
    edge *e2 = (edge *)second;
    int result = e1->angleBetweenFaces - e2->angleBetweenFaces;
    if (0 == result) {
      result = e1->angleWithY - e2->angleWithY;
    }
    return result;
}

static void rollTriangleIndices(face3 *face) {
  int i;
  int oldIndices[] = {face->indices[0],
    face->indices[1],
    face->indices[2]};
  for (i = 0; i < 3; ++i) {
    face->indices[(i + 1) % 3] = oldIndices[i];
  }
}

void convexHullMergeTriangles(convexHull *hull) {
  int edgeIndex;
  
  for (edgeIndex = 0; edgeIndex < arrayGetLength(hull->edgeArray);
      ++edgeIndex) {
    edge *e = arrayGetItem(hull->edgeArray, edgeIndex);
    if (-1 != e->face1 && -1 != e->face2) {
      vec3 f1normal;
      vec3 f2normal;
      const vec3 yAxis = {0, 1, 0};
      vec3 *v1 = arrayGetItem(hull->vertexArray, e->p1);
      vec3 *v2 = arrayGetItem(hull->vertexArray, e->p2);
      vec3 *vHill1 = arrayGetItem(hull->vertexArray, e->hill1);
      vec3 *vHill2 = arrayGetItem(hull->vertexArray, e->hill2);
      vec3 v12;
      vec3Sub(v1, v2, &v12);
      vec3Normal(v1, vHill1, v2, &f1normal);
      vec3Normal(v2, vHill2, v1, &f2normal);
      e->angleBetweenFaces = (int)vec3Angle(&f1normal, &f2normal);
      e->angleWithY = (int)vec3Angle(&v12, (vec3 *)&yAxis);
    }
  }
  
  qsort(arrayGetItem(hull->edgeArray, 0), arrayGetLength(hull->edgeArray),
    sizeof(edge), sortEdgeByScore);
  
  //
  // After sort by score, the edge hashmap can not be used anymore.
  //
  dictDestroy(hull->edgeDict);
  hull->edgeDict = 0;
  
  for (edgeIndex = 0; edgeIndex < arrayGetLength(hull->edgeArray);
      ++edgeIndex) {
    edge *e = arrayGetItem(hull->edgeArray, edgeIndex);
    if (-1 != e->face1 && -1 != e->face2) {
      convexHullFace *f1 = arrayGetItem(hull->faceArray, e->face1);
      convexHullFace *f2 = arrayGetItem(hull->faceArray, e->face2);
      if (3 == f1->vertexNum && 3 == f2->vertexNum) {
        if (e->angleBetweenFaces <= 40 &&
            f1->plane == f2->plane) {
          while (e->p1 == f1->u.t.indices[0] || e->p2 == f1->u.t.indices[0]) {
            rollTriangleIndices((face3 *)f1);
          }
          f1->u.q.indices[3] = f1->u.q.indices[2];
          f1->u.q.indices[2] = e->hill2;
          f1->vertexNum = 4;
          f2->vertexNum = 0;
        }
      }
    }
  }
  
  // After merge, face3 hashtable can not be used anymore.
  dictDestroy(hull->face3Dict);
  hull->face3Dict = 0;
}

convexHullFace *convexHullGetFace(convexHull *hull, int faceIndex) {
  convexHullFace *face = arrayGetItem(hull->faceArray, faceIndex);
  return face;
}

convexHullVertex *convexHullGetVertex(convexHull *hull, int vertexIndex) {
  convexHullVertex *vertex = arrayGetItem(hull->vertexArray, vertexIndex);
  return vertex;
}

int convexHullGetFaceNum(convexHull *hull) {
  return arrayGetLength(hull->faceArray);
}
