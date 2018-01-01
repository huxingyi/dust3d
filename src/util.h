#ifndef DUST3D_UTIL_H
#define DUST3D_UTIL_H

#define min(x, y) (((x) < (y)) ? (x) : (y))

#define swap(type, first, second) do {\
    type tmp = (first);\
    (first) = (second);\
    (second) = tmp;\
} while (0)

#define freeList(type, link) do {   \
    while ((link)) {                \
        type *del = (link);         \
        (link) = (link)->next;      \
        free(del);                  \
    }                               \
} while (0)

#define addToList(cur, link) do {   \
    (cur)->next = (link);           \
    (link) = (cur);                 \
} while (0)

#define addToDoubleLinkedListTail(cur, first, last) do {    \
    (cur)->next = 0;                                        \
    if (last) {                                             \
        (last)->next = (cur);                               \
    } else {                                                \
        (first) = (cur);                                    \
    }                                                       \
    (cur)->previous = (last);                               \
    (last) = (cur);                                         \
} while (0)

#define addToDoubleLinkedListHead(cur, first, last) do {    \
    (cur)->previous = 0;                                    \
    if (first) {                                            \
        (first)->previous = (cur);                          \
    } else {                                                \
        (last) = (cur);                                     \
    }                                                       \
    (cur)->next = (first);                                  \
    (first) = (cur);                                        \
} while (0)

#define removeFromDoubleLinkedList(cur, first, last) do {   \
    if ((cur)->next) {                                      \
        (cur)->next->previous = (cur)->previous;            \
    } else {                                                \
        (last) = (cur)->previous;                           \
    }                                                       \
    if ((cur)->previous) {                                  \
        (cur)->previous->next = (cur)->next;                \
    } else {                                                \
        (first) = (cur)->next;                              \
    }                                                       \
} while (0)

#endif
