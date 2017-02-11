#ifndef OS_UTIL_H
#define OS_UTIL_H

#ifdef __cplusplus
extern "C" {
#endif

#include <math.h>

long long osGetMilliseconds(void);

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define unsued(param) (void)param

#ifdef __cplusplus
}
#endif

#endif
