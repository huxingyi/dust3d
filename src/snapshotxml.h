#ifndef DUST3D_SNAPSHOT_XML_H
#define DUST3D_SNAPSHOT_XML_H
#include <QXmlStreamWriter>
#include "snapshot.h"

#define SNAPSHOT_ITEM_CANVAS        0x00000001
#define SNAPSHOT_ITEM_COMPONENT     0x00000002
#define SNAPSHOT_ITEM_MATERIAL      0x00000004
#define SNAPSHOT_ITEM_POSE          0x00000008
#define SNAPSHOT_ITEM_MOTION        0x00000010
#define SNAPSHOT_ITEM_ALL          (                                \
                                        SNAPSHOT_ITEM_CANVAS |      \
                                        SNAPSHOT_ITEM_COMPONENT |   \
                                        SNAPSHOT_ITEM_MATERIAL |    \
                                        SNAPSHOT_ITEM_POSE |        \
                                        SNAPSHOT_ITEM_MOTION        \
                                    )

void saveSkeletonToXmlStream(Snapshot *snapshot, QXmlStreamWriter *writer);
void loadSkeletonFromXmlStream(Snapshot *snapshot, QXmlStreamReader &reader, 
    quint32 flags=SNAPSHOT_ITEM_ALL);

#endif
