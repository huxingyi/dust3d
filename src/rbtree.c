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

#include <string.h>
#include <stdio.h>
#include "rbtree.h"

#define keyOfNode(tree, node) (const void *)((char *)(node) + (tree)->keyOffsetOfNode)
#define integerComparator(type, firstKey, secondKey)    (*(type *)(firstKey) == *(type *)(secondKey)) ? 0 : (*(type *)(firstKey) < *(type *)(secondKey) ? -1 : 1)

int rbtreeIntComparator(rbtree *tree, const void *firstKey, const void *secondKey) {
    return integerComparator(int, firstKey, secondKey);
}

int rbtreeUintComparator(rbtree *tree, const void *firstKey, const void *secondKey) {
    return integerComparator(int, firstKey, secondKey);
}

int rbtreeLongComparator(rbtree *tree, const void *firstKey, const void *secondKey) {
    return integerComparator(int, firstKey, secondKey);
}

int rbtreeUlongComparator(rbtree *tree, const void *firstKey, const void *secondKey) {
    return integerComparator(int, firstKey, secondKey);
}

int rbtreeLongLongComparator(rbtree *tree, const void *firstKey, const void *secondKey) {
    return integerComparator(int, firstKey, secondKey);
}

int rbtreeUlongLongComparator(rbtree *tree, const void *firstKey, const void *secondKey) {
    return integerComparator(int, firstKey, secondKey);
}

int rbtreeStringComparator(rbtree *tree, const void *firstKey, const void *secondKey) {
    return strcmp((const char *)firstKey, (const char *)secondKey);
}

rbtreeNode *rbtreeSearch(rbtree *tree, const void *key, rbtreeNode **lastNode, int *lastCmp) {
    rbtreeNode *node;
    node = tree->root;
    while (node) {
        *lastNode = node;
        *lastCmp = tree->comparator(tree, key, keyOfNode(tree, node));
        if (0 == *lastCmp) {
            break;
        } else if (*lastCmp < 0) {
            node = node->left;
        } else {
            node = node->right;
        }
    }
    return node;
}

#define RED     1
#define BLACK   0

#define isRed(node)     ((node) && RED == (node)->color)
#define isBlack(node)   (!isRed(node))
#define markBlack(node) (node)->color = BLACK
#define markRed(node)   (node)->color = RED

#define isRightChild(node)  ((node)->parent->right == (node))
#define sibling(node)       (((node) && (node)->parent) ? ((node)->parent->left == (node) ? (node)->parent->right : (node)->parent->left) : 0)

#define swapColors(firstNode, secondNode) do {  \
    unsigned char tmp = (firstNode)->color;     \
    (firstNode)->color = (secondNode)->color;   \
    (secondNode)->color = tmp;                  \
} while (0)

#define replaceChild(tree, child, newChild) do {    \
    if ((child)->parent) {                          \
        if ((child)->parent->left == (child)) {     \
            (child)->parent->left = (newChild);     \
        } else {                                    \
            (child)->parent->right = (newChild);    \
        }                                           \
    }                                               \
} while (0)

#define updateRoot(tree, node) do { \
    if (!(node)->parent) {          \
        (tree)->root = (node);      \
    }                               \
} while (0)

static void rotateLeft(rbtree *tree, rbtreeNode *node) {
    rbtreeNode *newParent = node->right;
    rbtreeNode *oldLeft = newParent->left;
    newParent->left = node;
    newParent->parent = node->parent;
    replaceChild(tree, node, newParent);
    node->parent = newParent;
    node->right = oldLeft;
    if (oldLeft) {
        oldLeft->parent = node;
    }
    updateRoot(tree, newParent);
}

static void rotateRight(rbtree *tree, rbtreeNode *node) {
    rbtreeNode *newParent = node->left;
    rbtreeNode *oldRight = newParent->right;
    newParent->right = node;
    newParent->parent = node->parent;
    replaceChild(tree, node, newParent);
    node->parent = newParent;
    node->left = oldRight;
    if (oldRight) {
        oldRight->parent = node;
    }
    updateRoot(tree, newParent);
}

static void fixInsert(rbtree *tree, rbtreeNode *current) {
    for (;;) {
        rbtreeNode *uncleNode;
        rbtreeNode *parentNode;
        rbtreeNode *grandParentNode;
        parentNode = current->parent;
        if (!parentNode) {
            markBlack(current);
            break;
        }
        if (isBlack(parentNode)) {
            break;
        }
        grandParentNode = parentNode->parent;
        uncleNode = sibling(parentNode);
        if (isRed(uncleNode)) {
            markBlack(parentNode);
            markBlack(uncleNode);
            markRed(uncleNode->parent);
            current = uncleNode->parent;
            continue;
        } else {
            if (parentNode == parentNode->parent->left) {
                if (current == parentNode->right) {
                    rotateLeft(tree, parentNode);
                }
                rotateRight(tree, grandParentNode);
                swapColors(grandParentNode, grandParentNode->parent);
            } else {
                if (current == parentNode->left) {
                    rotateRight(tree, parentNode);
                }
                rotateLeft(tree, grandParentNode);
                swapColors(grandParentNode, grandParentNode->parent);
            }
        }
        break;
    }
}

rbtreeNode *rbtreeFindNode(rbtree *tree, const void *key) {
    rbtreeNode *lastNode = 0;
    int lastCmp = 0;
    return rbtreeSearch(tree, key, &lastNode, &lastCmp);
}

rbtreeNode *rbtreeInsertNode(rbtree *tree, rbtreeNode *node) {
    rbtreeNode *lastNode = 0;
    int lastCmp = 0;
    rbtreeNode *existing = rbtreeSearch(tree, keyOfNode(tree, node), &lastNode, &lastCmp);
    if (existing) {
        return existing;
    }
    node->left = 0;
    node->right = 0;
    if (!lastNode) {
        markBlack(node);
        node->parent = 0;
        tree->root = node;
        return node;
    }
    node->parent = lastNode;
    markRed(node);
    if (lastCmp < 0) {
        lastNode->left = node;
    } else {
        lastNode->right = node;
    }
    fixInsert(tree, node);
    return node;
}

static void rbtreeFixDelete(rbtree *tree, rbtreeNode *fix, unsigned char replacedColor, rbtreeNode *sib) {
    rbtreeNode *parent;
    for (;;) {
        if (isRed(fix) || RED == replacedColor) {
            markBlack(fix);
            break;
        }
        if (tree->root == fix) {
            markBlack(fix);
            break;
        }
        if (isRed(sib)) {
            if (isRightChild(sib)) {
                parent = sib->parent;
                rotateLeft(tree, parent);
                swapColors(parent, parent->parent);
                replacedColor = BLACK;
                fix = parent->left;
                sib = parent->right;
            } else {
                parent = sib->parent;
                rotateRight(tree, parent);
                swapColors(parent, parent->parent);
                replacedColor = BLACK;
                fix = parent->right;
                sib = parent->left;
            }
            continue;
        }
        if (!sib) {
            break;
        }
        if (isRed(sib->left) || isRed(sib->right)) {
            if (isRightChild(sib)) {
                if (isRed(sib->left)) {
                    rotateRight(tree, sib);
                    swapColors(sib, sib->parent);
                    sib = sib->parent;
                    parent = sib->parent;
                } else {
                    parent = sib->parent;
                }
                rotateLeft(tree, parent);
                swapColors(sib, parent);
                markBlack(sib->right);
            } else {
                if (isRed(sib->right)) {
                    rotateLeft(tree, sib);
                    swapColors(sib, sib->parent);
                    sib = sib->parent;
                    parent = sib->parent;
                } else {
                    parent = sib->parent;
                }
                rotateRight(tree, parent);
                swapColors(sib, parent);
                markBlack(sib->left);
            }
            break;
        }
        markRed(sib);
        if (isRed(sib->parent)) {
            markBlack(sib->parent);
            break;
        }
        fix = sib->parent;
        replacedColor = BLACK;
        sib = sibling(fix);
    }
}

static rbtreeNode *successor(rbtreeNode *node) {
    rbtreeNode *search = node->right;
    while (search->left) {
        search = search->left;
    }
    return search;
}

static void copyNode(rbtree *tree, rbtreeNode *from, rbtreeNode *to) {
    replaceChild(tree, to, from);
    from->parent = to->parent;
    from->left = to->left;
    if (to->left) {
        to->left->parent = from;
    }
    from->right = to->right;
    if (to->right) {
        to->right->parent = from;
    }
    updateRoot(tree, from);
}

static void moveNode(rbtree *tree, rbtreeNode *from, rbtreeNode *to) {
    replaceChild(tree, from, 0);
    copyNode(tree, from, to);
}

void rbtreeDeleteNode(rbtree *tree, rbtreeNode *node) {
    rbtreeNode *rep;
    rbtreeNode *sib;
    unsigned char replacedColor = BLACK;
    if (node->left && node->right) {
        rep = successor(node);
        if (rep->right) {
            rbtreeNode *newRep = rep->right;
            replacedColor = rep->color;
            sib = sibling(rep);
            moveNode(tree, newRep, rep);
            copyNode(tree, rep, node);
            rep->color = node->color;
            rep = newRep;
        } else {
            sib = sibling(rep);
            moveNode(tree, rep, node);
            rep->color = node->color;
            rep = 0;
        }
    } else if ((rep = node->left ? node->left : node->right)) {
        sib = sibling(node);
        moveNode(tree, rep, node);
        replacedColor = node->color;
    } else {
        sib = sibling(node);
        if (node->parent) {
            replaceChild(tree, node, 0);
        } else {
            tree->root = 0;
            return;
        }
        rep = node;
    }
    rbtreeFixDelete(tree, rep, replacedColor, sib);
}

void *rbtreeFind(rbtree *tree, const void *key) {
    rbtreeNode *node = rbtreeFindNode(tree, key);
    if (!node) {
        return 0;
    }
    return (void *)((char *)node - tree->nodeOffsetOfParent);
}

void *rbtreeInsert(rbtree *tree, void *current) {
    rbtreeNode *node = rbtreeInsertNode(tree, (rbtreeNode *)((char *)current + tree->nodeOffsetOfParent));
    if (!node) {
        return 0;
    }
    return (void *)((char *)node - tree->nodeOffsetOfParent);
}

void rbtreeDelete(rbtree *tree, void *current) {
    rbtreeDeleteNode(tree, (rbtreeNode *)((char *)current + tree->nodeOffsetOfParent));
}

rbtreeNode *rbtreeFirstNode(rbtree *tree) {
    rbtreeNode *node;
    if (!tree->root) {
        return 0;
    }
    node = tree->root;
    while (node->left) {
        node = node->left;
    }
    return node;
}

rbtreeNode *rbtreeLastNode(rbtree *tree) {
    rbtreeNode *node;
    if (!tree->root) {
        return 0;
    }
    node = tree->root;
    while (node->right) {
        node = node->right;
    }
    return node;
}

rbtreeNode *rbtreeNextNode(rbtreeNode *node) {
    rbtreeNode *parent;
    if (node->right) {
        node = node->right;
        while (node->left) {
            node = node->left;
        }
        return node;
    }
    while ((parent=node->parent) && isRightChild(node)) {
        node = parent;
    }
    return parent;
}

rbtreeNode *rbtreePreviousNode(rbtreeNode *node) {
    rbtreeNode *parent;
    if (node->left) {
        node = node->left;
        while (node->right) {
            node = node->right;
        }
        return node;
    }
    while ((parent=node->parent) && !isRightChild(node)) {
        node = parent;
    }
    return parent;
}

void *rbtreeFirst(rbtree *tree) {
    rbtreeNode *node = rbtreeFirstNode(tree);
    if (!node) {
        return 0;
    }
    return (void *)((char *)node - tree->nodeOffsetOfParent);
}

void *rbtreeLast(rbtree *tree) {
    rbtreeNode *node = rbtreeLastNode(tree);
    if (!node) {
        return 0;
    }
    return (void *)((char *)node - tree->nodeOffsetOfParent);
}

void *rbtreeNext(rbtree *tree, void *current) {
    rbtreeNode *node = rbtreeNextNode((rbtreeNode *)((char *)current + tree->nodeOffsetOfParent));
    if (!node) {
        return 0;
    }
    return (void *)((char *)node - tree->nodeOffsetOfParent);
}

void *rbtreePrevious(rbtree *tree, void *current) {
    rbtreeNode *node = rbtreePreviousNode((rbtreeNode *)((char *)current + tree->nodeOffsetOfParent));
    if (!node) {
        return 0;
    }
    return (void *)((char *)node - tree->nodeOffsetOfParent);
}

////////////////////////////////////////////////////////////////////////////////

#if RBTREE_DEBUG

static void printNode(rbtree *tree, rbtreeNode *node, rbtreeKeyToIntConverter converter) {
    if (!node || (tree->root != node && !node->left && !node->right)) {
        return;
    }
    if (node->left) {
        printf("(%d:%c)", converter(keyOfNode(tree, node->left)), isRed(node->left) ? 'R' : 'B');
    } else {
        printf("NULL");
    }
    printf(" <- (%d:%c) -> ", converter(keyOfNode(tree, node)), isRed(node) ? 'R' : 'B');
    if (node->right) {
        printf("(%d:%c)", converter(keyOfNode(tree, node->right)), isRed(node->right) ? 'R' : 'B');
    } else {
        printf("NULL");
    }
    printf("\n");
    printNode(tree, node->left, converter);
    printNode(tree, node->right, converter);
}

void rbtreePrint(rbtree *tree, rbtreeKeyToIntConverter converter) {
    if (!tree->root) {
        printf("(Empty)\n");
        return;
    }
    printNode(tree, tree->root, converter);
}

int rbtreeDumpNodeToArray(rbtree *tree, rbtreeNode *node, rbtreeKeyToIntConverter converter, int *array, int maxNum, int *num) {
    if (!node) {
        return 0;
    }
    if (*num >= maxNum) {
        return -1;
    }
    array[(*num)++] = converter(keyOfNode(tree, node));
    if (0 != rbtreeDumpNodeToArray(tree, node->left, converter, array, maxNum, num)) {
        return -1;
    }
    if (0 != rbtreeDumpNodeToArray(tree, node->right, converter, array, maxNum, num)) {
        return -1;
    }
    return 0;
}

int rbtreeDumpToArray(rbtree *tree, rbtreeKeyToIntConverter converter, int *array, int maxNum) {
    int num = 0;
    int result = rbtreeDumpNodeToArray(tree, tree->root, converter, array, maxNum, &num);
    if (0 != result) {
        return -1;
    }
    return num;
}

static int checkConsectiveRedNodes(rbtreeNode *node) {
    if (!node) {
        return 0;
    }
    if (isRed(node) && isRed(node->parent)) {
        return -1;
    }
    if (0 != checkConsectiveRedNodes(node->left)) {
        return -1;
    }
    if (0 != checkConsectiveRedNodes(node->right)) {
        return -1;
    }
    return 0;
}

static int checkBlackNodesToLeaf(rbtreeNode *node, int blackNodes, int *cmp) {
    if (!node) {
        if (-1 == *cmp) {
            *cmp = blackNodes;
        }
        if (*cmp != blackNodes) {
            return -1;
        }
        return 0;
    }
    if (isBlack(node)) {
        blackNodes++;
    }
    if (0 != checkBlackNodesToLeaf(node->left, blackNodes, cmp)) {
        return -1;
    }
    if (0 != checkBlackNodesToLeaf(node->right, blackNodes, cmp)) {
        return -1;
    }
    return 0;
}

int rbtreeVerify(rbtree *tree) {
    int blackNodeNumber = -1;
    if (isRed(tree->root)) {
        fprintf(stderr, "Root node is not black\n");
        return -1;
    }
    if (0 != checkConsectiveRedNodes(tree->root)) {
        fprintf(stderr, "Found consective red nodes\n");
        return -1;
    }
    if (0 != checkBlackNodesToLeaf(tree->root, 0, &blackNodeNumber)) {
        fprintf(stderr, "Black nodes number incorrect\n");
        return -1;
    }
    return 0;
 }
#endif
