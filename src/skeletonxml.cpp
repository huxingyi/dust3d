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
    
        writer->writeStartElement("partIdList");
        std::vector<QString>::iterator partIdIterator;
        for (partIdIterator = snapshot->partIdList.begin(); partIdIterator != snapshot->partIdList.end(); partIdIterator++) {
            writer->writeStartElement("partId");
            writer->writeAttribute("id", *partIdIterator);
            writer->writeEndElement();
        }
        writer->writeEndElement();

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
    
        writer->writeStartElement("parts");
        std::map<QString, std::map<QString, QString>>::iterator partIterator;
        for (partIterator = snapshot->parts.begin(); partIterator != snapshot->parts.end(); partIterator++) {
            std::map<QString, QString>::iterator partAttributeIterator;
            writer->writeStartElement("part");
            for (partAttributeIterator = partIterator->second.begin(); partAttributeIterator != partIterator->second.end(); partAttributeIterator++) {
                writer->writeAttribute(partAttributeIterator->first, partAttributeIterator->second);
            }
            writer->writeEndElement();
        }
        writer->writeEndElement();
    
        if (!snapshot->animationParameters.empty()) {
            writer->writeStartElement("animations");
            std::map<QString, std::map<QString, QString>>::iterator animationIterator;
            for (animationIterator = snapshot->animationParameters.begin(); animationIterator != snapshot->animationParameters.end(); animationIterator++) {
                std::map<QString, QString>::iterator animationParamterIterator;
                if (animationIterator->second.find("clip") != animationIterator->second.end()) {
                    writer->writeStartElement("animation");
                    for (animationParamterIterator = animationIterator->second.begin(); animationParamterIterator != animationIterator->second.end(); animationParamterIterator++) {
                        writer->writeAttribute(animationParamterIterator->first, animationParamterIterator->second);
                    }
                    writer->writeEndElement();
                }
            }
            writer->writeEndElement();
        }
    
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
                if (nodeId.isEmpty())
                    continue;
                std::map<QString, QString> *nodeMap = &snapshot->nodes[nodeId];
                foreach(const QXmlStreamAttribute &attr, reader.attributes()) {
                    (*nodeMap)[attr.name().toString()] = attr.value().toString();
                }
            } else if (reader.name() == "edge") {
                QString edgeId = reader.attributes().value("id").toString();
                if (edgeId.isEmpty())
                    continue;
                std::map<QString, QString> *edgeMap = &snapshot->edges[edgeId];
                foreach(const QXmlStreamAttribute &attr, reader.attributes()) {
                    (*edgeMap)[attr.name().toString()] = attr.value().toString();
                }
            } else if (reader.name() == "part") {
                QString partId = reader.attributes().value("id").toString();
                if (partId.isEmpty())
                    continue;
                std::map<QString, QString> *partMap = &snapshot->parts[partId];
                foreach(const QXmlStreamAttribute &attr, reader.attributes()) {
                    (*partMap)[attr.name().toString()] = attr.value().toString();
                }
            } else if (reader.name() == "animation") {
               QString animationClipName = reader.attributes().value("clip").toString();
               if (animationClipName.isEmpty())
                   continue;
               std::map<QString, QString> *animationParameterMap = &snapshot->animationParameters[animationClipName];
               foreach(const QXmlStreamAttribute &attr, reader.attributes()) {
                   (*animationParameterMap)[attr.name().toString()] = attr.value().toString();
               }
           } else if (reader.name() == "partId") {
                QString partId = reader.attributes().value("id").toString();
                if (partId.isEmpty())
                    continue;
                snapshot->partIdList.push_back(partId);
            }
        }
    }
}
