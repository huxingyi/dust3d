#include "debug.h"

QDebug operator<<(QDebug debug, const dust3d::Uuid& uuid)
{
    QDebugStateSaver stateSaver(debug);
    debug.nospace() << uuid.toString().c_str();
    return debug;
}

QDebug operator<<(QDebug debug, const std::string& string)
{
    QDebugStateSaver stateSaver(debug);
    debug.nospace() << string.c_str();
    return debug;
}
