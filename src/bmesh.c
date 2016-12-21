#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "bmesh.h"
#include "array.h"

typedef struct bmeshNodeIndex {
  int nodeIndex;
  int nextChildIndex;
} bmeshNodeIndex;

struct bmesh {
  array *nodeArray;
  array *edgeArray;
  array *indexArray;
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
  bm->rootNodeIndex = -1;
  bm->roundColor = 0;
  return bm;
}

void bmeshDestroy(bmesh *bm) {
  arrayDestroy(bm->nodeArray);
  arrayDestroy(bm->edgeArray);
  arrayDestroy(bm->indexArray);
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

static int bmeshGenerateInbetweenNodesBetween(bmesh *bm,
      int firstNodeIndex, int secondNodeIndex) {
  float step = 0.5;
  float distance;
  int parentNodeIndex = firstNodeIndex;
  bmeshNode *firstNode = bmeshGetNode(bm, firstNodeIndex);
  bmeshNode *secondNode = bmeshGetNode(bm, secondNodeIndex);
  if (secondNode->roundColor == bm->roundColor) {
    return 0;
  }
  distance = vec3Distance(&firstNode->position, &secondNode->position);
  if (distance > 0) {
    float offset = step;
    if (offset + step <= distance) {
      while (offset + step <= distance) {
        float frac = offset / distance;
        parentNodeIndex = bmeshAddInbetweenNodeBetween(bm,
          firstNode, secondNode, frac, parentNodeIndex);
        if (-1 == parentNodeIndex) {
          return -1;
        }
        offset += step;
      }
    } else if (distance > step) {
      parentNodeIndex = bmeshAddInbetweenNodeBetween(bm, firstNode, secondNode, 0.5,
        parentNodeIndex);
      if (-1 == parentNodeIndex) {
        return -1;
      }
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
