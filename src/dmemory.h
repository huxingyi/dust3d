#ifndef DMEMORY_H
#define DMEMORY_H

void *dmalloc(int size);
void *dcalloc(int nitems, int size);
void *drealloc(void *ptr, int size);
void dfree(void *ptr);

#endif
