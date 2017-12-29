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

#endif
