#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "array.h"

struct array {
  int nodeSize;
  int capacity;
  int length;
  char *nodes;
};

array *arrayCreate(int nodeSize) {
  array *arr = (array *)calloc(1, sizeof(array));
  if (!arr) {
    fprintf(stderr, "%s:Insufficient memory.\n", __FUNCTION__);
    return 0;
  }
  arr->nodeSize = nodeSize;
  return arr;
}

int arraySetLength(array *arr, int length) {
  if (length > arr->capacity) {
    int newCapacity = (arr->capacity + 1) * 2;
    char *newNodes = (char *)realloc(arr->nodes, arr->nodeSize * newCapacity);
    if (!newNodes) {
      fprintf(stderr, "%s:Insufficient memory.\n", __FUNCTION__);
      return -1;
    }
    arr->capacity = newCapacity;
    arr->nodes = newNodes;
  }
  arr->length = length;
  return 0;
}

void *arrayGetItem(array *arr, int index) {
  if (index >= arr->length) {
    return 0;
  }
  return arr->nodes + arr->nodeSize * index;
}

int arrayGetLength(array *arr) {
  return arr->length;
}

void arrayDestroy(array *arr) {
  if (arr) {
    free(arr->nodes);
    free(arr);
  }
}
