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
void arrayDestroy(array *arr);

#endif

