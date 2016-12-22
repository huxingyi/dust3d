#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <math.h>
#include "bmesh.h"
#include "array.h"
#include "matrix.h"

typedef struct bmeshNodeIndex {
  int nodeIndex;
  int nextChildIndex;
} bmeshNodeIndex;

struct bmesh {
  array *nodeArray;
  array *edgeArray;
  array *indexArray;
  array *quadArray;
  int rootNodeIndex;
  int roundColor;
};

bmesh *bmeshCreate(void) {
  bmesh *bm = (bmesh *)calloc(1, sizeof(bmesh));
  if (!bm) {
    fprintf(stderr, "%s:Insufficient memory.\n", __FUNCTION__);
    return 0;
  }
  bm->nodeArray = arrayCreate(sizeof(bmeshNode));
  if (!bm->nodeArray) {
    fprintf(stderr, "%s:arrayCreate bmeshNode failed.\n", __FUNCTION__);
    bmeshDestroy(bm);
    return 0;
  }
  bm->edgeArray = arrayCreate(sizeof(bmeshEdge));
  if (!bm->edgeArray) {
    fprintf(stderr, "%s:arrayCreate bmeshEdge failed.\n", __FUNCTION__);
    bmeshDestroy(bm);
    return 0;
  }
  bm->indexArray = arrayCreate(sizeof(bmeshNodeIndex));
  if (!bm->indexArray) {
    fprintf(stderr, "%s:arrayCreate bmeshNodeIndex failed.\n", __FUNCTION__);
    bmeshDestroy(bm);
    return 0;
  }
  bm->quadArray = arrayCreate(sizeof(quad));
  if (!bm->quadArray) {
    fprintf(stderr, "%s:arrayCreate quad failed.\n", __FUNCTION__);
    bmeshDestroy(bm);
    return 0;
  }
  bm->rootNodeIndex = -1;
  bm->roundColor = 0;
  return bm;
}

void bmeshDestroy(bmesh *bm) {
  arrayDestroy(bm->nodeArray);
  arrayDestroy(bm->edgeArray);
  arrayDestroy(bm->indexArray);
  arrayDestroy(bm->quadArray);
  free(bm);
}

int bmeshGetNodeNum(bmesh *bm) {
  return arrayGetLength(bm->nodeArray);
}

int bmeshGetEdgeNum(bmesh *bm) {
  return arrayGetLength(bm->edgeArray);
}

bmeshNode *bmeshGetNode(bmesh *bm, int index) {
  return (bmeshNode *)arrayGetItem(bm->nodeArray, index);
}

bmeshEdge *bmeshGetEdge(bmesh *bm, int index) {
  return (bmeshEdge *)arrayGetItem(bm->edgeArray, index);
}

int bmeshAddNode(bmesh *bm, bmeshNode *node) {
  int index = arrayGetLength(bm->nodeArray);
  node->index = index;
  node->firstChildIndex = -1;
  node->childrenIndices = 0;
  if (0 != arraySetLength(bm->nodeArray, index + 1)) {
    fprintf(stderr, "%s:arraySetLength failed.\n", __FUNCTION__);
    return -1;
  }
  memcpy(arrayGetItem(bm->nodeArray, index), node, sizeof(bmeshNode));
  if (BMESH_NODE_TYPE_ROOT == node->type) {
    bm->rootNodeIndex = index;
  }
  return index;
}

static int bmeshAddChildNodeRelation(bmesh *bm, int parentNodeIndex,
    int childNodeIndex) {
  bmeshNode *parentNode = bmeshGetNode(bm, parentNodeIndex);
  bmeshNodeIndex *indexItem;
  int newChildIndex = arrayGetLength(bm->indexArray);
  if (0 != arraySetLength(bm->indexArray, newChildIndex + 1)) {
    fprintf(stderr, "%s:arraySetLength failed.\n", __FUNCTION__);
    return -1;
  }
  indexItem = (bmeshNodeIndex *)arrayGetItem(bm->indexArray, newChildIndex);
  indexItem->nodeIndex = childNodeIndex;
  indexItem->nextChildIndex = parentNode->firstChildIndex;
  parentNode->firstChildIndex = newChildIndex;
  parentNode->childrenIndices++;
  return 0;
}

int bmeshAddEdge(bmesh *bm, bmeshEdge *edge) {
  int index = arrayGetLength(bm->edgeArray);
  edge->index = index;
  if (0 != arraySetLength(bm->edgeArray, index + 1)) {
    fprintf(stderr, "%s:arraySetLength failed.\n", __FUNCTION__);
    return -1;
  }
  memcpy(arrayGetItem(bm->edgeArray, index), edge, sizeof(bmeshEdge));
  if (0 != bmeshAddChildNodeRelation(bm, edge->firstNodeIndex,
      edge->secondNodeIndex)) {
    fprintf(stderr, "%s:bmeshAddChildNodeRelation failed.\n", __FUNCTION__);
    return -1;
  }
  if (0 != bmeshAddChildNodeRelation(bm, edge->secondNodeIndex,
      edge->firstNodeIndex)) {
    fprintf(stderr, "%s:bmeshAddChildNodeRelation failed.\n", __FUNCTION__);
    return -1;
  }
  return index;
}

static int bmeshAddInbetweenNodeBetween(bmesh *bm,
    bmeshNode *firstNode, bmeshNode *secondNode, float frac,
    int parentNodeIndex) {
  bmeshNode newNode;
  memset(&newNode, 0, sizeof(newNode));
  newNode.type = BMESH_NODE_TYPE_INBETWEEN;
  newNode.radius = firstNode->radius * (1 - frac) +
    secondNode->radius * frac;
  vec3Lerp(&firstNode->position, &secondNode->position, frac,
    &newNode.position);
  if (-1 == bmeshAddNode(bm, &newNode)) {
    fprintf(stderr, "%s:bmeshAddNode failed.\n", __FUNCTION__);
    return -1;
  }
  if (-1 == bmeshAddChildNodeRelation(bm, parentNodeIndex, newNode.index)) {
    fprintf(stderr, "%s:bmeshAddChildNodeRelation failed.\n", __FUNCTION__);
    return -1;
  }
  return newNode.index;
}

static void floatsToQuad(float *floats, quad *q) {
  int i;
  int offset = 0;
  for (i = 0; i < 4; ++i) {
    q->pt[i].x = floats[offset++];
    q->pt[i].y = floats[offset++];
    q->pt[i].z = floats[offset++];
  }
}

static int bmeshGenerateNodeQuad(bmesh *bm, bmeshNode *node,
    matrix *matRotate, int connectWithQuad) {
  quad q;
  matrix matTranslate;
  matrix matFinal;
  int i;
  float floats[4][3] = {
    {-node->radius, +node->radius, 0},
    {-node->radius, -node->radius, 0},
    {+node->radius, -node->radius, 0},
    {+node->radius, +node->radius, 0},
  };
  matrixTranslate(&matTranslate, node->position.x, node->position.y,
    node->position.z);
  matrixLoadIdentity(&matFinal);
  matrixAppend(&matFinal, &matTranslate);
  matrixAppend(&matFinal, matRotate);
  matrixTransformVector(&matFinal, floats[0]);
  matrixTransformVector(&matFinal, floats[1]);
  matrixTransformVector(&matFinal, floats[2]);
  matrixTransformVector(&matFinal, floats[3]);
  floatsToQuad(&floats[0][0], &q);
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
}

static int bmeshGenerateInbetweenNodesBetween(bmesh *bm,
      int firstNodeIndex, int secondNodeIndex) {
  float step = 0.5;
  float distance;
  int parentNodeIndex = firstNodeIndex;
  float rotateAngle = 0;
  vec3 rotateAround = {0, 0, 0};
  vec3 p;
  vec3 zAxis = {0, 0, 1};
  matrix matRotate;
  int lastQuadIndex = -1;
  
  bmeshNode *firstNode = bmeshGetNode(bm, firstNodeIndex);
  bmeshNode *secondNode = bmeshGetNode(bm, secondNodeIndex);
  bmeshNode *newNode;
  if (secondNode->roundColor == bm->roundColor) {
    return 0;
  }
  vec3Sub(&firstNode->position, &secondNode->position, &p);
  vec3CrossProduct(&zAxis, &p, &rotateAround);
  vec3Normalize(&rotateAround);
  
  distance = vec3Length(&p);
  if (distance > 0) {
    float offset = step;
    rotateAngle = 180 / M_PI * acos(vec3DotProduct(&zAxis, &p) / distance);
    matrixRotate(&matRotate,
      rotateAngle, rotateAround.x, rotateAround.y, rotateAround.z);
    if (offset + step <= distance) {
      while (offset + step <= distance) {
        float frac = offset / distance;
        parentNodeIndex = bmeshAddInbetweenNodeBetween(bm,
          firstNode, secondNode, frac, parentNodeIndex);
        if (-1 == parentNodeIndex) {
          return -1;
        }
        newNode = bmeshGetNode(bm, parentNodeIndex);
        bmeshGenerateNodeQuad(bm, newNode, &matRotate,
          lastQuadIndex);
        lastQuadIndex = -1 == lastQuadIndex ? bmeshGetQuadNum(bm) - 1 :
          bmeshGetQuadNum(bm) - 5;
        offset += step;
      }
    } else if (distance > step) {
      parentNodeIndex = bmeshAddInbetweenNodeBetween(bm, firstNode, secondNode,
        0.5, parentNodeIndex);
      if (-1 == parentNodeIndex) {
        return -1;
      }
      newNode = bmeshGetNode(bm, parentNodeIndex);
      bmeshGenerateNodeQuad(bm, newNode, &matRotate, lastQuadIndex);
      lastQuadIndex = -1 == lastQuadIndex ? bmeshGetQuadNum(bm) - 1 :
          bmeshGetQuadNum(bm) - 5;
    }
  }
  if (-1 == bmeshAddChildNodeRelation(bm, parentNodeIndex, secondNodeIndex)) {
    fprintf(stderr, "%s:bmeshAddChildNodeRelation failed.\n", __FUNCTION__);
    return -1;
  }
  return 0;
}

int bmeshGetNodeNextChild(bmesh *bm, bmeshNode *node, int *childIndex) {
  int currentChildIndex = *childIndex;
  bmeshNodeIndex *indexItem;
  if (-1 == currentChildIndex) {
    if (-1 == node->firstChildIndex) {
      return -1;
    }
    currentChildIndex = node->firstChildIndex;
  }
  indexItem = (bmeshNodeIndex *)arrayGetItem(bm->indexArray, currentChildIndex);
  *childIndex = indexItem->nextChildIndex;
  return indexItem->nodeIndex;
}

bmeshNode *bmeshGetRootNode(bmesh *bm) {
  if (-1 == bm->rootNodeIndex) {
    return 0;
  }
  return bmeshGetNode(bm, bm->rootNodeIndex);
}

static int bmeshGenerateInbetweenNodesFrom(bmesh *bm, int parentNodeIndex) {
  int childIndex = -1;
  int nodeIndex;
  bmeshNode *parent;

  parent = bmeshGetNode(bm, parentNodeIndex);
  if (parent->roundColor == bm->roundColor) {
    return 0;
  }
  parent->roundColor = bm->roundColor;

  //
  // Old indices came from user's input will be removed
  // after the inbetween nodes are genereated, though
  // the space occupied in indexArray will not been release.
  //

  childIndex = parent->firstChildIndex;
  parent->firstChildIndex = -1;
  parent->childrenIndices = 0;

  while (-1 != childIndex) {
    nodeIndex = bmeshGetNodeNextChild(bm, parent, &childIndex);
    if (-1 == nodeIndex) {
      break;
    }
    if (0 != bmeshGenerateInbetweenNodesBetween(bm, parentNodeIndex,
        nodeIndex)) {
      fprintf(stderr,
        "%s:bmeshGenerateInbetweenNodesBetween failed(parentNodeIndex:%d).\n",
        __FUNCTION__, parentNodeIndex);
      return -1;
    }
    if (0 != bmeshGenerateInbetweenNodesFrom(bm, nodeIndex)) {
      fprintf(stderr,
        "%s:bmeshGenerateInbetweenNodesFrom failed(nodeIndex:%d).\n",
        __FUNCTION__, nodeIndex);
      return -1;
    }
  }

  return 0;
}

int bmeshGenerateInbetweenNodes(bmesh *bm) {
  if (-1 == bm->rootNodeIndex) {
    fprintf(stderr, "%s:No root node.\n", __FUNCTION__);
    return -1;
  }
  bm->roundColor++;
  return bmeshGenerateInbetweenNodesFrom(bm, bm->rootNodeIndex);
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


