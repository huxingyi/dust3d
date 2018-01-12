#ifndef ARRAY_H
#define ARRAY_H

typedef struct array array;

array *arrayCreate(int nodeSize);
int arrayGetNodeSize(array *arr);
void *arrayGetItem(array *arr, int index);
int arrayGetLength(array *arr);
void arraySetLength(array *arr, int length);
void *arrayNewItem(array *arr);
void *arrayNewItemClear(array *arr);
void *arrayNewItemAt(array *arr, int index);
void *arrayNewItemClearAt(array *arr, int index);
void arrayRemoveItem(array *arr, int index);
void arrayDestroy(array *arr);

#endif

