#ifndef HASHTABLE_H
#define HASHTABLE_H

typedef struct hashtable hashtable;

hashtable *hashtableCreate(int bucketSize,
  int (*hashCallback)(void *userData, const void *node),
  int (*compareCallback)(void *userData, const void *node1, const void *node2),
  void *userData);
void hashtableDestroy(hashtable *ht);
int hashtableInsert(hashtable *ht, const void *node);
void *hashtableGet(hashtable *ht, const void *node);

#endif

