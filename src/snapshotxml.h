#ifndef DUST3D_SNAPSHOT_XML_H
#define DUST3D_SNAPSHOT_XML_H
#include <QXmlStreamWriter>
#include "snapshot.h"

void saveSkeletonToXmlStream(Snapshot *snapshot, QXmlStreamWriter *writer);
void loadSkeletonFromXmlStream(Snapshot *snapshot, QXmlStreamReader &reader);

#endif
