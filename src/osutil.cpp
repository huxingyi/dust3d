#include "osutil.h"
#include <QDateTime>

long long osGetMilliseconds(void) {
  return (long long)QDateTime::currentMSecsSinceEpoch();
}
