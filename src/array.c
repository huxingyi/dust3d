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
  array *arr = calloc(1, sizeof(array));
  arr->nodeSize = nodeSize;
  return arr;
}

int arrayGetNodeSize(array *arr) {
  return arr->nodeSize;
}

void arraySetLength(array *arr, int length) {
  char *newNodes;
  if (length > arr->capacity) {
    int newCapacity = (arr->capacity + 1) * 2;
    if (newCapacity < length) {
      newCapacity = length;
    }
    newNodes = realloc(arr->nodes, arr->nodeSize * newCapacity);
    memset(newNodes + arr->nodeSize * arr->capacity, 0,
      arr->nodeSize * (newCapacity - arr->capacity));
    arr->capacity = newCapacity;
    arr->nodes = newNodes;
  }
  arr->length = length;
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
  arraySetLength(arr, newIndex + 1);
  return arrayGetItem(arr, newIndex);
}

void *arrayNewItemClear(array *arr) {
  void *ptr = arrayNewItem(arr);
  memset(ptr, 0, arr->nodeSize);
  return ptr;
}

