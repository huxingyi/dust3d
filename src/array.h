#ifndef ARRAY_H
#define ARRAY_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct array array;

array *arrayCreate(int nodeSize);
void *arrayGetItem(array *arr, int index);
int arrayGetLength(array *arr);
int arraySetLength(array *arr, int length);
void arrayDestroy(array *arr);

#ifdef __cplusplus
}
#endif

#endif
