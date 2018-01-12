#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
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

void *arrayNewItemAt(array *arr, int index) {
    int length = arrayGetLength(arr);
    if (-1 == index) {
        index = length;
    }
    if (index < 0 || index > length) {
        return 0;
    }
    if (index == length) {
        return arrayNewItem(arr);
    }
    (void)arrayNewItem(arr);
    memmove(arrayGetItem(arr, index + 1), arrayGetItem(arr, index), (length - index) * arrayGetNodeSize(arr));
    return arrayGetItem(arr, index);
}

void *arrayNewItemClearAt(array *arr, int index) {
    void *ptr = arrayNewItemAt(arr, index);
    memset(ptr, 0, arr->nodeSize);
    return ptr;
}

void arrayRemoveItem(array *arr, int index) {
    int length = arrayGetLength(arr);
    assert(index < length);
    memmove(arrayGetItem(arr, index), arrayGetItem(arr, index + 1), (length - index - 1) * arrayGetNodeSize(arr));
    arraySetLength(arr, arrayGetLength(arr) - 1);
}


