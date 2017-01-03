#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "subdivide.h"
#include "draw.h"

//
// Catmull Clark Subdivision
// Modified from https://rosettacode.org/wiki/Catmull%E2%80%93Clark_subdivision_surface/C
//

typedef struct subdivLink {
  int index;
  int nextLink;
} subdivLink;

typedef struct subdivVertex {
  vec3 v;
  vec3 avgNorm;
  int index;
  int newVertexIndex;
  int edgeLink;
  int edgeNum;
  int faceLink;
  int faceNum;
  int indexOnNewModel;
} subdivVertex;

typedef struct subdivEdge {
  int index;
  int faceLink;
  int faceNum;
  int v[2];
  int edgePt;
  vec3 avg;
} subdivEdge;

typedef struct subdivFace {
  int index;
  int edgeLink;
  int edgeNum;
  int vertexLink;
  int vertexNum;
  vec3 norm;
  int avg;
} subdivFace;

struct subdivModel {
  array *vertexArray;
  array *faceArray;
  array *edgeArray;
  array *indexArray;
  int faceLink;
  int faceNum;
  int edgeLink;
  int edgeNum;
  int vertexLink;
  int vertexNum;
};

static void initVertex(subdivVertex *v) {
  memset(v, 0, sizeof(subdivVertex));
  v->newVertexIndex = -1;
  v->edgeLink = -1;
  v->faceLink = -1;
  v->indexOnNewModel = -1;
};

static void initEdge(subdivEdge *e) {
  memset(e, 0, sizeof(subdivEdge));
  e->faceLink = -1;
  e->v[0] = -1;
  e->v[0] = -1;
  e->edgePt = -1;
}

static void initFace(subdivFace *f) {
  memset(f, 0, sizeof(subdivFace));
  f->edgeLink = -1;
  f->vertexLink = -1;
  f->avg = -1;
}

subdivModel *subdivCreateModel(void) {
  subdivModel *model = (subdivModel *)calloc(1, sizeof(subdivModel));
  if (!model) {
    fprintf(stderr, "%s:Insufficient memory.\n", __FUNCTION__);
    return 0;
  }
  model->faceLink = -1;
  model->edgeLink = -1;
  model->vertexLink = -1;
  model->vertexArray = arrayCreate(sizeof(subdivVertex));
  if (!model->vertexArray) {
    fprintf(stderr, "%s:Insufficient memory.\n", __FUNCTION__);
    return 0;
  }
  model->faceArray = arrayCreate(sizeof(subdivFace));
  if (!model->faceArray) {
    fprintf(stderr, "%s:Insufficient memory.\n", __FUNCTION__);
    return 0;
  }
  model->edgeArray = arrayCreate(sizeof(subdivEdge));
  if (!model->edgeArray) {
    fprintf(stderr, "%s:Insufficient memory.\n", __FUNCTION__);
    return 0;
  }
  model->indexArray = arrayCreate(sizeof(subdivLink));
  if (!model->indexArray) {
    fprintf(stderr, "%s:Insufficient memory.\n", __FUNCTION__);
    return 0;
  }
  return model;
}

void subdivDestroyModel(subdivModel *model) {
  if (model) {
    arrayDestroy(model->vertexArray);
    arrayDestroy(model->faceArray);
    arrayDestroy(model->edgeArray);
    arrayDestroy(model->indexArray);
    free(model);
  }
}

static int allocLink(subdivModel *model, int index) {
  subdivLink *linkItem;
  int newLink = arrayGetLength(model->indexArray);
  if (0 != arraySetLength(model->indexArray, newLink + 1)) {
    fprintf(stderr, "%s:arraySetLength failed.\n", __FUNCTION__);
    return -1;
  }
  linkItem = (subdivLink *)arrayGetItem(model->indexArray, newLink);
  linkItem->index = index;
  linkItem->nextLink = -1;
  return newLink;
}

static int subdivLinkElement(subdivModel *model, int current, int order) {
  int i, iterator;
  int element = -1;
  subdivLink *linkItem;
  iterator = current;
  for (i = 0; i <= order && -1 != iterator; ++i) {
    linkItem = (subdivLink *)arrayGetItem(model->indexArray, iterator);
    element = linkItem->index;
    iterator = linkItem->nextLink;
  }
  return element;
}

static int pushLink(subdivModel *model, int *link, int *num, int index) {
  int newLink = allocLink(model, index);
  int i, iterator;
  subdivLink *linkItem = 0;
  if (-1 == newLink) {
    fprintf(stderr, "%s:allocLink failed.\n", __FUNCTION__);
    return -1;
  }
  if (-1 != *link) {
    iterator = *link;
    for (i = 0; i < *num; ++i) {
      linkItem = (subdivLink *)arrayGetItem(model->indexArray, iterator);
      iterator = linkItem->nextLink;
    }
    linkItem->nextLink = newLink;
  } else {
    *link = newLink;
  }
  (*num)++;
  return 0;
}

static subdivVertex *allocVertex(subdivModel *model) {
  subdivVertex *vertex;
  int newVertexIndex = arrayGetLength(model->vertexArray);
  if (0 != arraySetLength(model->vertexArray, newVertexIndex + 1)) {
    fprintf(stderr, "%s:arraySetLength failed.\n", __FUNCTION__);
    return 0;
  }
  vertex = arrayGetItem(model->vertexArray, newVertexIndex);
  initVertex(vertex);
  vertex->index = newVertexIndex;
  if (0 != pushLink(model, &model->vertexLink, &model->vertexNum,
      vertex->index)) {
    fprintf(stderr, "%s:pushLink failed.\n", __FUNCTION__);
    return -1;
  }
  return vertex;
}

static subdivEdge *allocEdge(subdivModel *model) {
  subdivEdge *edge;
  int newEdgeIndex = arrayGetLength(model->edgeArray);
  if (0 != arraySetLength(model->edgeArray, newEdgeIndex + 1)) {
    fprintf(stderr, "%s:arraySetLength failed.\n", __FUNCTION__);
    return 0;
  }
  edge = arrayGetItem(model->edgeArray, newEdgeIndex);
  initEdge(edge);
  edge->index = newEdgeIndex;
  return edge;
}

static subdivFace *allocFace(subdivModel *model) {
  subdivFace *face;
  int newFaceIndex = arrayGetLength(model->faceArray);
  if (0 != arraySetLength(model->faceArray, newFaceIndex + 1)) {
    fprintf(stderr, "%s:arraySetLength failed.\n", __FUNCTION__);
    return 0;
  }
  face = arrayGetItem(model->faceArray, newFaceIndex);
  initFace(face);
  face->index = newFaceIndex;
  return face;
}

static subdivVertex *getVertex(subdivModel *model, int index) {
  if (-1 == index) {
    return 0;
  }
  return (subdivVertex *)arrayGetItem(model->vertexArray, index);
}

static subdivEdge *getEdge(subdivModel *model, int index) {
  if (-1 == index) {
    return 0;
  }
  return (subdivEdge *)arrayGetItem(model->edgeArray, index);
}

static subdivFace *getFace(subdivModel *model, int index) {
  if (-1 == index) {
    return 0;
  }
  return (subdivFace *)arrayGetItem(model->faceArray, index);
}

static int isHoleEdge(subdivModel *model, int e) {
  return 1 == getEdge(model, e)->faceNum;
}

static int isHoleVertex(subdivModel *model, int p) {
  return getVertex(model, p)->faceNum != getVertex(model, p)->edgeNum;
}

static int facePoint(subdivModel *model, int f);

static int edgePoint(subdivModel *model, int e) {
  int nv = getEdge(model, e)->edgePt;
  if (-1 == nv) {
    subdivVertex *newVertex = allocVertex(model);
    if (!newVertex) {
      fprintf(stderr, "%s:allocVertex failed.\n", __FUNCTION__);
      return 0;
    }
    nv = newVertex->index;
    getEdge(model, e)->edgePt = nv;
    getEdge(model, e)->avg = getVertex(model, getEdge(model, e)->v[0])->v;
    vec3Add(&getEdge(model, e)->avg,
      &getVertex(model, getEdge(model, e)->v[1])->v, &getEdge(model, e)->avg);
    getVertex(model, nv)->v = getEdge(model, e)->avg;
    if (!isHoleEdge(model, e)) {
      int iterator;
      iterator = getEdge(model, e)->faceLink;
      while (-1 != iterator) {
        int f;
        int fp;
        subdivLink *linkItem = (subdivLink *)arrayGetItem(model->indexArray,
          iterator);
        f = linkItem->index;
        iterator = linkItem->nextLink;
        fp = facePoint(model, f);
        if (-1 == fp) {
          fprintf(stderr, "%s:facePoint failed.\n", __FUNCTION__);
          return 0;
        }
        vec3Add(&getVertex(model, nv)->v, &getVertex(model, fp)->v,
          &getVertex(model, nv)->v);
      }
      vec3Scale(&getVertex(model, nv)->v, 1.0 / 4, &getVertex(model, nv)->v);
    } else {
      vec3Scale(&getVertex(model, nv)->v, 1.0 / 2, &getVertex(model, nv)->v);
    }
    vec3Scale(&getEdge(model, e)->avg, 1.0 / 2, &getEdge(model, e)->avg);
  }
  return nv;
}

static int facePoint(subdivModel *model, int f) {
  int nv = getFace(model, f)->avg;
  if (-1 == nv) {
    int iterator;
    subdivVertex *newVertex = allocVertex(model);
    if (!newVertex) {
      fprintf(stderr, "%s:allocVertex failed.\n", __FUNCTION__);
      return 0;
    }
    nv = newVertex->index;
    getFace(model, f)->avg = nv;
    iterator = getFace(model, f)->vertexLink;
    while (-1 != iterator) {
      int p;
      subdivLink *linkItem = (subdivLink *)arrayGetItem(model->indexArray,
        iterator);
      p = linkItem->index;
      iterator = linkItem->nextLink;
      if (getFace(model, f)->vertexLink == iterator) {
        getVertex(model, nv)->v = getVertex(model, p)->v;
      } else {
        vec3Add(&getVertex(model, nv)->v, &getVertex(model, p)->v,
          &getVertex(model, nv)->v);
      }
    }
    vec3Scale(&getVertex(model, nv)->v, 1.0 / (getFace(model, f)->vertexNum),
      &getVertex(model, nv)->v);
  }
  return nv;
}

static void scaleAdd(vec3 *a, vec3 *b, float scale, vec3 *result) {
  vec3 tmp;
  vec3Scale(b, scale, &tmp);
  vec3Add(a, &tmp, result);
}

static int updatedPoint(subdivModel *model, int p) {
  int n = 0;
  int iterator;
  subdivVertex *newVertex;
  int facePt;
  int e;
  subdivFace *f;
  subdivLink *link;
  int nv;
  int ep;
  vec3 sum = {0, 0, 0};

  nv = getVertex(model, p)->newVertexIndex;
  if (-1 != nv) {
    return nv;
  }

  newVertex = allocVertex(model);
  if (!newVertex) {
    fprintf(stderr, "%s:allocVertex failed.\n", __FUNCTION__);
    return 0;
  }
  nv = newVertex->index;
  getVertex(model, p)->newVertexIndex = nv;
  if (isHoleVertex(model, p)) {
    getVertex(model, nv)->v = getVertex(model, p)->v;
    iterator = getVertex(model, p)->edgeLink;
    while (-1 != iterator) {
      link = (subdivLink *)arrayGetItem(model->indexArray, iterator);
      e = link->index;
      iterator = link->nextLink;
      if (!isHoleEdge(model, e)) {
        continue;
      }
      ep = edgePoint(model, e);
      if (-1 == ep) {
        fprintf(stderr, "%s:edgePoint failed.\n", __FUNCTION__);
        return 0;
      }
      vec3Add(&getVertex(model, nv)->v, &getVertex(model, ep)->v,
        &getVertex(model, nv)->v);
      n++;
    }
    vec3Scale(&getVertex(model, nv)->v, 1.0 / (n + 1),
      &getVertex(model, nv)->v);
  } else {
    n = getVertex(model, p)->faceNum;
    iterator = getVertex(model, p)->faceLink;
    while (-1 != iterator) {
      link = (subdivLink *)arrayGetItem(model->indexArray, iterator);
      f = link->index;
      iterator = link->nextLink;
      facePt = facePoint(model, f);
      if (-1 == facePt) {
        fprintf(stderr, "%s:facePoint failed.\n", __FUNCTION__);
        return 0;
      }
      vec3Add(&sum, &getVertex(model, facePt)->v, &sum);
    }
    iterator = getVertex(model, p)->edgeLink;
    while (-1 != iterator) {
      link = (subdivLink *)arrayGetItem(model->indexArray, iterator);
      e = link->index;
      iterator = link->nextLink;
      ep = edgePoint(model, e);
      if (-1 == ep) {
        fprintf(stderr, "%s:edgePoint failed.\n", __FUNCTION__);
        return 0;
      }
      scaleAdd(&sum, &getVertex(model, ep)->v, 2, &sum);
    }
    vec3Scale(&sum, 1.0 / n, &sum);
    scaleAdd(&sum, &getVertex(model, p)->v, n - 3, &sum);
    vec3Scale(&sum, 1.0 / n, &sum);
    getVertex(model, nv)->v = sum;
  }

  return nv;
}

int subdivCalculteNorms(subdivModel *model) {
  int i, j, n;
  int faceIterator;
  int nextFaceIterator;
  int vertexIterator;
  int nextVertexIterator;
  subdivFace *f;
  subdivLink *linkItem;
  subdivVertex *v;
  subdivVertex *v0;
  subdivVertex *v1;
  vec3 d1, d2, norm;

  faceIterator = model->faceLink;
  j = 0;
  while (-1 != faceIterator) {
    linkItem = (subdivLink *)arrayGetItem(model->indexArray, faceIterator);
    f = getFace(model, linkItem->index);
    nextFaceIterator = linkItem->nextLink;
    vertexIterator = f->vertexLink;
    n = f->vertexNum;
    i = 0;
    while (-1 != vertexIterator) {
      linkItem = (subdivLink *)arrayGetItem(model->indexArray, faceIterator);
      f = getFace(model, linkItem->index);
      linkItem = (subdivLink *)arrayGetItem(model->indexArray, vertexIterator);
      v = getVertex(model, linkItem->index);
      vertexIterator = linkItem->nextLink;
      v0 = getVertex(model,
        subdivLinkElement(model, f->vertexLink, i ? i - 1 : n - 1));
      v1 = getVertex(model,
        subdivLinkElement(model, f->vertexLink, (i + 1) % n));
      vec3Sub(&v->v, &v0->v, &d1);
      vec3Sub(&v1->v, &v->v, &d2);
      vec3CrossProduct(&d1, &d2, &norm);
      vec3Add(&f->norm, &norm, &f->norm);
      ++i;
    }
    if (i > 1) {
      vec3Normalize(&f->norm);
    }
    faceIterator = nextFaceIterator;
    ++j;
  }

  vertexIterator = model->vertexLink;
  while (-1 != vertexIterator) {
    linkItem = (subdivLink *)arrayGetItem(model->indexArray, vertexIterator);
    v = getVertex(model, linkItem->index);
    nextVertexIterator = linkItem->nextLink;
    faceIterator = v->faceLink;
    j = 0;
    while (-1 != faceIterator) {
      linkItem = (subdivLink *)arrayGetItem(model->indexArray, faceIterator);
      f = getFace(model, linkItem->index);
      faceIterator = linkItem->nextLink;
      vec3Add(&v->avgNorm, &f->norm, &v->avgNorm);
      ++j;
    }
    if (j > 1) {
      vec3Normalize(&v->avgNorm);
    }
    vertexIterator = nextVertexIterator;
  }

  return 0;
}

void subdivDrawModel(subdivModel *model) {
  subdivLink *linkItem;
  subdivFace *f;
  subdivVertex *v;
  int faceIterator;
  int vertexIterator;
  faceIterator = model->faceLink;
  glColor4f(1.0, 1.0, 1.0, 1.0);
  while (-1 != faceIterator) {
    linkItem = (subdivLink *)arrayGetItem(model->indexArray, faceIterator);
    f = getFace(model, linkItem->index);
    faceIterator = linkItem->nextLink;
    vertexIterator = f->vertexLink;
    glBegin(GL_POLYGON);
    while (-1 != vertexIterator) {
      linkItem = (subdivLink *)arrayGetItem(model->indexArray, vertexIterator);
      v = getVertex(model, linkItem->index);
      vertexIterator = linkItem->nextLink;
      glNormal3fv(&(f->norm.x));
			glVertex3fv(&(v->v.x));
    }
    glEnd();
  }
}

subdivModel *subdivCatmullClark(subdivModel *model) {
  int j, ai, bi, ci, di;
  int a, b, c, d;
  int faceIterator;
  int nextFaceIterator;
  int vertexIterator;
  int f;
  subdivLink *linkItem;
  int p;
  subdivModel *outputModel;
  
  outputModel = subdivCreateModel();
  if (!outputModel) {
    fprintf(stderr, "%s:subdivCreateModel failed.\n", __FUNCTION__);
    return 0;
  }

  faceIterator = model->faceLink;
  while (-1 != faceIterator) {
    linkItem = (subdivLink *)arrayGetItem(model->indexArray, faceIterator);
    f = linkItem->index;
    j = 0;
    nextFaceIterator = linkItem->nextLink;
    vertexIterator = getFace(model, f)->vertexLink;
    while (-1 != vertexIterator) {
      linkItem = (subdivLink *)arrayGetItem(model->indexArray, vertexIterator);
      p = linkItem->index;
      vertexIterator = linkItem->nextLink;
      ai = updatedPoint(model, p);
      if (-1 == ai) {
        fprintf(stderr, "%s:updatedPoint failed.\n", __FUNCTION__);
        subdivDestroyModel(outputModel);
        return 0;
      }
      bi = edgePoint(model, subdivLinkElement(model,
        getFace(model, f)->edgeLink, (j + 1) % getFace(model, f)->edgeNum));
      if (-1 == bi) {
        fprintf(stderr, "%s:edgePoint failed.\n", __FUNCTION__);
        subdivDestroyModel(outputModel);
        return 0;
      }
      ci = facePoint(model, f);
      if (-1 == ci) {
        fprintf(stderr, "%s:facePoint failed.\n", __FUNCTION__);
        subdivDestroyModel(outputModel);
        return 0;
      }
      di = edgePoint(model, subdivLinkElement(model,
        getFace(model, f)->edgeLink, j));
      if (-1 == di) {
        fprintf(stderr, "%s:edgePoint failed.\n", __FUNCTION__);
        subdivDestroyModel(outputModel);
        return 0;
      }
      a = getVertex(model, ai)->indexOnNewModel;
      if (-1 == a) {
        a = subdivAddVertex(outputModel, &getVertex(model, ai)->v);
        getVertex(model, ai)->indexOnNewModel = a;
      }
      b = getVertex(model, bi)->indexOnNewModel;
      if (-1 == b) {
        b = subdivAddVertex(outputModel, &getVertex(model, bi)->v);
        getVertex(model, bi)->indexOnNewModel = b;
      }
      c = getVertex(model, ci)->indexOnNewModel;
      if (-1 == c) {
        c = subdivAddVertex(outputModel, &getVertex(model, ci)->v);
        getVertex(model, ci)->indexOnNewModel = c;
      }
      d = getVertex(model, di)->indexOnNewModel;
      if (-1 == d) {
        d = subdivAddVertex(outputModel, &getVertex(model, di)->v);
        getVertex(model, di)->indexOnNewModel = d;
      }
      if (-1 == a || -1 == b || -1 == c || -1 == d) {
        fprintf(stderr, "%s:subdivAddVertex failed.\n", __FUNCTION__);
        subdivDestroyModel(outputModel);
        return 0;
      }
      if (-1 == subdivAddQuadFace(outputModel, a, b, c, d)) {
        fprintf(stderr, "%s:subdivAddFace failed.\n", __FUNCTION__);
        subdivDestroyModel(outputModel);
        return 0;
      }
      ++j;
    }
    faceIterator = nextFaceIterator;
  }
  
  if (0 != subdivCalculteNorms(outputModel)) {
    fprintf(stderr, "%s:subdivCalculteNorms failed.\n", __FUNCTION__);
    subdivDestroyModel(outputModel);
    return 0;
  }

  return outputModel;
}

int subdivAddVertex(subdivModel *model, vec3 *v) {
  subdivVertex *newVertex = allocVertex(model);
  if (!newVertex) {
    fprintf(stderr, "%s:allocVertex failed.\n", __FUNCTION__);
    return -1;
  }
  newVertex->v = *v;
  return newVertex->index;
}

static int subdivAddEdge(subdivModel *model, int p1, int p2) {
  subdivEdge *newEdge = allocEdge(model);
  subdivVertex *v1;
  subdivVertex *v2;
  if (!newEdge) {
    fprintf(stderr, "%s:allocEdge failed.\n", __FUNCTION__);
    return -1;
  }
  newEdge->v[0] = p1;
  newEdge->v[1] = p2;
  v1 = getVertex(model, p1);
  v2 = getVertex(model, p2);
  if (-1 == pushLink(model, &v1->edgeLink, &v1->edgeNum, newEdge->index)) {
    fprintf(stderr, "%s:pushLink failed.\n", __FUNCTION__);
    return -1;
  }
  if (-1 == pushLink(model, &v2->edgeLink, &v2->edgeNum, newEdge->index)) {
    fprintf(stderr, "%s:pushLink failed.\n", __FUNCTION__);
    return -1;
  }
  if (-1 == pushLink(model, &model->edgeLink, &model->edgeNum, newEdge->index)) {
    fprintf(stderr, "%s:pushLink failed.\n", __FUNCTION__);
    return -1;
  }
  return newEdge->index;
}

static int subdivEdgeByVertexs(subdivModel *model, int p1, int p2) {
  subdivLink *linkItem;
  subdivEdge *e;
  subdivVertex *v1 = getVertex(model, p1);
  int newEdgeIndex;
  int iterator = v1->edgeLink;
  while (-1 != iterator) {
    linkItem = (subdivLink *)arrayGetItem(model->indexArray, iterator);
    e = getEdge(model, linkItem->index);
    iterator = linkItem->nextLink;
    if (e->v[0] == p2 || e->v[1] == p2) {
      return e->index;
    }
  }
  newEdgeIndex = subdivAddEdge(model, p1, p2);
  return newEdgeIndex;
}

static int subdivAddFace(subdivModel *model, int *vertexs, int vertexNum) {
  int i;
  subdivFace *f = allocFace(model);
  int p0, p1;
  int newFaceIndex;
  int edgeIndex;
  if (!f) {
    fprintf(stderr, "%s:allocFace failed.\n", __FUNCTION__);
    return -1;
  }
  newFaceIndex = f->index;
  if (0 != pushLink(model, &model->faceLink, &model->faceNum, newFaceIndex)) {
    fprintf(stderr, "%s:pushLink failed.\n", __FUNCTION__);
    return -1;
  }
  p0 = vertexs[0];
  for (i = 1; i <= vertexNum; ++i, p0 = p1) {
    subdivVertex *v1;
    subdivEdge *e;
    p1 = vertexs[i % vertexNum];
    v1 = getVertex(model, p1);
    if (0 != pushLink(model, &v1->faceLink, &v1->faceNum, newFaceIndex)) {
      fprintf(stderr, "%s:pushLink failed.\n", __FUNCTION__);
      return -1;
    }
    edgeIndex = subdivEdgeByVertexs(model, p0, p1);
    if (-1 == edgeIndex) {
      fprintf(stderr, "%s:subdivEdgeByVertexs failed.\n", __FUNCTION__);
      return -1;
    }
    e = getEdge(model, edgeIndex);
    if (0 != pushLink(model, &e->faceLink, &e->faceNum, newFaceIndex)) {
      fprintf(stderr, "%s:pushLink failed.\n", __FUNCTION__);
      return -1;
    }
    f = getFace(model, newFaceIndex);
    if (0 != pushLink(model, &f->edgeLink, &f->edgeNum, edgeIndex)) {
      fprintf(stderr, "%s:pushLink failed.\n", __FUNCTION__);
      return -1;
    }
    if (0 != pushLink(model, &f->vertexLink, &f->vertexNum, p1)) {
      fprintf(stderr, "%s:pushLink failed.\n", __FUNCTION__);
      return -1;
    }
  }
  return newFaceIndex;
}

int subdivAddTriangleFace(subdivModel *model, int p1, int p2, int p3) {
  int vertexs[3] = {p1, p2, p3};
  return subdivAddFace(model, vertexs, 3);
}

int subdivAddQuadFace(subdivModel *model, int p1, int p2, int p3, int p4) {
  int vertexs[4] = {p1, p2, p3, p4};
  return subdivAddFace(model, vertexs, 4);
}

int subdivAddCube(subdivModel *model) {
  int x, y, z;
  for (x = -1; x <= 1; x += 2) {
    for (y = -1; y <= 1; y += 2) {
      for (z = -1; z <= 1; z += 2) {
        vec3 v = {x, y, z};
        subdivAddVertex(model, &v);
      }
    }
  }
  subdivAddQuadFace(model, 0, 1, 3, 2);
  subdivAddQuadFace(model, 6, 7, 5, 4);
	subdivAddQuadFace(model, 4, 5, 1, 0);
	subdivAddQuadFace(model, 2, 3, 7, 6);
	subdivAddQuadFace(model, 0, 2, 6, 4);
	subdivAddQuadFace(model, 5, 7, 3, 1);
  subdivCalculteNorms(model);
  return 0;
}

subdivModel *subdivCatmullClarkWithLoops(subdivModel *model, int loops) {
  int i;
  subdivModel *outputModel;
  outputModel = subdivCatmullClark(model);
  if (!outputModel) {
    fprintf(stderr, "%s:subdivCatmullClark failed.\n", __FUNCTION__);
    return 0;
  }
  for (i = 1; i < loops; ++i) {
    subdivModel *loopInput = outputModel;
    outputModel = subdivCatmullClark(loopInput);
    subdivDestroyModel(loopInput);
    if (!outputModel) {
      fprintf(stderr, "%s:subdivCatmullClark failed(loops:%d i:%d).\n",
        __FUNCTION__, loops, i);
      return 0;
    }
  }
  drawDebugPrintf("faces: %d", outputModel->faceNum);
  subdivDrawModel(outputModel);
  return outputModel;
}
