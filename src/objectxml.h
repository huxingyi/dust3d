#ifndef DUST3D_OBJECT_XML_H
#define DUST3D_OBJECT_XML_H
#include <QXmlStreamWriter>
#include "object.h"

void saveObjectToXmlStream(const Object *object, QXmlStreamWriter *writer);
void loadObjectFromXmlStream(Object *object, QXmlStreamReader &reader);

#endif
