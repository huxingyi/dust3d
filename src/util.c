#include <QDateTime>
#include <QGLWidget>
#include "osutil.h"

long long osGetMilliseconds(void) {
  return (long long)QDateTime::currentMSecsSinceEpoch();
}
