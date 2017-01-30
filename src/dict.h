#ifndef DICT_H
#define DICT_H
#include "array.h"

typedef struct dict dict;
dict *dictCreate(array *arr, int bucketSize,
  int (*hashCallback)(void *userData, const void *key),
  int (*compareCallback)(void *userData, const void *key1, const void *key2),
  void *userData);
void dictDestroy(dict *dct);
void *dictFind(dict *dct, void *key);
void *dictGet(dict *dct, void *key);
void *dictGetClear(dict *dct, void *key);

#endif

