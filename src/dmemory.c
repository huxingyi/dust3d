#include <stdio.h>
#include <stdlib.h>
#include "dmemory.h"

static void outOfMemory(void) {
  fprintf(stderr, "Out of memory\n");
  abort();
}

void *dmalloc(int size) {
  void *ptr = malloc(size);
  if (!ptr) {
    outOfMemory();
  }
  return ptr;
}

void *dcalloc(int nitems, int size) {
  void *ptr = calloc(nitems, size);
  if (!ptr) {
    outOfMemory();
  }
  return ptr;
}

void *drealloc(void *ptr, int size) {
  ptr = realloc(ptr, size);
  if (!ptr) {
    outOfMemory();
  }
  return ptr;
}

void dfree(void *ptr) {
  free(ptr);
}
