#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "dmemory.h"
#include "dict.h"

typedef struct hashtableEntry {
  const void *node;
  int nextEntryIndex;
} hashtableEntry;

typedef struct hashtableKey {
  int entryNum;
  int firstEntryIndex;
} hashtableKey;

typedef struct hashtable {
  array *keyArray;
  array *entryArray;
  int bucketSize;
  int (*hashCallback)(void *userData, const void *node);
  int (*compareCallback)(void *userData, const void *node1, const void *node2);
  void *userData;
} hashtable;

void hashtableDestroy(hashtable *ht) {
  if (ht) {
    arrayDestroy(ht->keyArray);
    arrayDestroy(ht->entryArray);
    dfree(ht);
  }
}

hashtable *hashtableCreate(int bucketSize,
    int (*hashCallback)(void *userData, const void *node),
    int (*compareCallback)(void *userData, const void *node1,
      const void *node2),
    void *userData) {
  hashtable *ht = (hashtable *)calloc(1, sizeof(hashtable));
  if (!ht) {
    fprintf(stderr, "%s:Insufficient memory.\n", __FUNCTION__);
    return 0;
  }
  ht->keyArray = arrayCreate(sizeof(hashtableKey));
  if (!ht->keyArray) {
    fprintf(stderr, "%s:arrayCreate failed.\n", __FUNCTION__);
    hashtableDestroy(ht);
    return 0;
  }
  ht->entryArray = arrayCreate(sizeof(hashtableEntry));
  if (!ht->entryArray) {
    fprintf(stderr, "%s:arrayCreate failed.\n", __FUNCTION__);
    hashtableDestroy(ht);
    return 0;
  }
  ht->bucketSize = bucketSize;
  ht->hashCallback = hashCallback;
  ht->compareCallback = compareCallback;
  ht->userData = userData;
  arraySetLength(ht->keyArray, bucketSize);
  return ht;
}

static int hashtableGetNodeHash(hashtable *ht, const void *node) {
  return (int)((unsigned int)ht->hashCallback(ht->userData,
    node) % ht->bucketSize);
}

static hashtableEntry *findEntry(hashtable *ht, hashtableKey *key,
    const void *node) {
  int i;
  int nextEntryIndex = key->firstEntryIndex;
  for (i = 0; i < key->entryNum; ++i) {
    hashtableEntry *entry;
    assert(-1 != nextEntryIndex);
    entry = (hashtableEntry *)arrayGetItem(ht->entryArray, nextEntryIndex);
    if (0 == ht->compareCallback(ht->userData, entry->node, node)) {
      return (void *)entry->node;
    }
    nextEntryIndex = entry->nextEntryIndex;
  }
  return 0;
}

static void hashtableInsert(hashtable *ht, const void *keyNode,
    const void *node) {
  int newEntryIndex;
  hashtableEntry *entry;
  int hash = hashtableGetNodeHash(ht, keyNode);
  hashtableKey *key = arrayGetItem(ht->keyArray, hash);
  newEntryIndex = arrayGetLength(ht->entryArray);
  arraySetLength(ht->entryArray, newEntryIndex + 1);
  entry = arrayGetItem(ht->entryArray, newEntryIndex);
  entry->node = node;
  entry->nextEntryIndex = 0 == key->entryNum ? -1 : key->firstEntryIndex;
  key->firstEntryIndex = newEntryIndex;
  key->entryNum++;
}

void *hashtableGet(hashtable *ht, const void *node) {
  int hash = hashtableGetNodeHash(ht, node);
  hashtableKey *key = (hashtableKey *)arrayGetItem(ht->keyArray, hash);
  return findEntry(ht, key, node);
}

struct dict {
  hashtable *ht;
  array *arr;
  int (*hashCallback)(void *userData, const void *key);
  int (*compareCallback)(void *userData, const void *key1, const void *key2);
  void *userData;
  void *findKey;
};

static const void *dictKeyTranslate(dict *dct, const void *key) {
  if (0 == key) {
    key = dct->findKey;
  } else {
    key = arrayGetItem(dct->arr, ((char *)key - 0) - 1);
  }
  return key;
}

static int dictHash(void *userData, const void *key) {
  dict *dct = userData;
  return dct->hashCallback(dct->userData, dictKeyTranslate(dct, key));
}

static int dictCompare(void *userData, const void *key1, const void *key2) {
  dict *dct = userData;
  return dct->compareCallback(dct->userData, dictKeyTranslate(dct, key1),
    dictKeyTranslate(dct, key2));
}

dict *dictCreate(array *arr, int bucketSize,
    int (*hashCallback)(void *userData, const void *key),
    int (*compareCallback)(void *userData, const void *key1, const void *key2),
    void *userData) {
  dict *dct = dcalloc(1, sizeof(dict));
  dct->arr = arr;
  dct->hashCallback = hashCallback;
  dct->compareCallback = compareCallback;
  dct->userData = userData;
  dct->findKey = 0;
  dct->ht = hashtableCreate(bucketSize, dictHash, dictCompare, dct);
  return dct;
}

void dictDestroy(dict *dct) {
  if (dct && dct->ht) {
    hashtableDestroy(dct->ht);
  }
  dfree(dct);
}

void *dictFind(dict *dct, void *key) {
  int index;
  dct->findKey = key;
  index = (char *)hashtableGet(dct->ht, 0) - 0;
  if (0 == index) {
    return 0;
  }
  return arrayGetItem(dct->arr, index - 1);
}

void *dictGet(dict *dct, void *key) {
  void *value = dictFind(dct, key);
  if (!value) {
    int newIndex = arrayGetLength(dct->arr);
    value = arrayNewItem(dct->arr);
    dct->findKey = key;
    hashtableInsert(dct->ht, 0, (char *)0 + newIndex + 1);
  }
  return value;
}

void *dictGetClear(dict *dct, void *key) {
  void *ptr = dictGet(dct, key);
  memset(ptr, 0, arrayGetNodeSize(dct->arr));
  return ptr;
}
