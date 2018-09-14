#include <stack>
#include <QUuid>
#include <QDebug>
#include "skeletonxml.h"

static void saveSkeletonComponent(SkeletonSnapshot *snapshot, QXmlStreamWriter *writer, const QString &componentId)
{
    const auto findComponent = snapshot->components.find(componentId);
    if (findComponent == snapshot->components.end())
        return;
    auto &component = findComponent->second;
    writer->writeStartElement("component");
    std::map<QString, QString>::iterator componentAttributeIterator;
    QString children;
    for (componentAttributeIterator = component.begin(); componentAttributeIterator != component.end(); componentAttributeIterator++) {
        if ("children" == componentAttributeIterator->first) {
            children = componentAttributeIterator->second;
            continue;
        }
        if ("dirty" == componentAttributeIterator->first)
            continue;
        writer->writeAttribute(componentAttributeIterator->first, componentAttributeIterator->second);
    }
    if (!children.isEmpty()) {
        for (const auto &childId: children.split(",")) {
            if (childId.isEmpty())
                continue;
            saveSkeletonComponent(snapshot, writer, childId);
        }
    }
    writer->writeEndElement();
}

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
    
        writer->writeStartElement("parts");
        std::map<QString, std::map<QString, QString>>::iterator partIterator;
        for (partIterator = snapshot->parts.begin(); partIterator != snapshot->parts.end(); partIterator++) {
            std::map<QString, QString>::iterator partAttributeIterator;
            writer->writeStartElement("part");
            for (partAttributeIterator = partIterator->second.begin(); partAttributeIterator != partIterator->second.end(); partAttributeIterator++) {
                if ("dirty" == partAttributeIterator->first)
                    continue;
                writer->writeAttribute(partAttributeIterator->first, partAttributeIterator->second);
            }
            writer->writeEndElement();
        }
        writer->writeEndElement();
    
        const auto &childrenIds = snapshot->rootComponent.find("children");
        if (childrenIds != snapshot->rootComponent.end()) {
            writer->writeStartElement("components");
            for (const auto &componentId: childrenIds->second.split(",")) {
                if (componentId.isEmpty())
                    continue;
                saveSkeletonComponent(snapshot, writer, componentId);
            }
            writer->writeEndElement();
        }
    
    writer->writeEndElement();
    
    writer->writeEndDocument();
}

void loadSkeletonFromXmlStream(SkeletonSnapshot *snapshot, QXmlStreamReader &reader)
{
    std::stack<QString> componentStack;
    std::vector<QString> elementNameStack;
    while (!reader.atEnd()) {
        reader.readNext();
        if (!reader.isStartElement() && !reader.isEndElement())
            continue;
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
            if (fullName == "canvas") {
                foreach(const QXmlStreamAttribute &attr, reader.attributes()) {
                    snapshot->canvas[attr.name().toString()] = attr.value().toString();
                }
            } else if (fullName == "canvas.nodes.node") {
                QString nodeId = reader.attributes().value("id").toString();
                if (nodeId.isEmpty())
                    continue;
                std::map<QString, QString> *nodeMap = &snapshot->nodes[nodeId];
                foreach(const QXmlStreamAttribute &attr, reader.attributes()) {
                    (*nodeMap)[attr.name().toString()] = attr.value().toString();
                }
            } else if (fullName == "canvas.edges.edge") {
                QString edgeId = reader.attributes().value("id").toString();
                if (edgeId.isEmpty())
                    continue;
                std::map<QString, QString> *edgeMap = &snapshot->edges[edgeId];
                foreach(const QXmlStreamAttribute &attr, reader.attributes()) {
                    (*edgeMap)[attr.name().toString()] = attr.value().toString();
                }
            } else if (fullName == "canvas.parts.part") {
                QString partId = reader.attributes().value("id").toString();
                if (partId.isEmpty())
                    continue;
                std::map<QString, QString> *partMap = &snapshot->parts[partId];
                foreach(const QXmlStreamAttribute &attr, reader.attributes()) {
                    (*partMap)[attr.name().toString()] = attr.value().toString();
                }
            } else if (fullName == "canvas.partIdList.partId") {
                QString partId = reader.attributes().value("id").toString();
                if (partId.isEmpty())
                    continue;
                QString componentId = QUuid::createUuid().toString();
                auto &component = snapshot->components[componentId];
                component["id"] = componentId;
                component["linkData"] = partId;
                component["linkDataType"] = "partId";
                auto &childrenIds = snapshot->rootComponent["children"];
                if (!childrenIds.isEmpty())
                    childrenIds += ",";
                childrenIds += componentId;
            } else if (fullName.startsWith("canvas.components.component")) {
                QString componentId = reader.attributes().value("id").toString();
                QString parentId;
                if (!componentStack.empty())
                    parentId = componentStack.top();
                componentStack.push(componentId);
                if (componentId.isEmpty())
                    continue;
                std::map<QString, QString> *componentMap = &snapshot->components[componentId];
                foreach(const QXmlStreamAttribute &attr, reader.attributes()) {
                    (*componentMap)[attr.name().toString()] = attr.value().toString();
                }
                auto &parentChildrenIds = parentId.isEmpty() ? snapshot->rootComponent["children"] : snapshot->components[parentId]["children"];
                if (!parentChildrenIds.isEmpty())
                    parentChildrenIds += ",";
                parentChildrenIds += componentId;
            }
        } else if (reader.isEndElement()) {
            if (fullName.startsWith("canvas.components.component")) {
                componentStack.pop();
            }
        }
    }
}
