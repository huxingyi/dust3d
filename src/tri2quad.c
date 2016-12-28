#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "tri2quad.h"
#include "vector3d.h"
#include "array.h"
#include "hashtable.h"

#define EDGE_HASHTABLE_SIZE   100

typedef struct edge {
  vec3 p1;
  vec3 p2;
  int inputA;
  int inputB;
  int score;
} edge;

typedef struct inputTriangle {
  triangle tri;
  float area;
  vec3 normal;
  int merged;
} inputTriangle;

struct tri2QuadContext {
  array *inputArray;
  array *triangleArray;
  array *quadArray;
  array *edgeArray;
  hashtable *edgeHashtable;
  edge findEdge;
};

static edge *tri2QuadGetEdgeByHashtableParam(tri2QuadContext *ctx,
    const void *param) {
  int edgeIndex = (char *)param - (char *)0;
  if (0 == edgeIndex) {
    return &ctx->findEdge;
  }
  return (edge *)arrayGetItem(ctx->edgeArray, edgeIndex - 1);
}

static int edgeHash(void *userData, const void *node) {
  edge *e = tri2QuadGetEdgeByHashtableParam((tri2QuadContext *)userData, node);
  return (int)(e->p1.x + 1) * (e->p1.y + 1) * (e->p1.z + 1) *
    (e->p2.x + 1) * (e->p2.y + 1) * (e->p2.z + 1);
}

static int edgeCompare(void *userData, const void *node1, const void *node2) {
  const edge *e1 = (const edge *)tri2QuadGetEdgeByHashtableParam(
    (tri2QuadContext *)userData, node1);
  const edge *e2 = (const edge *)tri2QuadGetEdgeByHashtableParam(
    (tri2QuadContext *)userData, node2);
  if ((0 == memcmp(&e1->p1, &e2->p1, sizeof(e1->p1)) &&
        0 == memcmp(&e1->p2, &e2->p2, sizeof(e1->p1))) ||
      (0 == memcmp(&e1->p1, &e2->p2, sizeof(e1->p1)) &&
        0 == memcmp(&e1->p2, &e2->p1, sizeof(e1->p1)))) {
    return 0;
  }
  return -1;
}

tri2QuadContext *tri2QuadContextCreate(void) {
  tri2QuadContext *ctx = (tri2QuadContext *)calloc(1, sizeof(tri2QuadContext));
  if (!ctx) {
    fprintf(stderr, "%s:Insufficient memory.", __FUNCTION__);
    return 0;
  }
  ctx->inputArray = arrayCreate(sizeof(inputTriangle));
  if (!ctx->inputArray) {
    fprintf(stderr, "%s:arrayCreate failed.", __FUNCTION__);
    tri2QuadContextDestroy(ctx);
    return 0;
  }
  ctx->triangleArray = arrayCreate(sizeof(triangle));
  if (!ctx->triangleArray) {
    fprintf(stderr, "%s:arrayCreate failed.", __FUNCTION__);
    tri2QuadContextDestroy(ctx);
    return 0;
  }
  ctx->quadArray = arrayCreate(sizeof(quad));
  if (!ctx->quadArray) {
    fprintf(stderr, "%s:arrayCreate failed.", __FUNCTION__);
    tri2QuadContextDestroy(ctx);
    return 0;
  }
  ctx->edgeHashtable = hashtableCreate(EDGE_HASHTABLE_SIZE, edgeHash,
    edgeCompare, ctx);
  if (!ctx->edgeHashtable) {
    fprintf(stderr, "%s:hashtableCreate failed.", __FUNCTION__);
    tri2QuadContextDestroy(ctx);
    return 0;
  }
  ctx->edgeArray = arrayCreate(sizeof(edge));
  if (!ctx->edgeArray) {
    fprintf(stderr, "%s:arrayCreate failed.", __FUNCTION__);
    tri2QuadContextDestroy(ctx);
    return 0;
  }
  return ctx;
}

int tri2QuadAddTriangle(tri2QuadContext *ctx, triangle *tri) {
  int newInputIndex;
  inputTriangle *newInput;
  newInputIndex = arrayGetLength(ctx->inputArray);
  if (0 != arraySetLength(ctx->inputArray, newInputIndex + 1)) {
    fprintf(stderr, "%s:arraySetLength failed.", __FUNCTION__);
    return -1;
  }
  newInput = (inputTriangle *)arrayGetItem(ctx->inputArray, newInputIndex);
  newInput->merged = 0;
  newInput->tri = *tri;
  newInput->area = vec3TriangleArea(&tri->pt[0], &tri->pt[1], &tri->pt[2]);
  vec3Normal(&tri->pt[0], &tri->pt[1], &tri->pt[2], &newInput->normal);
  return newInputIndex;
}

static edge *tri2QuadFindEdge(tri2QuadContext *ctx,
    vec3 *p1, vec3 *p2) {
  int edgeIndex;
  ctx->findEdge.p1 = *p1;
  ctx->findEdge.p2 = *p2;
  edgeIndex = (char *)hashtableGet(ctx->edgeHashtable, (char *)0) - (char *)0;
  if (0 == edgeIndex) {
    return 0;
  }
  return (edge *)arrayGetItem(ctx->edgeArray, edgeIndex - 1);
}

static int tri2QuadAddInitialEdge(tri2QuadContext *ctx,
    vec3 *p1, vec3 *p2, int inputIndex) {
  edge *existedEdge = tri2QuadFindEdge(ctx, p1, p2);
  if (!existedEdge) {
    edge *newEdge;
    int newEdgeIndex = arrayGetLength(ctx->edgeArray);
    if (0 != arraySetLength(ctx->edgeArray, newEdgeIndex + 1)) {
      fprintf(stderr, "%s:arraySetLength failed(newEdgeIndex:%d).",
        __FUNCTION__, newEdgeIndex);
      return -1;
    }
    newEdge = (edge *)arrayGetItem(ctx->edgeArray, newEdgeIndex);
    newEdge->p1 = *p1;
    newEdge->p2 = *p2;
    if (0 != hashtableInsert(ctx->edgeHashtable,
        (char *)0 + newEdgeIndex + 1)) {
      fprintf(stderr, "%s:hashtableInsert failed.", __FUNCTION__);
      return -1;
    }
    newEdge->inputA = inputIndex;
    newEdge->inputB = -1;
    newEdge->score = 0;
  } else {
    existedEdge->inputB = inputIndex;
  }
  return 0;
}

static int tri2QuadAddOutputQuad(tri2QuadContext *ctx, vec3 *first,
    vec3 *second, vec3 *third, vec3 *fourth) {
  quad *q;
  int newQuadIndex = arrayGetLength(ctx->quadArray);
  if (0 != arraySetLength(ctx->quadArray, newQuadIndex + 1)) {
    fprintf(stderr, "%s:arraySetLength failed(newQuadIndex:%d).", __FUNCTION__,
      newQuadIndex);
    return -1;
  }
  q = (quad *)arrayGetItem(ctx->quadArray, newQuadIndex);
  memset(q, 0, sizeof(quad));
  q->pt[0] = *first;
  q->pt[1] = *second;
  q->pt[2] = *third;
  q->pt[3] = *fourth;
  return newQuadIndex;
}

static int tri2QuadAddOutputTriangle(tri2QuadContext *ctx, vec3 *first,
    vec3 *second, vec3 *third) {
  triangle *tri;
  int newTriangleIndex = arrayGetLength(ctx->triangleArray);
  if (0 != arraySetLength(ctx->triangleArray, newTriangleIndex + 1)) {
    fprintf(stderr, "%s:arraySetLength failed(newTriangleIndex:%d).",
      __FUNCTION__, newTriangleIndex);
    return -1;
  }
  tri = (triangle *)arrayGetItem(ctx->triangleArray, newTriangleIndex);
  memset(tri, 0, sizeof(triangle));
  tri->pt[0] = *first;
  tri->pt[1] = *second;
  tri->pt[2] = *third;
  return newTriangleIndex;
}

static int tri2QuadAddOutputQuadByEdgeAndInput(tri2QuadContext *ctx, edge *e,
    inputTriangle *input1, inputTriangle *input2) {
  vec3 outputPt[4];
  int i;
  int nextSaveToIndex;
  memset(outputPt, 0, sizeof(outputPt));
  nextSaveToIndex = 0;
  for (i = 0; i < 3; ++i) {
    vec3 *pt = &input1->tri.pt[i];
    if (0 == memcmp(&e->p1, pt, sizeof(e->p1))) {
      outputPt[nextSaveToIndex] = *pt;
      nextSaveToIndex += 2;
    } else if (0 == memcmp(&e->p2, pt, sizeof(e->p2))) {
      outputPt[nextSaveToIndex] = *pt;
      nextSaveToIndex += 2;
    } else {
      outputPt[1] = *pt;
    }
  }
  for (i = 0; i < 3; ++i) {
    vec3 *pt = &input2->tri.pt[i];
    if (0 != memcmp(&e->p1, pt, sizeof(e->p1)) &&
        0 != memcmp(&e->p2, pt, sizeof(e->p2))) {
      outputPt[3] = *pt;
      break;
    }
  }
  return tri2QuadAddOutputQuad(ctx, &outputPt[0], &outputPt[1],
    &outputPt[2], &outputPt[3]);
}

static int sortEdgeByScoreDesc(const void *first, const void *second) {
  const edge *e1 = (const edge *)first;
  const edge *e2 = (const edge *)second;
  return e2->score - e1->score;
}

int tri2QuadConvert(tri2QuadContext *ctx) {
  int inputIndex;
  int edgeIndex;
  
  for (inputIndex = 0; inputIndex < arrayGetLength(ctx->inputArray);
      ++inputIndex) {
    inputTriangle *inputItem = (inputTriangle *)arrayGetItem(ctx->inputArray,
      inputIndex);
    tri2QuadAddInitialEdge(ctx, &inputItem->tri.pt[0], &inputItem->tri.pt[1],
      inputIndex);
    tri2QuadAddInitialEdge(ctx, &inputItem->tri.pt[1], &inputItem->tri.pt[2],
      inputIndex);
    tri2QuadAddInitialEdge(ctx, &inputItem->tri.pt[2], &inputItem->tri.pt[0],
      inputIndex);
  }
  for (edgeIndex = 0; edgeIndex < arrayGetLength(ctx->edgeArray); ++edgeIndex) {
    edge *e = (edge *)arrayGetItem(ctx->edgeArray, edgeIndex);
    inputTriangle *input1;
    inputTriangle *input2;
    if (-1 != e->inputA && -1 != e->inputB) {
      input1 = (inputTriangle *)arrayGetItem(ctx->inputArray,
        e->inputA);
      input2 = (inputTriangle *)arrayGetItem(ctx->inputArray,
        e->inputB);
      e->score = (int)((input1->area + input2->area) *
        vec3DotProduct(&input1->normal, &input2->normal) * 100);
    }
  }
  qsort(arrayGetItem(ctx->edgeArray, 0), arrayGetLength(ctx->edgeArray),
    sizeof(edge), sortEdgeByScoreDesc);
  
  //
  // After qsort, the edge indices inside the hashtable are no longer right.
  //
  
  hashtableDestroy(ctx->edgeHashtable);
  ctx->edgeHashtable = 0;
  
  for (edgeIndex = 0; edgeIndex < arrayGetLength(ctx->edgeArray); ++edgeIndex) {
    edge *e = (edge *)arrayGetItem(ctx->edgeArray, edgeIndex);
    inputTriangle *input1;
    inputTriangle *input2;
    if (-1 != e->inputA && -1 != e->inputB) {
      input1 = (inputTriangle *)arrayGetItem(ctx->inputArray,
        e->inputA);
      input2 = (inputTriangle *)arrayGetItem(ctx->inputArray,
        e->inputB);
      if (!input1->merged && !input2->merged) {
        //input1->merged = 1;
        //input2->merged = 1;
        //tri2QuadAddOutputQuadByEdgeAndInput(ctx, e, input1, input2);
      }
    }
  }
  
  for (inputIndex = 0; inputIndex < arrayGetLength(ctx->inputArray);
      ++inputIndex) {
    inputTriangle *inputItem = (inputTriangle *)arrayGetItem(ctx->inputArray,
      inputIndex);
    if (!inputItem->merged) {
      tri2QuadAddOutputTriangle(ctx, &inputItem->tri.pt[0],
        &inputItem->tri.pt[1], &inputItem->tri.pt[2]);
    }
  }
  
  return 0;
}

int tri2QuadGetTriangleNum(tri2QuadContext *ctx) {
  return arrayGetLength(ctx->triangleArray);
}

triangle *tri2QuadGetTriangle(tri2QuadContext *ctx, int index) {
  return (triangle *)arrayGetItem(ctx->triangleArray, index);
}

int tri2QuadGetQuadNum(tri2QuadContext *ctx) {
  return arrayGetLength(ctx->quadArray);
}

quad *tri2QuadGetQuad(tri2QuadContext *ctx, int index) {
  return (quad *)arrayGetItem(ctx->quadArray, index);
}

void tri2QuadContextDestroy(tri2QuadContext *ctx) {
  arrayDestroy(ctx->inputArray);
  arrayDestroy(ctx->triangleArray);
  arrayDestroy(ctx->quadArray);
  hashtableDestroy(ctx->edgeHashtable);
  arrayDestroy(ctx->edgeArray);
  free(ctx);
}
