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
  char *newNodes;
  if (length > arr->capacity) {
    int newCapacity = (arr->capacity + 1) * 2;
    if (newCapacity < length) {
      newCapacity = length;
    }
    newNodes = (char *)realloc(arr->nodes, arr->nodeSize * newCapacity);
    if (!newNodes) {
      fprintf(stderr, "%s:Insufficient memory.\n", __FUNCTION__);
      return -1;
    }
    memset(newNodes + arr->nodeSize * arr->capacity, 0,
      arr->nodeSize * (newCapacity - arr->capacity));
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

void *arrayNewItem(array *arr) {
  int newIndex = arrayGetLength(arr);
  if (0 != arraySetLength(arr, newIndex + 1)) {
    fprintf(stderr, "%s:arraySetLength.\n", __FUNCTION__);
    return 0;
  }
  return arrayGetItem(arr, newIndex);
}
