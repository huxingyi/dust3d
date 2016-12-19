#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "bmesh.h"
#include "array.h"

struct bmesh {
  array *nodeArray;
  array *edgeArray;
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
  return bm;
}

void bmeshDestroy(bmesh *bm) {
  arrayDestroy(bm->nodeArray);
  arrayDestroy(bm->edgeArray);
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
  if (0 != arraySetLength(bm->nodeArray, index + 1)) {
    fprintf(stderr, "%s:arraySetLength failed.\n", __FUNCTION__);
    return -1;
  }
  memcpy(arrayGetItem(bm->nodeArray, index), node, sizeof(bmeshNode));
  return index;
}

int bmeshAddEdge(bmesh *bm, bmeshEdge *edge) {
  int index = arrayGetLength(bm->edgeArray);
  edge->index = index;
  if (0 != arraySetLength(bm->edgeArray, index + 1)) {
    fprintf(stderr, "%s:arraySetLength failed.\n", __FUNCTION__);
    return -1;
  }
  memcpy(arrayGetItem(bm->edgeArray, index), edge, sizeof(bmeshEdge));
  return index;
}
