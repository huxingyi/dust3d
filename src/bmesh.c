#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <math.h>
#include <time.h>
#include "bmesh.h"
#include "array.h"
#include "matrix.h"
#include "convexhull.h"
#include "draw.h"

#define BMESH_STEP_DISTANCE 0.2
#define BMESH_MAX_PARENT_BALL_DEPTH 1000

typedef struct bmeshBallIndex {
  int ballIndex;
  int nextChildIndex;
} bmeshBallIndex;

struct bmesh {
  array *ballArray;
  array *boneArray;
  array *indexArray;
  array *quadArray;
  int rootBallIndex;
  int roundColor;
  bmeshBall *parentBallStack[BMESH_MAX_PARENT_BALL_DEPTH];
};

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
  bm->rootBallIndex = -1;
  bm->roundColor = 0;
  return bm;
}

void bmeshDestroy(bmesh *bm) {
  arrayDestroy(bm->ballArray);
  arrayDestroy(bm->boneArray);
  arrayDestroy(bm->indexArray);
  arrayDestroy(bm->quadArray);
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
  if (0 == vec3Angle(boneDirection, &worldYaxis)) {
    vec3CrossProduct(&worldXaxis, boneDirection, localYaxis);
  } else {
    vec3CrossProduct(&worldYaxis, boneDirection, localYaxis);
  }
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
  if (secondBall->roundColor == bm->roundColor) {
    return 0;
  }
  
  step = BMESH_STEP_DISTANCE;
  
  vec3Sub(&firstBall->position, &secondBall->position, &boneDirection);
  normalizedBoneDirection = boneDirection;
  vec3Normalize(&normalizedBoneDirection);
  generateYZfromBoneDirection(&boneDirection,
    &localYaxis, &localZaxis);
  
  /*
  glColor3f(0.0, 0.0, 0.0);
  drawDebugPrintf("<%f,%f,%f> <%f,%f,%f> <%f,%f,%f>",
    localYaxis.x,
    localYaxis.y,
    localYaxis.z,
    localZaxis.x,
    localZaxis.y,
    localZaxis.z,
    boneDirection.x,
    boneDirection.y,
    boneDirection.z);
  */
  
  distance = vec3Length(&boneDirection);
  if (distance > BMESH_STEP_DISTANCE) {
    float offset;
    int calculatedStepCount = (int)(distance / BMESH_STEP_DISTANCE);
    float remaining = distance - BMESH_STEP_DISTANCE * calculatedStepCount;
    step += remaining / calculatedStepCount;
    offset = step;
    if (offset < distance) {
      while (offset < distance && offset + BMESH_STEP_DISTANCE <= distance) {
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
        /*
        if (1 || 11 == ball->index || 20 == ball->index) {
          glColor3f(0.0, 0.0, 0.0);
          drawDebugPrintf("<%f,%f,%f> <%f,%f,%f> <%f,%f,%f> rotateAngle:%f",
            parent->boneDirection.x,
            parent->boneDirection.y,
            parent->boneDirection.z,
            child->boneDirection.x,
            child->boneDirection.y,
            child->boneDirection.z,
            rotateAxis.x,
            rotateAxis.y,
            rotateAxis.z,
            rotateAngle);
        }*/
        rotateAngle *= 0.5;
        
        /*
        if (11 == ball->index) {
          glPushMatrix();
          //glTranslatef(parent->position.x, parent->position.y, parent->position.z);
          glColor3f(1.0, 0.0, 1.0);
          glBegin(GL_LINES);
          glVertex3f(0, 0, 0);
          glVertex3f(parent->boneDirection.x, parent->boneDirection.y,
            parent->boneDirection.z);
          glEnd();
          glPopMatrix();
          
          glPushMatrix();
          //glTranslatef(child->position.x, child->position.y, child->position.z);
          glColor3f(0.0, 1.0, 1.0);
          glBegin(GL_LINES);
          glVertex3f(0, 0, 0);
          glVertex3f(child->boneDirection.x, child->boneDirection.y,
            child->boneDirection.z);
          glEnd();
          glPopMatrix();
          
          glPushMatrix();
          //glTranslatef(ball->position.x, ball->position.y, ball->position.z);
          glColor3f(1.0, 0.0, 0.0);
          glBegin(GL_LINES);
          glVertex3f(0, 0, 0);
          glVertex3f(rotateAxis.x, rotateAxis.y, rotateAxis.z);
          glEnd();
          glPopMatrix();
        }*/
        
        ball->boneDirection = parent->boneDirection;
        vec3RotateAlong(&ball->boneDirection, rotateAngle, &rotateAxis,
          &ball->boneDirection);
        generateYZfromBoneDirection(&ball->boneDirection,
          &ball->localYaxis, &ball->localZaxis);
      } else {
        // TODO:
      }
    } else {
      ball->boneDirection = parent->boneDirection;
      generateYZfromBoneDirection(&ball->boneDirection,
        &ball->localYaxis, &ball->localZaxis);
      /*
      vec3CrossProduct(&worldYaxis, &ball->boneDirection, &ball->localYaxis);
      vec3Normalize(&ball->localYaxis);
      vec3CrossProduct(&ball->localYaxis, &ball->boneDirection,
        &ball->localZaxis);
      vec3Normalize(&ball->localZaxis);*/
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
  if (distance >= root->radius + BMESH_STEP_DISTANCE) {
    return 1;
  }
  return 0;
}

static bmeshBall *bmeshFindChildBallForConvexHull(bmesh *bm, bmeshBall *root,
      bmeshBall *ball) {
  bmeshBallIterator iterator;
  bmeshBall *child;
  if (isDistanceEnoughForConvexHull(root, ball)) {
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
  if (depth <= 0) {
    return ball;
  }
  if (isDistanceEnoughForConvexHull(root, ball)) {
    return ball;
  }
  ball->radius = 0;
  return bmeshFindParentBallForConvexHull(bm, root, depth - 1,
    bm->parentBallStack[depth - 1]);
}

static void addBallToHull(convexHull *hull, bmeshBall *ballForConvexHull,
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
  
  vertexIndex[0] = convexHullAddVertex(hull, &q.pt[0], ballForConvexHull->index, 0);
  vertexIndex[1] = convexHullAddVertex(hull, &q.pt[1], ballForConvexHull->index, 1);
  vertexIndex[2] = convexHullAddVertex(hull, &q.pt[2], ballForConvexHull->index, 2);
  vertexIndex[3] = convexHullAddVertex(hull, &q.pt[3], ballForConvexHull->index, 3);
  
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
}

#include "osutil.h"
int showFaceIndex = 10000000;
static long long lastShowFaceIndexIncTime = 0;

static int bmeshStichFrom(bmesh *bm, int depth, bmeshBall *ball) {
  int result = 0;
  bmeshBallIterator iterator;
  bmeshBall *child;
  bmeshBall *ballForConvexHull;
  if (bm->roundColor == ball->roundColor) {
    return 0;
  }
  if (depth < BMESH_MAX_PARENT_BALL_DEPTH) {
    bm->parentBallStack[depth] = ball;
  }
  ball->roundColor = bm->roundColor;
  if (BMESH_BALL_TYPE_ROOT == ball->type) {
    convexHull *hull;
    bmeshBall *outmostBall = 0;
    int outmostBallFirstVertexIndex = 0;
    hull = convexHullCreate();
    if (!hull) {
      fprintf(stderr, "%s:convexHullCreate failed.\n", __FUNCTION__);
      return -1;
    }
    glColor3f(0.0, 0.0, 0.0);
    drawDebugPrintf("root <%f,%f,%f>", ball->position.x,
      ball->position.y, ball->position.z);
    for (child = bmeshGetBallFirstChild(bm, ball, &iterator);
        child;
        child = bmeshGetBallNextChild(bm, ball, &iterator)) {
      ballForConvexHull = bmeshFindChildBallForConvexHull(bm, ball, child);
      addBallToHull(hull, ballForConvexHull,
        &outmostBall, &outmostBallFirstVertexIndex);
    }
    if (depth > 0 && depth - 1 < BMESH_MAX_PARENT_BALL_DEPTH) {
      ballForConvexHull = bmeshFindParentBallForConvexHull(bm, ball, depth - 1,
        bm->parentBallStack[depth - 1]);
      addBallToHull(hull, ballForConvexHull,
        &outmostBall, &outmostBallFirstVertexIndex);
    }
    if (outmostBall) {
      convexHullAddTodo(hull, outmostBallFirstVertexIndex + 0,
        outmostBallFirstVertexIndex + 1, outmostBallFirstVertexIndex + 2);
    }
    convexHullGenerate(hull);
    convexHullUnifyNormals(hull, &ball->position);
    convexHullMergeTriangles(hull);
    glPushMatrix();
    
    /*
    if (lastShowFaceIndexIncTime + 0 < osGetMilliseconds()) {
      if (17 == showFaceIndex) {
        if (lastShowFaceIndexIncTime + 0 < osGetMilliseconds()) {
          if (showFaceIndex > convexHullGetFace3Num(hull)) {
            showFaceIndex = 0;
          } else {
            showFaceIndex++;
          }
          lastShowFaceIndexIncTime = osGetMilliseconds();
        }
      } else {
        if (showFaceIndex > convexHullGetFace3Num(hull)) {
          showFaceIndex = 0;
        } else {
          showFaceIndex++;
        }
        lastShowFaceIndexIncTime = osGetMilliseconds();
      }
    }*/
    
    {
      int triIndex;
      for (triIndex = 0; triIndex < convexHullGetFaceNum(hull);
          ++triIndex) {
        convexHullFace *face = (convexHullFace *)convexHullGetFace(hull,
          triIndex);
        if (3 == face->vertexNum) {
          triangle tri;
          int j;
          tri.pt[0] = *convexHullGetVertex(hull, face->u.t.indices[0]);
          tri.pt[1] = *convexHullGetVertex(hull, face->u.t.indices[1]);
          tri.pt[2] = *convexHullGetVertex(hull, face->u.t.indices[2]);
          
          if (triIndex >= showFaceIndex) {
            break;
          }
          
          glColor3f(1.0f, 1.0f, 1.0f);
          drawTriangle(&tri);
          
          glBegin(GL_LINE_STRIP);
          for (j = 0; j < 3; ++j) {
            glVertex3f(tri.pt[j].x, tri.pt[j].y, tri.pt[j].z);
          }
          glVertex3f(tri.pt[0].x, tri.pt[0].y, tri.pt[0].z);
          glEnd();
          
        } else if (4 == face->vertexNum) {
          quad q;
          vec3 normal;
          int j;
          q.pt[0] = *convexHullGetVertex(hull, face->u.q.indices[0]);
          q.pt[1] = *convexHullGetVertex(hull, face->u.q.indices[1]);
          q.pt[2] = *convexHullGetVertex(hull, face->u.q.indices[2]);
          q.pt[3] = *convexHullGetVertex(hull, face->u.q.indices[3]);
          
          glColor3f(1.0f, 1.0f, 1.0f);
          glBegin(GL_QUADS);
          vec3Normal(&q.pt[0], &q.pt[1], &q.pt[2], &normal);
          for (j = 0; j < 4; ++j) {
            glNormal3f(normal.x, normal.y, normal.z);
            glVertex3f(q.pt[j].x, q.pt[j].y, q.pt[j].z);
          }
          glEnd();
          
          glBegin(GL_LINE_STRIP);
          for (j = 0; j < 4; ++j) {
            glVertex3f(q.pt[j].x, q.pt[j].y, q.pt[j].z);
          }
          glVertex3f(q.pt[0].x, q.pt[0].y, q.pt[0].z);
          glEnd();
        }
      }
    }
    
    /*
    glColor3f(0.0f, 0.0f, 0.0f);
    {
      int triIndex;
      int j;
      for (triIndex = 0; triIndex < convexHullGetFaceNum(hull);
          ++triIndex) {
        convexHullFace *face = (convexHullFace *)convexHullGetFace(hull,
          triIndex);
        if (3 == face->vertexNum) {
          triangle tri;
          tri.pt[0] = *convexHullGetVertex(hull, face->u.t.indices[0]);
          tri.pt[1] = *convexHullGetVertex(hull, face->u.t.indices[1]);
          tri.pt[2] = *convexHullGetVertex(hull, face->u.t.indices[2]);
          glBegin(GL_LINE_STRIP);
          for (j = 0; j < 3; ++j) {
            glVertex3f(tri.pt[j].x, tri.pt[j].y, tri.pt[j].z);
          }
          glVertex3f(tri.pt[0].x, tri.pt[0].y, tri.pt[0].z);
          glEnd();
        } else if ()
      }
    }*/
  
    /*
    glColor3f(1.0f, 1.0f, 1.0f);
    {
      int triIndex;
      for (triIndex = 0; triIndex < tri2QuadGetTriangleNum(t2q);
          ++triIndex) {
        triangle *tri = (triangle *)tri2QuadGetTriangle(t2q, triIndex);
        drawTriangle(tri);
      }
    }
    glColor3f(0.0f, 0.0f, 0.0f);
    {
      int triIndex;
      int j;
      for (triIndex = 0; triIndex < tri2QuadGetTriangleNum(t2q);
          ++triIndex) {
        triangle *tri = (triangle *)tri2QuadGetTriangle(t2q, triIndex);
        glBegin(GL_LINE_STRIP);
        for (j = 0; j < 3; ++j) {
          glVertex3f(tri->pt[j].x, tri->pt[j].y, tri->pt[j].z);
        }
        glVertex3f(tri->pt[0].x, tri->pt[0].y, tri->pt[0].z);
        glEnd();
      }
    }
    
    glColor3f(1.0f, 1.0f, 1.0f);
    {
      int quadIndex;
      glBegin(GL_QUADS);
      for (quadIndex = 0; quadIndex < tri2QuadGetQuadNum(t2q);
          ++quadIndex) {
        quad *q = (quad *)tri2QuadGetQuad(t2q, quadIndex);
        vec3 normal;
        int j;
        vec3Normal(&q->pt[0], &q->pt[1], &q->pt[2], &normal);
        for (j = 0; j < 4; ++j) {
          glNormal3f(normal.x, normal.y, normal.z);
          glVertex3f(q->pt[j].x, q->pt[j].y, q->pt[j].z);
        }
      }
      glEnd();
    }
    glColor3f(0.0f, 0.0f, 0.0f);
    {
      int quadIndex;
      int j;
      for (quadIndex = 0; quadIndex < tri2QuadGetQuadNum(t2q);
          ++quadIndex) {
        quad *q = (quad *)tri2QuadGetQuad(t2q, quadIndex);
        glBegin(GL_LINE_STRIP);
        for (j = 0; j < 4; ++j) {
          glVertex3f(q->pt[j].x, q->pt[j].y, q->pt[j].z);
        }
        glVertex3f(q->pt[0].x, q->pt[0].y, q->pt[0].z);
        glEnd();
      }
    }*/
    
    glPopMatrix();
    convexHullDestroy(hull);
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
  return bmeshStichFrom(bm, 0, bmeshGetRootBall(bm));
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

static void drawWallsBetweenQuads(vec3 *origin, quad *q1, quad *q2) {
  int i;
  vec3 normal;
  vec3 o2v;
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
    drawQuad(&wall);
  }
}

static int bmeshGenerateInbetweenMeshFrom(bmesh *bm, bmeshBall *parent,
    bmeshBall *ball) {
  int result = 0;
  bmeshBallIterator iterator;
  bmeshBall *child = 0;
  if (bm->roundColor == ball->roundColor) {
    return 0;
  }
  ball->roundColor = bm->roundColor;
  if (ball->radius > 0) {
    quad currentFace;
    calculateBallQuad(ball, &currentFace);
    if (parent && parent->radius > 0) {
      quad parentFace;
      calculateBallQuad(parent, &parentFace);
      drawWallsBetweenQuads(&ball->position, &parentFace, &currentFace);
      child = bmeshGetBallFirstChild(bm, ball, &iterator);
      if (!child) {
        bmeshBall fakeParentParent = *parent;
        bmeshBall fakeParent = *ball;
        bmeshBall fakeBall;
        for (;;) {
          quad childFace;
          fakeBall = fakeParent;
          vec3Lerp(&fakeParentParent.position, &fakeParent.position, 2,
            &fakeBall.position);
          calculateBallQuad(&fakeBall, &childFace);
          drawWallsBetweenQuads(&fakeBall.position, &currentFace, &childFace);
          if (vec3Distance(&ball->position, &fakeBall.position) >=
              ball->radius) {
            drawQuad(&childFace);
            break;
          }
          fakeParentParent = fakeParent;
          fakeParent = fakeBall;
        }
      }
    }
  }
  for (child = bmeshGetBallFirstChild(bm, ball, &iterator);
      child;
      child = bmeshGetBallNextChild(bm, ball, &iterator)) {
    result = bmeshGenerateInbetweenMeshFrom(bm, ball, child);
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
  return bmeshGenerateInbetweenMeshFrom(bm, 0, bmeshGetRootBall(bm));
}

