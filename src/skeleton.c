#include "skeleton.h"
#include "dmemory.h"
#include "osutil.h"

struct skeleton {
  skeletonNode *root;
};

skeleton *skeletonCreate(void) {
  return dcalloc(1, sizeof(skeleton));
}

void skeletonDestroy(skeleton *skl) {
  dfree(skl);
}

skeletonNode *skeletonNewNode(skeleton *skl) {
  unsued(skl);
  return dcalloc(1, sizeof(skeletonNode));
}

void skeletonRemoveNode(skeleton *skl, skeletonNode *node) {
  skeletonNode *parent = node->parent;
  skeletonNode *pre = 0;
  skeletonNode *loop;
  skeletonNode *next;
  if (parent) {
    for (loop = parent->firstChild; loop; pre = loop, loop = loop->nextSib) {
      if (loop == node) {
        break;
      }
    }
    if (pre) {
      pre->nextSib = node->nextSib;
    } else {
      parent->firstChild = 0;
    }
  }
  for (loop = node->firstChild; loop; loop = next) {
    next = loop->nextSib;
    skeletonRemoveNode(skl, loop);
  }
  if (skl->root == node) {
    skl->root = 0;
  }
  dfree(node);
}

void skeletonConnectNodes(skeleton *skl, skeletonNode *first,
    skeletonNode *second) {
  if (first->firstChild) {
    skeletonNode *loop;
    for (loop = first->firstChild; loop->nextSib; loop = loop->nextSib) {
      // void
    }
    loop->nextSib = second;
  } else {
    first->firstChild = second;
  }
  second->parent = first;
  if (!skl->root) {
    skl->root = first;
  }
}

skeletonNode *skeletonGetRoot(skeleton *skl) {
  return skl->root;
}
