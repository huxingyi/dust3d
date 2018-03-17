// Copyright 2006 Tobias Sargeant (toby@permuted.net)
// All rights reserved.
#pragma once

#pragma warning (disable : 4996)
#pragma warning (disable : 4786)

typedef char int8_t;
typedef short int16_t;
typedef long int32_t;

typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned long uint32_t;

#include <string.h>
#include <stdlib.h>

inline int strcasecmp(const char *a, const char *b) {
  return _stricmp(a,b);
}

inline void srandom(unsigned long input) {
  srand(input);
}

inline long random() {
  return rand();
}

// intptr_t is an integer type that is big enough to hold a pointer
// It is not defined in VC6 so include a definition here for the older compiler
#if defined(_MSC_VER) && _MSC_VER < 1300
typedef long intptr_t;
typedef unsigned long uintptr_t;
#endif

#if defined(_MSC_VER)
typedef __int64 int64_t;
typedef unsigned __int64 uint64_t;
#endif

#if defined(_MSC_VER)
#  include <carve/cbrt.h>
#endif
