#ifndef DUST3D_VARIABLES_XML_H
#define DUST3D_VARIABLES_XML_H
#include <QXmlStreamWriter>
#include <map>

void saveVariablesToXmlStream(const std::map<QString, std::map<QString, QString>> &variables, QXmlStreamWriter *writer);
void loadVariablesFromXmlStream(std::map<QString, std::map<QString, QString>> *variables, QXmlStreamReader &reader);

#endif
