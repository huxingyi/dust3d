#include <stack>
#include <QUuid>
#include <QDebug>
#include "snapshotxml.h"

static void saveSkeletonComponent(Snapshot *snapshot, QXmlStreamWriter *writer, const QString &componentId)
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

void saveSkeletonToXmlStream(Snapshot *snapshot, QXmlStreamWriter *writer)
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
    
        writer->writeStartElement("materials");
        std::vector<std::pair<std::map<QString, QString>, std::vector<std::pair<std::map<QString, QString>, std::vector<std::map<QString, QString>>>>>>::iterator materialIterator;
        for (materialIterator = snapshot->materials.begin(); materialIterator != snapshot->materials.end(); materialIterator++) {
            std::map<QString, QString>::iterator materialAttributeIterator;
            writer->writeStartElement("material");
                for (materialAttributeIterator = materialIterator->first.begin(); materialAttributeIterator != materialIterator->first.end(); materialAttributeIterator++) {
                    writer->writeAttribute(materialAttributeIterator->first, materialAttributeIterator->second);
                }
                writer->writeStartElement("layers");
                std::vector<std::pair<std::map<QString, QString>, std::vector<std::map<QString, QString>>>>::iterator layerIterator;
                for (layerIterator = materialIterator->second.begin(); layerIterator != materialIterator->second.end(); layerIterator++) {
                    std::map<QString, QString>::iterator layerAttributeIterator;
                    writer->writeStartElement("layer");
                        for (layerAttributeIterator = layerIterator->first.begin(); layerAttributeIterator != layerIterator->first.end(); layerAttributeIterator++) {
                            writer->writeAttribute(layerAttributeIterator->first, layerAttributeIterator->second);
                        }
                        writer->writeStartElement("maps");
                        std::vector<std::map<QString, QString>>::iterator mapIterator;
                        for (mapIterator = layerIterator->second.begin(); mapIterator != layerIterator->second.end(); mapIterator++) {
                            std::map<QString, QString>::iterator attributesIterator;
                            writer->writeStartElement("map");
                            for (attributesIterator = mapIterator->begin(); attributesIterator != mapIterator->end();
                                    attributesIterator++) {
                                writer->writeAttribute(attributesIterator->first, attributesIterator->second);
                            }
                            writer->writeEndElement();
                        }
                        writer->writeEndElement();
                    writer->writeEndElement();
                }
                writer->writeEndElement();
            writer->writeEndElement();
        }
        writer->writeEndElement();
    
        writer->writeStartElement("poses");
        //std::vector<std::pair<std::map<QString, QString>, std::map<QString, std::map<QString, QString>>>>::iterator poseIterator;
        std::vector<std::pair<std::map<QString, QString>, std::vector<std::pair<std::map<QString, QString>, std::map<QString, std::map<QString, QString>>>>>>::iterator poseIterator;
        for (poseIterator = snapshot->poses.begin(); poseIterator != snapshot->poses.end(); poseIterator++) {
            std::map<QString, QString>::iterator poseAttributeIterator;
            writer->writeStartElement("pose");
                for (poseAttributeIterator = poseIterator->first.begin(); poseAttributeIterator != poseIterator->first.end(); poseAttributeIterator++) {
                    writer->writeAttribute(poseAttributeIterator->first, poseAttributeIterator->second);
                }
                writer->writeStartElement("frames");
                    std::vector<std::pair<std::map<QString, QString>, std::map<QString, std::map<QString, QString>>>>::iterator frameIterator;
                    for (frameIterator = poseIterator->second.begin(); frameIterator != poseIterator->second.end(); frameIterator++) {
                        std::map<QString, QString>::iterator frameAttributeIterator;
                        writer->writeStartElement("frame");
                            for (frameAttributeIterator = frameIterator->first.begin(); frameAttributeIterator != frameIterator->first.end(); frameAttributeIterator++) {
                                writer->writeAttribute(frameAttributeIterator->first, frameAttributeIterator->second);
                            }
                            writer->writeStartElement("parameters");
                            std::map<QString, std::map<QString, QString>>::iterator itemsIterator;
                            for (itemsIterator = frameIterator->second.begin(); itemsIterator != frameIterator->second.end(); itemsIterator++) {
                                std::map<QString, QString>::iterator parametersIterator;
                                writer->writeStartElement("parameter");
                                writer->writeAttribute("for", itemsIterator->first);
                                for (parametersIterator = itemsIterator->second.begin(); parametersIterator != itemsIterator->second.end();
                                        parametersIterator++) {
                                    writer->writeAttribute(parametersIterator->first, parametersIterator->second);
                                }
                                writer->writeEndElement();
                            }
                            writer->writeEndElement();
                        writer->writeEndElement();
                    }
                writer->writeEndElement();
            writer->writeEndElement();
        }
        writer->writeEndElement();
    
        writer->writeStartElement("motions");
        std::vector<std::pair<std::map<QString, QString>, std::vector<std::map<QString, QString>>>>::iterator motionIterator;
        for (motionIterator = snapshot->motions.begin(); motionIterator != snapshot->motions.end(); motionIterator++) {
            std::map<QString, QString>::iterator motionAttributeIterator;
            writer->writeStartElement("motion");
                for (motionAttributeIterator = std::get<0>(*motionIterator).begin(); motionAttributeIterator != std::get<0>(*motionIterator).end(); motionAttributeIterator++) {
                    writer->writeAttribute(motionAttributeIterator->first, motionAttributeIterator->second);
                }
                writer->writeStartElement("clips");
                {
                    std::vector<std::map<QString, QString>>::iterator itemsIterator;
                    for (itemsIterator = std::get<1>(*motionIterator).begin(); itemsIterator != std::get<1>(*motionIterator).end(); itemsIterator++) {
                        std::map<QString, QString>::iterator attributesIterator;
                        writer->writeStartElement("clip");
                        for (attributesIterator = itemsIterator->begin(); attributesIterator != itemsIterator->end();
                                attributesIterator++) {
                            writer->writeAttribute(attributesIterator->first, attributesIterator->second);
                        }
                        writer->writeEndElement();
                    }
                }
                writer->writeEndElement();
            writer->writeEndElement();
        }
        writer->writeEndElement();
    
    writer->writeEndElement();
    
    writer->writeEndDocument();
}

void loadSkeletonFromXmlStream(Snapshot *snapshot, QXmlStreamReader &reader, quint32 flags)
{
    std::stack<QString> componentStack;
    std::vector<QString> elementNameStack;
    std::pair<std::map<QString, QString>, std::vector<std::map<QString, QString>>> currentMaterialLayer;
    std::pair<std::map<QString, QString>, std::vector<std::pair<std::map<QString, QString>, std::vector<std::map<QString, QString>>>>> currentMaterial;
    std::pair<std::map<QString, QString>, std::vector<std::pair<std::map<QString, QString>, std::map<QString, std::map<QString, QString>>>>> currentPose;
    std::pair<std::map<QString, QString>, std::map<QString, std::map<QString, QString>>> currentPoseFrame;
    std::pair<std::map<QString, QString>, std::vector<std::map<QString, QString>>> currentMotion;
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
        //qDebug() << (reader.isStartElement() ? "<" : ">") << "fullName:" << fullName << "baseName:" << baseName;
        if (reader.isStartElement()) {
            if (fullName == "canvas") {
                if (flags & SNAPSHOT_ITEM_CANVAS) {
                    foreach(const QXmlStreamAttribute &attr, reader.attributes()) {
                        snapshot->canvas[attr.name().toString()] = attr.value().toString();
                    }
                }
            } else if (fullName == "canvas.nodes.node") {
                QString nodeId = reader.attributes().value("id").toString();
                if (nodeId.isEmpty())
                    continue;
                if (flags & SNAPSHOT_ITEM_COMPONENT) {
                    std::map<QString, QString> *nodeMap = &snapshot->nodes[nodeId];
                    foreach(const QXmlStreamAttribute &attr, reader.attributes()) {
                        (*nodeMap)[attr.name().toString()] = attr.value().toString();
                    }
                }
            } else if (fullName == "canvas.edges.edge") {
                QString edgeId = reader.attributes().value("id").toString();
                if (edgeId.isEmpty())
                    continue;
                if (flags & SNAPSHOT_ITEM_COMPONENT) {
                    std::map<QString, QString> *edgeMap = &snapshot->edges[edgeId];
                    foreach(const QXmlStreamAttribute &attr, reader.attributes()) {
                        (*edgeMap)[attr.name().toString()] = attr.value().toString();
                    }
                }
            } else if (fullName == "canvas.parts.part") {
                QString partId = reader.attributes().value("id").toString();
                if (partId.isEmpty())
                    continue;
                if (flags & SNAPSHOT_ITEM_COMPONENT) {
                    std::map<QString, QString> *partMap = &snapshot->parts[partId];
                    foreach(const QXmlStreamAttribute &attr, reader.attributes()) {
                        (*partMap)[attr.name().toString()] = attr.value().toString();
                    }
                }
            } else if (fullName == "canvas.partIdList.partId") {
                QString partId = reader.attributes().value("id").toString();
                if (partId.isEmpty())
                    continue;
                if (flags & SNAPSHOT_ITEM_COMPONENT) {
                    QString componentId = QUuid::createUuid().toString();
                    auto &component = snapshot->components[componentId];
                    component["id"] = componentId;
                    component["linkData"] = partId;
                    component["linkDataType"] = "partId";
                    auto &childrenIds = snapshot->rootComponent["children"];
                    if (!childrenIds.isEmpty())
                        childrenIds += ",";
                    childrenIds += componentId;
                }
            } else if (fullName.startsWith("canvas.components.component")) {
                QString componentId = reader.attributes().value("id").toString();
                QString parentId;
                if (!componentStack.empty())
                    parentId = componentStack.top();
                componentStack.push(componentId);
                if (componentId.isEmpty())
                    continue;
                if (flags & SNAPSHOT_ITEM_COMPONENT) {
                    std::map<QString, QString> *componentMap = &snapshot->components[componentId];
                    foreach(const QXmlStreamAttribute &attr, reader.attributes()) {
                        (*componentMap)[attr.name().toString()] = attr.value().toString();
                    }
                    auto &parentChildrenIds = parentId.isEmpty() ? snapshot->rootComponent["children"] : snapshot->components[parentId]["children"];
                    if (!parentChildrenIds.isEmpty())
                        parentChildrenIds += ",";
                    parentChildrenIds += componentId;
                }
            } else if (fullName == "canvas.materials.material.layers.layer") {
                currentMaterialLayer = decltype(currentMaterialLayer)();
                foreach(const QXmlStreamAttribute &attr, reader.attributes()) {
                    currentMaterialLayer.first[attr.name().toString()] = attr.value().toString();
                }
            } else if (fullName == "canvas.materials.material.layers.layer.maps.map") {
                std::map<QString, QString> attributes;
                foreach(const QXmlStreamAttribute &attr, reader.attributes()) {
                    attributes[attr.name().toString()] = attr.value().toString();
                }
                currentMaterialLayer.second.push_back(attributes);
            } else if (fullName == "canvas.materials.material") {
                QString materialId = reader.attributes().value("id").toString();
                if (materialId.isEmpty())
                    continue;
                currentMaterial = decltype(currentMaterial)();
                foreach(const QXmlStreamAttribute &attr, reader.attributes()) {
                    currentMaterial.first[attr.name().toString()] = attr.value().toString();
                }
            } else if (fullName == "canvas.poses.pose") {
                QString poseId = reader.attributes().value("id").toString();
                if (poseId.isEmpty())
                    continue;
                currentPose = decltype(currentPose)();
                foreach(const QXmlStreamAttribute &attr, reader.attributes()) {
                    currentPose.first[attr.name().toString()] = attr.value().toString();
                }
            } else if (fullName == "canvas.poses.pose.frames.frame") {
                foreach(const QXmlStreamAttribute &attr, reader.attributes()) {
                    currentPoseFrame.first[attr.name().toString()] = attr.value().toString();
                }
            } else if (fullName == "canvas.poses.pose.frames.frame.parameters.parameter") {
                QString forWhat = reader.attributes().value("for").toString();
                if (forWhat.isEmpty())
                    continue;
                foreach(const QXmlStreamAttribute &attr, reader.attributes()) {
                    if ("for" == attr.name().toString())
                        continue;
                    currentPoseFrame.second[forWhat][attr.name().toString()] = attr.value().toString();
                }
            } else if (fullName == "canvas.motions.motion") {
               QString motionId = reader.attributes().value("id").toString();
               if (motionId.isEmpty())
                   continue;
               currentMotion = decltype(currentMotion)();
               foreach(const QXmlStreamAttribute &attr, reader.attributes()) {
                   currentMotion.first[attr.name().toString()] = attr.value().toString();
               }
           } else if (fullName == "canvas.motions.motion.clips.clip") {
                std::map<QString, QString> attributes;
                foreach(const QXmlStreamAttribute &attr, reader.attributes()) {
                    attributes[attr.name().toString()] = attr.value().toString();
                }
                currentMotion.second.push_back(attributes);
            }
        } else if (reader.isEndElement()) {
            if (fullName.startsWith("canvas.components.component")) {
                componentStack.pop();
            } else if (fullName == "canvas.materials.material.layers.layer") {
                currentMaterial.second.push_back(currentMaterialLayer);
            } else if (fullName == "canvas.materials.material") {
                if (flags & SNAPSHOT_ITEM_MATERIAL) {
                    snapshot->materials.push_back(currentMaterial);
                }
            } else if (fullName == "canvas.poses.pose.frames.frame") {
                currentPose.second.push_back(currentPoseFrame);
            } else if (fullName == "canvas.poses.pose") {
                if (flags & SNAPSHOT_ITEM_POSE) {
                    snapshot->poses.push_back(currentPose);
                }
            } else if (fullName == "canvas.motions.motion") {
                if (flags & SNAPSHOT_ITEM_MOTION) {
                    snapshot->motions.push_back(currentMotion);
                }
            }
        }
    }
}
