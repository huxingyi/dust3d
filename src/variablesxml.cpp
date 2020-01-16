#include <QDebug>
#include "variablesxml.h"

void saveVariablesToXmlStream(const std::map<QString, std::map<QString, QString>> &variables, QXmlStreamWriter *writer)
{
    writer->setAutoFormatting(true);
    writer->writeStartDocument();
    
    writer->writeStartElement("variables");
        for (const auto &it: variables) {
            writer->writeStartElement("variable");
                writer->writeAttribute("name", it.first);
                for (const auto &subIt: it.second) {
                    if (subIt.first == "name")
                        continue;
                    writer->writeAttribute(subIt.first, subIt.second);
                }
            writer->writeEndElement();
        }
    writer->writeEndElement();
    
    writer->writeEndDocument();
}

void loadVariablesFromXmlStream(std::map<QString, std::map<QString, QString>> *variables, QXmlStreamReader &reader)
{
    std::vector<QString> elementNameStack;
    while (!reader.atEnd()) {
        reader.readNext();
        if (!reader.isStartElement() && !reader.isEndElement()) {
            if (!reader.name().toString().isEmpty())
                qDebug() << "Skip xml element:" << reader.name().toString() << " tokenType:" << reader.tokenType();
            continue;
        }
        QString baseName = reader.name().toString();
        if (reader.isStartElement())
            elementNameStack.push_back(baseName);
        QStringList nameItems;
        for (const auto &nameItem: elementNameStack) {
            nameItems.append(nameItem);
        }
        QString fullName = nameItems.join(".");
        if (reader.isEndElement())
            elementNameStack.pop_back();
        if (reader.isStartElement()) {
            if (fullName == "variables.variable") {
                QString variableName;
                std::map<QString, QString> map;
                foreach(const QXmlStreamAttribute &attr, reader.attributes()) {
                    auto attrNameString = attr.name().toString();
                    auto attrValueString = attr.value().toString();
                    if ("name" == attrNameString) {
                        variableName = attrValueString;
                    } else {
                        map[attrNameString] = attrValueString;
                    }
                }
                if (!variableName.isEmpty()) {
                    (*variables)[variableName] = map;
                }
            }
        }
    }
}
