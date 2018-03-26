#include "skeletonxml.h"

void saveSkeletonToXmlStream(SkeletonSnapshot *snapshot, QXmlStreamWriter *writer)
{
    writer->setAutoFormatting(true);
    writer->writeStartDocument();
    
    writer->writeStartElement("canvas");
        std::map<QString, QString>::iterator canvasIterator;
        for (canvasIterator = snapshot->canvas.begin(); canvasIterator != snapshot->canvas.end(); canvasIterator++) {
            writer->writeAttribute(canvasIterator->first, canvasIterator->second);
        }

        writer->writeStartElement("nodes");
        std::map<QString, std::map<QString, QString>>::iterator nodeIterator;
        for (nodeIterator = snapshot->nodes.begin(); nodeIterator != snapshot->nodes.end(); nodeIterator++) {
            std::map<QString, QString>::iterator nodeAttributeIterator;
            writer->writeStartElement("node");
            for (nodeAttributeIterator = nodeIterator->second.begin(); nodeAttributeIterator != nodeIterator->second.end(); nodeAttributeIterator++) {
                writer->writeAttribute(nodeAttributeIterator->first, nodeAttributeIterator->second);
            }
            writer->writeEndElement();
        }
        writer->writeEndElement();
    
        writer->writeStartElement("edges");
        std::map<QString, std::map<QString, QString>>::iterator edgeIterator;
        for (edgeIterator = snapshot->edges.begin(); edgeIterator != snapshot->edges.end(); edgeIterator++) {
            std::map<QString, QString>::iterator edgeAttributeIterator;
            writer->writeStartElement("edge");
            for (edgeAttributeIterator = edgeIterator->second.begin(); edgeAttributeIterator != edgeIterator->second.end(); edgeAttributeIterator++) {
                writer->writeAttribute(edgeAttributeIterator->first, edgeAttributeIterator->second);
            }
            writer->writeEndElement();
        }
        writer->writeEndElement();
    writer->writeEndElement();
    
    writer->writeEndDocument();
}

void loadSkeletonFromXmlStream(SkeletonSnapshot *snapshot, QXmlStreamReader &reader)
{
    while (!reader.atEnd()) {
        reader.readNext();
        if (reader.isStartElement()) {
            if (reader.name() == "canvas") {
                foreach(const QXmlStreamAttribute &attr, reader.attributes()) {
                    snapshot->canvas[attr.name().toString()] = attr.value().toString();
                }
            } else if (reader.name() == "node") {
                QString nodeId = reader.attributes().value("id").toString();
                std::map<QString, QString> *nodeMap = &snapshot->nodes[nodeId];
                foreach(const QXmlStreamAttribute &attr, reader.attributes()) {
                    (*nodeMap)[attr.name().toString()] = attr.value().toString();
                }
            } else if (reader.name() == "edge") {
                QString nodeId = reader.attributes().value("id").toString();
                std::map<QString, QString> *edgeMap = &snapshot->edges[nodeId];
                foreach(const QXmlStreamAttribute &attr, reader.attributes()) {
                    (*edgeMap)[attr.name().toString()] = attr.value().toString();
                }
            }
        }
    }
}
