#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "hashtable.h"
#include "array.h"

typedef struct hashtableEntry {
  const void *node;
  int nextEntryIndex;
} hashtableEntry;

typedef struct hashtableKey {
  int entryNum;
  int firstEntryIndex;
} hashtableKey;

struct hashtable {
  array *keyArray;
  array *entryArray;
  int bucketSize;
  int (*hashCallback)(void *userData, const void *node);
  int (*compareCallback)(void *userData, const void *node1, const void *node2);
  void *userData;
};

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
  if (0 != arraySetLength(ht->keyArray, bucketSize)) {
    fprintf(stderr, "%s:arraySetLength failed(bucketSize:%d).\n", __FUNCTION__,
      bucketSize);
    hashtableDestroy(ht);
    return 0;
  }
  return ht;
}

void hashtableDestroy(hashtable *ht) {
  if (ht) {
    arrayDestroy(ht->keyArray);
    arrayDestroy(ht->entryArray);
    free(ht);
  }
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

int hashtableInsert(hashtable *ht, const void *node) {
  int newEntryIndex;
  int hash = hashtableGetNodeHash(ht, node);
  hashtableKey *key = (hashtableKey *)arrayGetItem(ht->keyArray, hash);
  hashtableEntry *entry = findEntry(ht, key, node);
  if (entry) {
    return -1;
  }
  newEntryIndex = arrayGetLength(ht->entryArray);
  if (0 != arraySetLength(ht->entryArray, newEntryIndex + 1)) {
    fprintf(stderr, "%s:arraySetLength failed(newEntryIndex:%d).\n",
      __FUNCTION__, newEntryIndex);
    return -1;
  }
  entry = (hashtableEntry *)arrayGetItem(ht->entryArray, newEntryIndex);
  entry->node = node;
  entry->nextEntryIndex = 0 == key->entryNum ? -1 : key->firstEntryIndex;
  key->firstEntryIndex = newEntryIndex;
  key->entryNum++;
  return 0;
}

void *hashtableGet(hashtable *ht, const void *node) {
  int hash = hashtableGetNodeHash(ht, node);
  hashtableKey *key = (hashtableKey *)arrayGetItem(ht->keyArray, hash);
  return findEntry(ht, key, node);
}
