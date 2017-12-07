/* Copyright (c) huxingyi@msn.com 2017 All rights reserved.
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#ifndef RBTREE_H
#define RBTREE_H

typedef struct rbtreeNode {
    unsigned char color;
    struct rbtreeNode *parent;
    struct rbtreeNode *left;
    struct rbtreeNode *right;
} rbtreeNode;

typedef struct rbtree rbtree;

typedef int (*rbtreeComparator)(rbtree *tree, const void *firstKey, const void *secondKey);

struct rbtree {
    rbtreeNode *root;
    int keyOffsetOfNode;
    int nodeOffsetOfParent;
    rbtreeComparator comparator;
};

#ifndef offsetof
#define offsetof(TYPE, MEMBER) ((int)&((TYPE *)0)->MEMBER)
#endif

/*Preprovided Common Comparators*/
int rbtreeIntComparator(rbtree *tree, const void *firstKey, const void *secondKey);
int rbtreeUintComparator(rbtree *tree, const void *firstKey, const void *secondKey);
int rbtreeLongComparator(rbtree *tree, const void *firstKey, const void *secondKey);
int rbtreeUlongComparator(rbtree *tree, const void *firstKey, const void *secondKey);
int rbtreeLongLongComparator(rbtree *tree, const void *firstKey, const void *secondKey);
int rbtreeUlongLongComparator(rbtree *tree, const void *firstKey, const void *secondKey);
int rbtreeStringComparator(rbtree *tree, const void *firstKey, const void *secondKey);

/*API*/
#define rbtreeInit(tree_, parentType_, nodeFieldName_, keyFieldName_, comparator_) do {  \
    (tree_)->root = 0;                                                                  \
    (tree_)->nodeOffsetOfParent = (int)offsetof(parentType_, nodeFieldName_);            \
    (tree_)->keyOffsetOfNode = (int)offsetof(parentType_, keyFieldName_) -              \
        (tree_)->nodeOffsetOfParent;                                                    \
    (tree_)->comparator = comparator_ ? comparator_ : rbtreeStringComparator;           \
} while (0)
void *rbtreeFind(rbtree *tree, const void *key);
void *rbtreeInsert(rbtree *tree, void *current);
void rbtreeDelete(rbtree *tree, void *current);
void *rbtreeFirst(rbtree *tree);
void *rbtreeLast(rbtree *tree);
void *rbtreeNext(rbtree *tree, void *current);
void *rbtreePrevious(rbtree *tree, void *current);

/*Internal API*/
void rbtreeDeleteNode(rbtree *tree, rbtreeNode *node);
rbtreeNode *rbtreeInsertNode(rbtree *tree, rbtreeNode *node);
rbtreeNode *rbtreeFindNode(rbtree *tree, const void *key);
rbtreeNode *rbtreeSearch(rbtree *tree, const void *key, rbtreeNode **lastNode, int *lastCmp);

/*Debug Utilities*/
#if RBTREE_DEBUG
typedef int (*rbtreeKeyToIntConverter)(const void *key);
void rbtreePrint(rbtree *tree, rbtreeKeyToIntConverter converter);
int rbtreeDumpToArray(rbtree *tree, rbtreeKeyToIntConverter converter, int *array, int maxNum);
int rbtreeVerify(rbtree *tree);
#endif

#endif
