#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <math.h>
#include <time.h>
#include "bmesh.h"
#include "array.h"
#include "hashtable.h"
#include "matrix.h"
#include "convexhull.h"
#include "subdivide.h"
#include "draw.h"

#define BMESH_MAX_PARENT_BALL_DEPTH   1000
#define BMESH_INTVAL_DIST_DIV         10
#define BMESH_MODEL_VERTEX_HASHTABLE_SIZE   100

typedef struct bmeshBallIndex {
  int ballIndex;
  int nextChildIndex;
} bmeshBallIndex;

typedef struct bmeshModelVertex {
  vec3 vertex;
  int indexOnModel;
} bmeshModelVertex;

struct bmesh {
  array *ballArray;
  array *boneArray;
  array *indexArray;
  array *quadArray;
  int rootBallIndex;
  int roundColor;
  bmeshBall *parentBallStack[BMESH_MAX_PARENT_BALL_DEPTH];
  subdivModel *model;
  subdivModel *subdivModel;
  array *modelVertexArray;
  hashtable *modelVertexHashtable;
  bmeshModelVertex findModelVertex;
};

static bmeshModelVertex *bmeshFindModelVertex(bmesh *bm, vec3 *vertex) {
  int index;
  bm->findModelVertex.vertex = *vertex;
  index = (char *)hashtableGet(bm->modelVertexHashtable, (char *)0) - (char *)0;
  if (0 == index) {
    return 0;
  }
  return (bmeshModelVertex *)arrayGetItem(bm->modelVertexArray, index - 1);
}

static int bmeshAddModelVertex(bmesh *bm, vec3 *vertex) {
  bmeshModelVertex *v = bmeshFindModelVertex(bm, vertex);
  if (!v) {
    v = (bmeshModelVertex *)arrayNewItem(bm->modelVertexArray);
    if (!v) {
      fprintf(stderr, "%s:arrayNewItem failed.\n", __FUNCTION__);
      return -1;
    }
    memset(v, 0, sizeof(bmeshModelVertex));
    v->vertex = *vertex;
    v->indexOnModel = subdivAddVertex(bm->model, &v->vertex);
    if (-1 == hashtableInsert(bm->modelVertexHashtable,
        (char *)0 + arrayGetLength(bm->modelVertexArray))) {
      fprintf(stderr, "%s:hashtableInsert failed.\n", __FUNCTION__);
      return -1;
    }
  }
  return v->indexOnModel;
}

static int bmeshAddQuadToModel(bmesh *bm, quad *q) {
  int indices[4];
  indices[0] = bmeshAddModelVertex(bm, &q->pt[0]);
  indices[1] = bmeshAddModelVertex(bm, &q->pt[1]);
  indices[2] = bmeshAddModelVertex(bm, &q->pt[2]);
  indices[3] = bmeshAddModelVertex(bm, &q->pt[3]);
  if (-1 == indices[0] || -1 == indices[1] || -1 == indices[2] ||
      -1 == indices[3]) {
    fprintf(stderr, "%s:bmeshAddModelVertex failed.\n", __FUNCTION__);
    return -1;
  }
  if (-1 == subdivAddQuadFace(bm->model, indices[0], indices[1], indices[2],
      indices[3])) {
    fprintf(stderr, "%s:subdivAddQuadFace failed.\n", __FUNCTION__);
    return -1;
  }
  return 0;
}

static int bmeshAddTriangleToModel(bmesh *bm, triangle *t) {
  int indices[3];
  indices[0] = bmeshAddModelVertex(bm, &t->pt[0]);
  indices[1] = bmeshAddModelVertex(bm, &t->pt[1]);
  indices[2] = bmeshAddModelVertex(bm, &t->pt[2]);
  if (-1 == indices[0] || -1 == indices[1] || -1 == indices[2]) {
    fprintf(stderr, "%s:bmeshAddModelVertex failed.\n", __FUNCTION__);
    return -1;
  }
  if (-1 == subdivAddTriangleFace(bm->model, indices[0], indices[1],
      indices[2])) {
    fprintf(stderr, "%s:subdivAddTriangleFace failed.\n", __FUNCTION__);
    return -1;
  }
  return 0;
}

bmeshModelVertex *bmeshGetModelVertexByHashtableParam(void *userData,
    const void *node) {
  bmesh *bm = (bmesh *)userData;
  int index = (char *)node - (char *)0;
  if (0 == index) {
    return &bm->findModelVertex;
  }
  return (bmeshModelVertex *)arrayGetItem(bm->modelVertexArray, index - 1);
}

static int modelVertexHash(void *userData, const void *node) {
  bmeshModelVertex *v = bmeshGetModelVertexByHashtableParam(userData, node);
  return abs(*((int *)v));
}

static int modelVertexCompare(void *userData, const void *firstNode,
    const void *secondNode) {
  bmeshModelVertex *v1 = bmeshGetModelVertexByHashtableParam(userData,
    firstNode);
  bmeshModelVertex *v2 = bmeshGetModelVertexByHashtableParam(userData,
    secondNode);
  if (0 == v1->vertex.x - v2->vertex.x &&
      0 == v1->vertex.y - v2->vertex.y &&
      0 == v1->vertex.z - v2->vertex.z) {
    return 0;
  }
  return 1;
}

bmesh *bmeshCreate(void) {
  bmesh *bm = (bmesh *)calloc(1, sizeof(bmesh));
  if (!bm) {
    fprintf(stderr, "%s:Insufficient memory.\n", __FUNCTION__);
    return 0;
  }
  bm->ballArray = arrayCreate(sizeof(bmeshBall));
  if (!bm->ballArray) {
    fprintf(stderr, "%s:arrayCreate bmeshBall failed.\n", __FUNCTION__);
    bmeshDestroy(bm);
    return 0;
  }
  bm->boneArray = arrayCreate(sizeof(bmeshBone));
  if (!bm->boneArray) {
    fprintf(stderr, "%s:arrayCreate bmeshBone failed.\n", __FUNCTION__);
    bmeshDestroy(bm);
    return 0;
  }
  bm->indexArray = arrayCreate(sizeof(bmeshBallIndex));
  if (!bm->indexArray) {
    fprintf(stderr, "%s:arrayCreate bmeshBallIndex failed.\n", __FUNCTION__);
    bmeshDestroy(bm);
    return 0;
  }
  bm->quadArray = arrayCreate(sizeof(quad));
  if (!bm->quadArray) {
    fprintf(stderr, "%s:arrayCreate quad failed.\n", __FUNCTION__);
    bmeshDestroy(bm);
    return 0;
  }
  bm->model = subdivCreateModel();
  if (!bm->model) {
    fprintf(stderr, "%s:subdivCreateModel failed.\n", __FUNCTION__);
    bmeshDestroy(bm);
    return 0;
  }
  bm->modelVertexArray = arrayCreate(sizeof(bmeshModelVertex));
  if (!bm->modelVertexArray) {
    fprintf(stderr, "%s:arrayCreate quad failed.\n", __FUNCTION__);
    bmeshDestroy(bm);
    return 0;
  }
  bm->modelVertexHashtable = hashtableCreate(BMESH_MODEL_VERTEX_HASHTABLE_SIZE,
    modelVertexHash, modelVertexCompare, bm);
  if (!bm->modelVertexHashtable) {
    fprintf(stderr, "%s:hashtableCreate quad failed.\n", __FUNCTION__);
    bmeshDestroy(bm);
    return 0;
  }
  bm->rootBallIndex = -1;
  bm->roundColor = 0;
  return bm;
}

void bmeshDestroy(bmesh *bm) {
  arrayDestroy(bm->ballArray);
  arrayDestroy(bm->boneArray);
  arrayDestroy(bm->indexArray);
  arrayDestroy(bm->quadArray);
  subdivDestroyModel(bm->model);
  subdivDestroyModel(bm->subdivModel);
  arrayDestroy(bm->modelVertexArray);
  hashtableDestroy(bm->modelVertexHashtable);
  free(bm);
}

int bmeshGetBallNum(bmesh *bm) {
  return arrayGetLength(bm->ballArray);
}

int bmeshGetBoneNum(bmesh *bm) {
  return arrayGetLength(bm->boneArray);
}

bmeshBall *bmeshGetBall(bmesh *bm, int index) {
  return (bmeshBall *)arrayGetItem(bm->ballArray, index);
}

bmeshBone *bmeshGetBone(bmesh *bm, int index) {
  return (bmeshBone *)arrayGetItem(bm->boneArray, index);
}

int bmeshAddBall(bmesh *bm, bmeshBall *ball) {
  int index = arrayGetLength(bm->ballArray);
  ball->index = index;
  ball->firstChildIndex = -1;
  ball->childrenIndices = 0;
  ball->notFitHull = 0;
  if (0 != arraySetLength(bm->ballArray, index + 1)) {
    fprintf(stderr, "%s:arraySetLength failed.\n", __FUNCTION__);
    return -1;
  }
  memcpy(arrayGetItem(bm->ballArray, index), ball, sizeof(bmeshBall));
  if (BMESH_BALL_TYPE_ROOT == ball->type) {
    bm->rootBallIndex = index;
  }
  return index;
}

static int bmeshAddChildBallRelation(bmesh *bm, int parentBallIndex,
    int childBallIndex) {
  bmeshBall *parentBall = bmeshGetBall(bm, parentBallIndex);
  bmeshBallIndex *indexItem;
  int newChildIndex = arrayGetLength(bm->indexArray);
  if (0 != arraySetLength(bm->indexArray, newChildIndex + 1)) {
    fprintf(stderr, "%s:arraySetLength failed.\n", __FUNCTION__);
    return -1;
  }
  indexItem = (bmeshBallIndex *)arrayGetItem(bm->indexArray, newChildIndex);
  indexItem->ballIndex = childBallIndex;
  indexItem->nextChildIndex = parentBall->firstChildIndex;
  parentBall->firstChildIndex = newChildIndex;
  parentBall->childrenIndices++;
  return 0;
}

int bmeshAddBone(bmesh *bm, bmeshBone *bone) {
  int index = arrayGetLength(bm->boneArray);
  bone->index = index;
  if (0 != arraySetLength(bm->boneArray, index + 1)) {
    fprintf(stderr, "%s:arraySetLength failed.\n", __FUNCTION__);
    return -1;
  }
  memcpy(arrayGetItem(bm->boneArray, index), bone, sizeof(bmeshBone));
  if (0 != bmeshAddChildBallRelation(bm, bone->firstBallIndex,
      bone->secondBallIndex)) {
    fprintf(stderr, "%s:bmeshAddChildBallRelation failed.\n", __FUNCTION__);
    return -1;
  }
  if (0 != bmeshAddChildBallRelation(bm, bone->secondBallIndex,
      bone->firstBallIndex)) {
    fprintf(stderr, "%s:bmeshAddChildBallRelation failed.\n", __FUNCTION__);
    return -1;
  }
  return index;
}

static int bmeshAddInbetweenBallBetween(bmesh *bm,
    bmeshBall *firstBall, bmeshBall *secondBall, float frac,
    int parentBallIndex) {
  bmeshBall newBall;
  memset(&newBall, 0, sizeof(newBall));
  newBall.type = BMESH_BALL_TYPE_INBETWEEN;
  newBall.radius = firstBall->radius * (1 - frac) +
    secondBall->radius * frac;
  vec3Lerp(&firstBall->position, &secondBall->position, frac,
    &newBall.position);
  if (-1 == bmeshAddBall(bm, &newBall)) {
    fprintf(stderr, "%s:bmeshAddBall failed.\n", __FUNCTION__);
    return -1;
  }
  if (-1 == bmeshAddChildBallRelation(bm, parentBallIndex, newBall.index)) {
    fprintf(stderr, "%s:bmeshAddChildBallRelation failed.\n", __FUNCTION__);
    return -1;
  }
  return newBall.index;
}

/*
static int bmeshGenerateBallCrossSection(bmesh *bm, bmeshBall *ball,
    vec3 *boneDirection, vec3 *localYaxis, vec3 *localZaxis) {
  //int i;
  //quad q;
  vec3 z, y;
  //vec3Scale(localYaxis, ball->radius, &y);
  //vec3Scale(localZaxis, ball->radius, &z);
  vec3Sub(&ball->position, &y, &q.pt[0]);
  vec3Add(&q.pt[0], &z, &q.pt[0]);
  vec3Sub(&ball->position, &y, &q.pt[1]);
  vec3Sub(&q.pt[1], &z, &q.pt[1]);
  vec3Add(&ball->position, &y, &q.pt[2]);
  vec3Sub(&q.pt[2], &z, &q.pt[2]);
  vec3Add(&ball->position, &y, &q.pt[3]);
  vec3Add(&q.pt[3], &z, &q.pt[3]);
  ball->crossSection = q;
  ball->boneDirection = *boneDirection;
  if (-1 == bmeshAddQuad(bm, &q)) {
    fprintf(stderr, "%s:meshAddQuad failed.\n", __FUNCTION__);
    return -1;
  }
  if (connectWithQuad >= 0) {
    for (i = 0; i < 4; ++i) {
      quad face;
      quad *lastQ = bmeshGetQuad(bm, connectWithQuad);
      face.pt[0].x = lastQ->pt[(0 + i) % 4].x;
      face.pt[0].y = lastQ->pt[(0 + i) % 4].y;
      face.pt[0].z = lastQ->pt[(0 + i) % 4].z;
      face.pt[1].x = q.pt[(0 + i) % 4].x;
      face.pt[1].y = q.pt[(0 + i) % 4].y;
      face.pt[1].z = q.pt[(0 + i) % 4].z;
      face.pt[2].x = q.pt[(1 + i) % 4].x;
      face.pt[2].y = q.pt[(1 + i) % 4].y;
      face.pt[2].z = q.pt[(1 + i) % 4].z;
      face.pt[3].x = lastQ->pt[(1 + i) % 4].x;
      face.pt[3].y = lastQ->pt[(1 + i) % 4].y;
      face.pt[3].z = lastQ->pt[(1 + i) % 4].z;
      if (-1 == bmeshAddQuad(bm, &face)) {
        fprintf(stderr, "%s:meshAddQuad failed.\n", __FUNCTION__);
        return -1;
      }
    }
  }
  return 0;
}*/

static void generateYZfromBoneDirection(vec3 *boneDirection,
    vec3 *localYaxis, vec3 *localZaxis) {
  vec3 worldYaxis = {0, 1, 0};
  vec3 worldXaxis = {1, 0, 0};
  //if (0 == vec3Angle(boneDirection, &worldYaxis)) {
  //  vec3CrossProduct(&worldXaxis, boneDirection, localYaxis);
  //} else {
    vec3CrossProduct(&worldYaxis, boneDirection, localYaxis);
  //}
  vec3Normalize(localYaxis);
  vec3CrossProduct(localYaxis, boneDirection, localZaxis);
  vec3Normalize(localZaxis);
}

static int bmeshGenerateInbetweenBallsBetween(bmesh *bm,
      int firstBallIndex, int secondBallIndex) {
  float step;
  float distance;
  int parentBallIndex = firstBallIndex;
  vec3 localZaxis;
  vec3 localYaxis;
  vec3 boneDirection;
  vec3 normalizedBoneDirection;
  bmeshBall *firstBall = bmeshGetBall(bm, firstBallIndex);
  bmeshBall *secondBall = bmeshGetBall(bm, secondBallIndex);
  bmeshBall *newBall;
  float intvalDist;
  if (secondBall->roundColor == bm->roundColor) {
    return 0;
  }
  
  vec3Sub(&firstBall->position, &secondBall->position, &boneDirection);
  normalizedBoneDirection = boneDirection;
  vec3Normalize(&normalizedBoneDirection);
  generateYZfromBoneDirection(&boneDirection,
    &localYaxis, &localZaxis);
  
  intvalDist = (firstBall->radius + secondBall->radius) / BMESH_INTVAL_DIST_DIV;
  step = intvalDist;
  distance = vec3Length(&boneDirection);
  if (distance > intvalDist) {
    float offset;
    int calculatedStepCount = (int)(distance / intvalDist);
    float remaining = distance - intvalDist * calculatedStepCount;
    step += remaining / calculatedStepCount;
    offset = step;
    if (offset < distance) {
      while (offset < distance && offset + intvalDist <= distance) {
        float frac = offset / distance;
        parentBallIndex = bmeshAddInbetweenBallBetween(bm,
          firstBall, secondBall, frac, parentBallIndex);
        if (-1 == parentBallIndex) {
          return -1;
        }
        newBall = bmeshGetBall(bm, parentBallIndex);
        newBall->localYaxis = localYaxis;
        newBall->localZaxis = localZaxis;
        newBall->boneDirection = normalizedBoneDirection;
        offset += step;
      }
    } else if (distance > step) {
      parentBallIndex = bmeshAddInbetweenBallBetween(bm, firstBall, secondBall,
        0.5, parentBallIndex);
      if (-1 == parentBallIndex) {
        return -1;
      }
      newBall = bmeshGetBall(bm, parentBallIndex);
      newBall->localYaxis = localYaxis;
      newBall->localZaxis = localZaxis;
      newBall->boneDirection = normalizedBoneDirection;
    }
  }
  if (-1 == bmeshAddChildBallRelation(bm, parentBallIndex, secondBallIndex)) {
    fprintf(stderr, "%s:bmeshAddChildBallRelation failed.\n", __FUNCTION__);
    return -1;
  }
  return 0;
}

bmeshBall *bmeshGetBallFirstChild(bmesh *bm, bmeshBall *ball,
    bmeshBallIterator *iterator) {
  if (-1 == ball->firstChildIndex) {
    return 0;
  }
  *iterator = ball->firstChildIndex;
  return bmeshGetBallNextChild(bm, ball, iterator);
}

bmeshBall *bmeshGetBallNextChild(bmesh *bm, bmeshBall *ball,
    bmeshBallIterator *iterator) {
  bmeshBallIndex *indexItem;
  if (-1 == *iterator) {
    return 0;
  }
  indexItem = (bmeshBallIndex *)arrayGetItem(bm->indexArray, *iterator);
  *iterator = indexItem->nextChildIndex;
  return bmeshGetBall(bm, indexItem->ballIndex);
}

bmeshBall *bmeshGetRootBall(bmesh *bm) {
  if (-1 == bm->rootBallIndex) {
    return 0;
  }
  return bmeshGetBall(bm, bm->rootBallIndex);
}

static int bmeshGenerateInbetweenBallsFrom(bmesh *bm, int parentBallIndex) {
  bmeshBallIterator iterator;
  int ballIndex;
  bmeshBall *parent;
  bmeshBall *ball;
  int oldChildrenIndices;

  parent = bmeshGetBall(bm, parentBallIndex);
  if (parent->roundColor == bm->roundColor) {
    return 0;
  }
  parent->roundColor = bm->roundColor;

  //
  // Old indices came from user's input will be removed
  // after the inbetween balls are genereated, though
  // the space occupied in indexArray will not been release.
  //

  ball = bmeshGetBallFirstChild(bm, parent, &iterator);
  parent->firstChildIndex = -1;
  oldChildrenIndices = parent->childrenIndices;
  parent->childrenIndices = 0;
  
  for (;
      ball;
      ball = bmeshGetBallNextChild(bm, parent, &iterator)) {
    ballIndex = ball->index;
    if (0 != bmeshGenerateInbetweenBallsBetween(bm, parentBallIndex,
        ballIndex)) {
      fprintf(stderr,
        "%s:bmeshGenerateInbetweenBallsBetween failed(parentBallIndex:%d).\n",
        __FUNCTION__, parentBallIndex);
      return -1;
    }
    if (0 != bmeshGenerateInbetweenBallsFrom(bm, ballIndex)) {
      fprintf(stderr,
        "%s:bmeshGenerateInbetweenBallsFrom failed(ballIndex:%d).\n",
        __FUNCTION__, ballIndex);
      return -1;
    }
  }

  return 0;
}

int bmeshGenerateInbetweenBalls(bmesh *bm) {
  if (-1 == bm->rootBallIndex) {
    fprintf(stderr, "%s:No root ball.\n", __FUNCTION__);
    return -1;
  }
  bm->roundColor++;
  return bmeshGenerateInbetweenBallsFrom(bm, bm->rootBallIndex);
}

int bmeshGetQuadNum(bmesh *bm) {
  return arrayGetLength(bm->quadArray);
}

quad *bmeshGetQuad(bmesh *bm, int index) {
  return (quad *)arrayGetItem(bm->quadArray, index);
}

int bmeshAddQuad(bmesh *bm, quad *q) {
  int index = arrayGetLength(bm->quadArray);
  if (0 != arraySetLength(bm->quadArray, index + 1)) {
    fprintf(stderr, "%s:arraySetLength failed.\n", __FUNCTION__);
    return -1;
  }
  memcpy(arrayGetItem(bm->quadArray, index), q, sizeof(quad));
  return index;
}

static int bmeshSweepFrom(bmesh *bm, bmeshBall *parent, bmeshBall *ball) {
  int result = 0;
  bmeshBallIterator iterator;
  bmeshBall *child = 0;
  if (bm->roundColor == ball->roundColor) {
    return 0;
  }
  ball->roundColor = bm->roundColor;
  if (BMESH_BALL_TYPE_KEY == ball->type) {
    child = bmeshGetBallFirstChild(bm, ball, &iterator);
    if (child) {
      if (parent) {
        float rotateAngle;
        vec3 rotateAxis;
        vec3CrossProduct(&parent->boneDirection, &child->boneDirection,
          &rotateAxis);
        vec3Normalize(&rotateAxis);
        rotateAngle = vec3Angle(&parent->boneDirection, &child->boneDirection);
        ball->boneDirection = parent->boneDirection;
        rotateAxis.y = 0;
        rotateAngle *= 0.5;
        vec3RotateAlong(&ball->boneDirection, rotateAngle, &rotateAxis,
          &ball->boneDirection);
        generateYZfromBoneDirection(&ball->boneDirection,
          &ball->localYaxis, &ball->localZaxis);
      }
    } else {
      ball->boneDirection = parent->boneDirection;
      generateYZfromBoneDirection(&ball->boneDirection,
        &ball->localYaxis, &ball->localZaxis);
    }
  }
  for (child = bmeshGetBallFirstChild(bm, ball, &iterator);
      child;
      child = bmeshGetBallNextChild(bm, ball, &iterator)) {
    result = bmeshSweepFrom(bm, ball, child);
    if (0 != result) {
      fprintf(stderr, "%s:bmeshSweepFrom failed.\n", __FUNCTION__);
      return result;
    }
  }
  return result;
}

int bmeshSweep(bmesh *bm) {
  bm->roundColor++;
  return bmeshSweepFrom(bm, 0, bmeshGetRootBall(bm));
}

static int isDistanceEnoughForConvexHull(bmeshBall *root,
      bmeshBall *ball) {
  float distance = vec3Distance(&root->position, &ball->position);
  if (distance >= root->radius) {
    return 1;
  }
  return 0;
}

static bmeshBall *bmeshFindChildBallForConvexHull(bmesh *bm, bmeshBall *root,
      bmeshBall *ball) {
  bmeshBallIterator iterator;
  bmeshBall *child;
  if (ball->convexHullCount) {
    return ball;
  }
  if (!ball->notFitHull && isDistanceEnoughForConvexHull(root, ball)) {
    return ball;
  }
  child = bmeshGetBallFirstChild(bm, ball, &iterator);
  if (!child) {
    return ball;
  }
  ball->radius = 0;
  return bmeshFindChildBallForConvexHull(bm, root, child);
}

static bmeshBall *bmeshFindParentBallForConvexHull(bmesh *bm, bmeshBall *root,
    int depth, bmeshBall *ball) {
  if (depth <= 0 || ball->convexHullCount) {
    return ball;
  }
  if (!ball->notFitHull && isDistanceEnoughForConvexHull(root, ball)) {
    return ball;
  }
  ball->radius = 0;
  return bmeshFindParentBallForConvexHull(bm, root, depth - 1,
    bm->parentBallStack[depth - 1]);
}

static int addBallToHull(convexHull *hull, bmeshBall *ballForConvexHull,
    bmeshBall **outmostBall, int *outmostBallFirstVertexIndex) {
  vec3 z, y;
  quad q;
  int vertexIndex[4];
  int needUpdateOutmost = 0;
  
  vec3Scale(&ballForConvexHull->localYaxis, ballForConvexHull->radius, &y);
  vec3Scale(&ballForConvexHull->localZaxis, ballForConvexHull->radius, &z);
  vec3Sub(&ballForConvexHull->position, &y, &q.pt[0]);
  vec3Add(&q.pt[0], &z, &q.pt[0]);
  vec3Sub(&ballForConvexHull->position, &y, &q.pt[1]);
  vec3Sub(&q.pt[1], &z, &q.pt[1]);
  vec3Add(&ballForConvexHull->position, &y, &q.pt[2]);
  vec3Sub(&q.pt[2], &z, &q.pt[2]);
  vec3Add(&ballForConvexHull->position, &y, &q.pt[3]);
  vec3Add(&q.pt[3], &z, &q.pt[3]);
  
  vertexIndex[0] = convexHullAddVertex(hull, &q.pt[0],
    ballForConvexHull->index, 0);
  vertexIndex[1] = convexHullAddVertex(hull, &q.pt[1],
    ballForConvexHull->index, 1);
  vertexIndex[2] = convexHullAddVertex(hull, &q.pt[2],
    ballForConvexHull->index, 2);
  vertexIndex[3] = convexHullAddVertex(hull, &q.pt[3],
    ballForConvexHull->index, 3);
  if (-1 == vertexIndex[0] || -1 == vertexIndex[1] || -1 == vertexIndex[2] ||
      -1 == vertexIndex[3]) {
    fprintf(stderr, "%s:convexHullAddVertex failed.\n", __FUNCTION__);
    return -1;
  }
  
  if (*outmostBall) {
    if (ballForConvexHull->radius > (*outmostBall)->radius) {
      needUpdateOutmost = 1;
    }
  } else {
    needUpdateOutmost = 1;
  }
  if (needUpdateOutmost) {
    *outmostBall = ballForConvexHull;
    *outmostBallFirstVertexIndex = vertexIndex[0];
  }
  
  return 0;
}

#include "osutil.h"
int showFaceIndex = 10000000;
static long long lastShowFaceIndexIncTime = 0;

static convexHull *createConvexHullForBall(bmesh *bm, int depth,
    bmeshBall *ball, int *needRetry) {
  convexHull *hull;
  bmeshBallIterator iterator;
  bmeshBall *child;
  bmeshBall *ballForConvexHull;
  bmeshBall *outmostBall = 0;
  int outmostBallFirstVertexIndex = 0;
  array *ballPtrArray;
  bmeshBall **ballPtr;
  int i;
  int hasVertexNotFitOnHull = 0;
  
  *needRetry = 0;
  
  ballPtrArray = arrayCreate(sizeof(bmeshBall *));
  if (!ballPtrArray) {
    fprintf(stderr, "%s:arrayCreate failed.\n", __FUNCTION__);
    return 0;
  }
  hull = convexHullCreate();
  if (!hull) {
    fprintf(stderr, "%s:convexHullCreate failed.\n", __FUNCTION__);
    arrayDestroy(ballPtrArray);
    return 0;
  }
  if (BMESH_BALL_TYPE_KEY == ball->type) {
    bmeshBall reduceBall = *ball;
    reduceBall.radius *= 0.75;
    if (-1 == addBallToHull(hull, &reduceBall,
        &outmostBall, &outmostBallFirstVertexIndex)) {
      fprintf(stderr, "%s:addBallToHull failed.\n", __FUNCTION__);
      arrayDestroy(ballPtrArray);
      convexHullDestroy(hull);
      return 0;
    }
  }
  for (child = bmeshGetBallFirstChild(bm, ball, &iterator);
      child;
      child = bmeshGetBallNextChild(bm, ball, &iterator)) {
    ballForConvexHull = bmeshFindChildBallForConvexHull(bm, ball, child);
    ballPtr = (bmeshBall **)arrayNewItem(ballPtrArray);
    if (!ballPtr) {
      fprintf(stderr, "%s:arrayNewItem failed.\n", __FUNCTION__);
      arrayDestroy(ballPtrArray);
      convexHullDestroy(hull);
      return 0;
    }
    *ballPtr = ballForConvexHull;
    if (-1 == addBallToHull(hull, ballForConvexHull,
        &outmostBall, &outmostBallFirstVertexIndex)) {
      fprintf(stderr, "%s:addBallToHull failed.\n", __FUNCTION__);
      arrayDestroy(ballPtrArray);
      convexHullDestroy(hull);
      return 0;
    }
  }
  if (depth > 0 && depth - 1 < BMESH_MAX_PARENT_BALL_DEPTH) {
    ballForConvexHull = bmeshFindParentBallForConvexHull(bm, ball, depth - 1,
      bm->parentBallStack[depth - 1]);
    ballPtr = (bmeshBall **)arrayNewItem(ballPtrArray);
    if (!ballPtr) {
      fprintf(stderr, "%s:arrayNewItem failed.\n", __FUNCTION__);
      arrayDestroy(ballPtrArray);
      convexHullDestroy(hull);
      return 0;
    }
    *ballPtr = ballForConvexHull;
    if (-1 == addBallToHull(hull, ballForConvexHull,
        &outmostBall, &outmostBallFirstVertexIndex)) {
      fprintf(stderr, "%s:addBallToHull failed.\n", __FUNCTION__);
      arrayDestroy(ballPtrArray);
      convexHullDestroy(hull);
      return 0;
    }
  }
  if (outmostBall) {
    if (-1 == convexHullAddTodo(hull, outmostBallFirstVertexIndex + 0,
        outmostBallFirstVertexIndex + 1, outmostBallFirstVertexIndex + 2)) {
      fprintf(stderr, "%s:convexHullAddTodo failed.\n", __FUNCTION__);
      arrayDestroy(ballPtrArray);
      convexHullDestroy(hull);
      return 0;
    }
  }
  
  for (i = 0; i < arrayGetLength(ballPtrArray); ++i) {
    bmeshBall *ballItem = *((bmeshBall **)arrayGetItem(ballPtrArray, i));
    ballItem->flagForHull = 0;
  }
  
  if (-1 == convexHullGenerate(hull)) {
    fprintf(stderr, "%s:convexHullGenerate failed.\n", __FUNCTION__);
    arrayDestroy(ballPtrArray);
    convexHullDestroy(hull);
    return 0;
  }
  
  convexHullUnifyNormals(hull, &ball->position);
  convexHullMergeTriangles(hull);
  
  for (i = 0; i < convexHullGetFaceNum(hull); ++i) {
    convexHullFace *f = (convexHullFace *)convexHullGetFace(hull, i);
    if (-1 != f->plane) {
      bmeshBall *ballItem = (bmeshBall *)arrayGetItem(bm->ballArray, f->plane);
      ballItem->flagForHull = 1;
      f->vertexNum = 0;
    }
  }
  
  for (i = 0; i < arrayGetLength(ballPtrArray); ++i) {
    bmeshBall *ballItem = *((bmeshBall **)arrayGetItem(ballPtrArray, i));
    if (!ballItem->flagForHull) {
      hasVertexNotFitOnHull = 1;
      if (!ballItem->notFitHull) {
        *needRetry = 1;
        ballItem->notFitHull = 1;
        arrayDestroy(ballPtrArray);
        convexHullDestroy(hull);
        return 0;
      }
    }
  }
  
  if (hasVertexNotFitOnHull) {
    fprintf(stderr, "%s:hasVertexNotFitOnHull.\n", __FUNCTION__);
    arrayDestroy(ballPtrArray);
    convexHullDestroy(hull);
    return 0;
  }
  
  for (i = 0; i < arrayGetLength(ballPtrArray); ++i) {
    bmeshBall *ballItem = *((bmeshBall **)arrayGetItem(ballPtrArray, i));
    ballItem->convexHullCount++;
    drawDebugPrintf("convexHullCount:%d", ballItem->convexHullCount);
    if (ballItem->convexHullCount == 1) {
      drawDebugPoint(&ballItem->position, ballItem->index);
    }
  }
  
  arrayDestroy(ballPtrArray);
  return hull;
}

static int bmeshStichFrom(bmesh *bm, int depth, bmeshBall *ball) {
  bmeshBallIterator iterator;
  bmeshBall *child;
  int result = 0;
  if (bm->roundColor == ball->roundColor) {
    return 0;
  }
  if (depth < BMESH_MAX_PARENT_BALL_DEPTH) {
    bm->parentBallStack[depth] = ball;
  }
  ball->roundColor = bm->roundColor;
  if ((BMESH_BALL_TYPE_ROOT == ball->type ||
        BMESH_BALL_TYPE_KEY == ball->type) &&
      bmeshGetBallFirstChild(bm, ball, &iterator)) {
    convexHull *hull = 0;
    
    for (;;) {
      int needRetry = 0;
      hull = createConvexHullForBall(bm, depth, ball, &needRetry);
      if (hull) {
        break;
      }
      if (!needRetry) {
        break;
      }
    }
    
    if (hull) {
      int i;
      for (i = 0; i < convexHullGetFaceNum(hull); ++i) {
        convexHullFace *face = (convexHullFace *)convexHullGetFace(hull, i);
        if (4 == face->vertexNum) {
          quad q;
          q.pt[0] = convexHullGetVertex(hull, face->u.q.indices[0])->pt;
          q.pt[1] = convexHullGetVertex(hull, face->u.q.indices[1])->pt;
          q.pt[2] = convexHullGetVertex(hull, face->u.q.indices[2])->pt;
          q.pt[3] = convexHullGetVertex(hull, face->u.q.indices[3])->pt;
          result = bmeshAddQuadToModel(bm, &q);
          if (-1 == result) {
            break;
          }
        } else if (3 == face->vertexNum) {
          triangle t;
          t.pt[0] = convexHullGetVertex(hull, face->u.t.indices[0])->pt;
          t.pt[1] = convexHullGetVertex(hull, face->u.t.indices[1])->pt;
          t.pt[2] = convexHullGetVertex(hull, face->u.t.indices[2])->pt;
          result = bmeshAddTriangleToModel(bm, &t);
          if (-1 == result) {
            break;
          }
        }
      }
      convexHullDestroy(hull);
    }
  }

  for (child = bmeshGetBallFirstChild(bm, ball, &iterator);
      child;
      child = bmeshGetBallNextChild(bm, ball, &iterator)) {
    result = bmeshStichFrom(bm, depth + 1, child);
    if (0 != result) {
      fprintf(stderr, "%s:bmeshSweepFrom failed.\n", __FUNCTION__);
      return result;
    }
  }
  return result;
}

int bmeshStitch(bmesh *bm) {
  bm->roundColor++;
  memset(bm->parentBallStack, 0, sizeof(bm->parentBallStack));
  return bmeshStichFrom(bm, 0, bmeshGetRootBall(bm));
}

static void rollQuadVertexs(quad *q) {
  int i;
  quad oldQuad = *q;
  for (i = 0; i < 4; ++i) {
    q->pt[i] = oldQuad.pt[(i + 1) % 4];
  }
}

static void matchTwoFaces(quad *q1, quad *q2) {
  int i;
  float minDistance = 9999;
  int matchTo = 0;
  for (i = 0; i < 4; ++i) {
    float distance = vec3Distance(&q1->pt[0], &q2->pt[i]);
    if (distance < minDistance) {
      minDistance = distance;
      matchTo = i;
    }
  }
  for (i = 0; i < matchTo; ++i) {
    rollQuadVertexs(q2);
  }
}

void calculateBallQuad(bmeshBall *ball, quad *q) {
  vec3 z, y;
  vec3Scale(&ball->localYaxis, ball->radius, &y);
  vec3Scale(&ball->localZaxis, ball->radius, &z);
  vec3Sub(&ball->position, &y, &q->pt[0]);
  vec3Add(&q->pt[0], &z, &q->pt[0]);
  vec3Sub(&ball->position, &y, &q->pt[1]);
  vec3Sub(&q->pt[1], &z, &q->pt[1]);
  vec3Add(&ball->position, &y, &q->pt[2]);
  vec3Sub(&q->pt[2], &z, &q->pt[2]);
  vec3Add(&ball->position, &y, &q->pt[3]);
  vec3Add(&q->pt[3], &z, &q->pt[3]);
}

static int bmeshAddWallsBetweenQuadsToModel(bmesh *bm, vec3 *origin, quad *q1,
    quad *q2) {
  int i;
  int result = 0;
  vec3 normal;
  vec3 o2v;
  matchTwoFaces(q1, q2);
  for (i = 0; i < 4; ++i) {
    quad wall = {{q1->pt[i], q2->pt[i],
      q2->pt[(i + 1) % 4], q1->pt[(i + 1) % 4]}};
    vec3Normal(&wall.pt[0], &wall.pt[1], &wall.pt[2], &normal);
    vec3Sub(&wall.pt[0], origin, &o2v);
    if (vec3DotProduct(&o2v, &normal) < 0) {
      int j;
      quad oldWall = wall;
      for (j = 0; j < 4; ++j) {
        wall.pt[j] = oldWall.pt[3 - j];
      }
    }
    result = bmeshAddQuadToModel(bm, &wall);
    if (-1 == result) {
      break;
    }
    //drawQuad(&wall);
  }
  return result;
}

static bmeshBall *bmeshFindChildBallForInbetweenMesh(bmesh *bm, bmeshBall *ball) {
  bmeshBallIterator iterator;
  bmeshBall *child;
  if (ball->convexHullCount > 0) {
    return ball;
  }
  child = bmeshGetBallFirstChild(bm, ball, &iterator);
  if (!child) {
    return ball;
  }
  return bmeshFindChildBallForInbetweenMesh(bm, child);
}

static int bmeshGenerateInbetweenMeshFrom(bmesh *bm, int depth,
    bmeshBall *ball) {
  int result = 0;
  bmeshBallIterator iterator;
  bmeshBall *child = 0;
  bmeshBall *parent;
  quad currentFace, childFace;
  if (bm->roundColor == ball->roundColor) {
    return 0;
  }
  if (depth < BMESH_MAX_PARENT_BALL_DEPTH) {
    bm->parentBallStack[depth] = ball;
  }
  ball->roundColor = bm->roundColor;
  calculateBallQuad(ball, &currentFace);
  if (BMESH_BALL_TYPE_KEY == ball->type &&
      !bmeshGetBallFirstChild(bm, ball, &iterator)) {
    if (depth - 1 >= 0) {
      bmeshBall fakeBall;
      parent = bm->parentBallStack[depth - 1];
      fakeBall = *ball;
      vec3Lerp(&parent->position, &ball->position, BMESH_INTVAL_DIST_DIV,
          &fakeBall.position);
      calculateBallQuad(&fakeBall, &childFace);
      bmeshAddWallsBetweenQuadsToModel(bm, &ball->position, &currentFace,
        &childFace);
      result = bmeshAddQuadToModel(bm, &childFace);
      if (-1 == result) {
        return result;
      }
      drawQuad(&childFace);
    }
  }
  else if (1 == ball->convexHullCount && !ball->meshGenerated) {
    child = bmeshGetBallFirstChild(bm, ball, &iterator);
    if (child && !child->meshGenerated) {
      //if (vec3Distance(&ball->position, &child->position) <=
      //    (ball->radius + child->radius) / 10) {
        // TODO: merge two face
      //} else {
        child = bmeshFindChildBallForInbetweenMesh(bm, child);
        calculateBallQuad(child, &childFace);
        result = bmeshAddWallsBetweenQuadsToModel(bm, &ball->position, &currentFace,
          &childFace);
        if (-1 == result) {
          return result;
        }
      //}
      ball->meshGenerated = 1;
      child->meshGenerated = 1;
    }
  }
  for (child = bmeshGetBallFirstChild(bm, ball, &iterator);
      child;
      child = bmeshGetBallNextChild(bm, ball, &iterator)) {
    result = bmeshGenerateInbetweenMeshFrom(bm, depth + 1, child);
    if (0 != result) {
      fprintf(stderr, "%s:bmeshGenerateInbetweenMeshFrom failed.\n",
        __FUNCTION__);
      return result;
    }
  }
  return result;
}

int bmeshGenerateInbetweenMesh(bmesh *bm) {
  bm->roundColor++;
  memset(bm->parentBallStack, 0, sizeof(bm->parentBallStack));
  return bmeshGenerateInbetweenMeshFrom(bm, 0, bmeshGetRootBall(bm));
}

int bmeshGenerate(bmesh *bm) {
  bmeshGenerateInbetweenBalls(bm);
  bmeshSweep(bm);
  bmeshStitch(bm);
  bmeshGenerateInbetweenMesh(bm);
  subdivCalculteNorms(bm->model);
  //bm->subdivModel = subdivCatmullClark(bm->model);
  bm->subdivModel = subdivCatmullClarkWithLoops(bm->model, 1);
  return 0;
}

int bmeshDraw(bmesh *bm) {
  glPushMatrix();
  glTranslatef(-7, 0, 0);
  subdivDrawModel(bm->model);
  glPopMatrix();
  subdivDrawModel(bm->subdivModel);
  return 0;
}

