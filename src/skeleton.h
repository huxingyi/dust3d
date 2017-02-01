#ifndef SKELETON_H
#define SKELETON_H
#include "3dstruct.h"

typedef struct skeleton skeleton;
typedef struct skeletonNode {
  vec3 position;
  float radius;
  struct skeletonNode *parent;
  struct skeletonNode *firstChild;
  struct skeletonNode *nextSib;
} skeletonNode;

skeleton *skeletonCreate(void);
void skeletonDestroy(skeleton *skl);
skeletonNode *skeletonNewNode(skeleton *skl);
void skeletonRemoveNode(skeleton *skl, skeletonNode *node);
void skeletonConnectNodes(skeleton *skl, skeletonNode *first,
  skeletonNode *second);
skeletonNode *skeletonGetRoot(skeleton *skl);

#endif
