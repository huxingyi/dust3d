#ifndef SKELETON_XML_H
#define SKELETON_XML_H
#include <QXmlStreamWriter>
#include "skeletonsnapshot.h"

void saveSkeletonToXmlStream(SkeletonSnapshot *snapshot, QXmlStreamWriter *writer);
void loadSkeletonFromXmlStream(SkeletonSnapshot *snapshot, QXmlStreamReader &reader);

#endif
