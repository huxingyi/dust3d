#include <stack>
#include <QUuid>
#include <QDebug>
#include <QStringList>
#include <QRegExp>
#include "objectxml.h"
#include "util.h"

void saveObjectToXmlStream(const Object *object, QXmlStreamWriter *writer)
{
    std::map<std::pair<QUuid, QUuid>, size_t> nodeIdMap;
    for (size_t i = 0; i < object->nodes.size(); ++i) {
        const auto &it = object->nodes[i];
        nodeIdMap.insert({{it.partId, it.nodeId}, i});
    }
        
    writer->setAutoFormatting(true);
    writer->writeStartDocument();
    
    writer->writeStartElement("object");
        if (object->alphaEnabled)
            writer->writeAttribute("alphaEnabled", "true");
        writer->writeStartElement("nodes");
        for (const auto &node: object->nodes) {
            writer->writeStartElement("node");
            writer->writeAttribute("partId", node.partId.toString());
            writer->writeAttribute("id", node.nodeId.toString());
            writer->writeAttribute("x", QString::number(node.origin.x()));
            writer->writeAttribute("y", QString::number(node.origin.y()));
            writer->writeAttribute("z", QString::number(node.origin.z()));
            writer->writeAttribute("radius", QString::number(node.radius));
            writer->writeAttribute("color", node.color.name(QColor::HexArgb));
            writer->writeAttribute("colorSolubility", QString::number(node.colorSolubility));
            writer->writeAttribute("metallic", QString::number(node.metalness));
            writer->writeAttribute("roughness", QString::number(node.roughness));
            if (!node.materialId.isNull())
                writer->writeAttribute("materialId", node.materialId.toString());
            if (node.countershaded)
                writer->writeAttribute("countershaded", "true");
            if (!node.mirrorFromPartId.isNull())
                writer->writeAttribute("mirrorFromPartId", node.mirrorFromPartId.toString());
            if (!node.mirroredByPartId.isNull())
                writer->writeAttribute("mirroredByPartId", node.mirroredByPartId.toString());
            if (node.boneMark != BoneMark::None)
                writer->writeAttribute("boneMark", BoneMarkToString(node.boneMark));
            if (!node.joined)
                writer->writeAttribute("joined", "false");
            writer->writeEndElement();
        }
        writer->writeEndElement();
        
        writer->writeStartElement("edges");
        for (const auto &edge: object->edges) {
            writer->writeStartElement("edge");
            writer->writeAttribute("fromPartId", edge.first.first.toString());
            writer->writeAttribute("fromNodeId", edge.first.second.toString());
            writer->writeAttribute("toPartId", edge.second.first.toString());
            writer->writeAttribute("toNodeId", edge.second.second.toString());
            writer->writeEndElement();
        }
        writer->writeEndElement();
        
        writer->writeStartElement("vertices");
        QStringList vertexList;
        for (const auto &vertex: object->vertices) {
            vertexList += QString::number(vertex.x()) + "," + QString::number(vertex.y()) + "," + QString::number(vertex.z());
        }
        writer->writeCharacters(vertexList.join(" "));
        writer->writeEndElement();

        writer->writeStartElement("vertexSourceNodes");
        QStringList vertexSourceNodeList;
        for (const auto &it: object->vertexSourceNodes) {
            auto findIndex = nodeIdMap.find(it);
            if (findIndex == nodeIdMap.end()) {
                vertexSourceNodeList += "-1";
            } else {
                vertexSourceNodeList += QString::number(findIndex->second);
            }
        }
        writer->writeCharacters(vertexSourceNodeList.join(" "));
        writer->writeEndElement();
    
        writer->writeStartElement("triangleAndQuads");
        QStringList triangleAndQuadList;
        for (const auto &it: object->triangleAndQuads) {
            QStringList face;
            for (const auto &index: it)
                face += QString::number(index);
            triangleAndQuadList += face.join(",");
        }
        writer->writeCharacters(triangleAndQuadList.join(" "));
        writer->writeEndElement();
        
        writer->writeStartElement("triangles");
        QStringList triangleList;
        for (const auto &it: object->triangles) {
            QStringList face;
            for (const auto &index: it)
                face += QString::number(index);
            triangleList += face.join(",");
        }
        writer->writeCharacters(triangleList.join(" "));
        writer->writeEndElement();
        
        writer->writeStartElement("triangleNormals");
        QStringList triangleNormalList;
        for (const auto &normal: object->triangleNormals) {
            triangleNormalList += QString::number(normal.x()) + "," + QString::number(normal.y()) + "," + QString::number(normal.z());
        }
        writer->writeCharacters(triangleNormalList.join(" "));
        writer->writeEndElement();
        
        writer->writeStartElement("triangleColors");
        QStringList triangleColorList;
        for (const auto &color: object->triangleColors) {
            triangleColorList += color.name(QColor::HexArgb);
        }
        writer->writeCharacters(triangleColorList.join(" "));
        writer->writeEndElement();
        
        const std::vector<std::pair<QUuid, QUuid>> *triangleSourceNodes = object->triangleSourceNodes();
        if (nullptr != triangleSourceNodes) {
            writer->writeStartElement("triangleSourceNodes");
            QStringList triangleSourceNodeList;
            for (const auto &it: *triangleSourceNodes) {
                auto findIndex = nodeIdMap.find(it);
                if (findIndex == nodeIdMap.end()) {
                    triangleSourceNodeList += "-1";
                } else {
                    triangleSourceNodeList += QString::number(findIndex->second);
                }
            }
            writer->writeCharacters(triangleSourceNodeList.join(" "));
            writer->writeEndElement();
        }
        
        const std::vector<std::vector<QVector2D>> *triangleVertexUvs = object->triangleVertexUvs();
        if (nullptr != triangleVertexUvs) {
            writer->writeStartElement("triangleVertexUvs");
            QStringList triangleVertexUvList;
            for (const auto &triangleUvs: *triangleVertexUvs) {
                for (const auto &uv: triangleUvs) {
                    triangleVertexUvList += QString::number(uv.x()) + "," + QString::number(uv.y());
                }
            }
            writer->writeCharacters(triangleVertexUvList.join(" "));
            writer->writeEndElement();
        }
        
        const std::vector<std::vector<QVector3D>> *triangleVertexNormals = object->triangleVertexNormals();
        if (nullptr != triangleVertexNormals) {
            writer->writeStartElement("triangleVertexNormals");
            QStringList triangleVertexNormalList;
            for (const auto &triangleNormals: *triangleVertexNormals) {
                for (const auto &normal: triangleNormals) {
                    triangleVertexNormalList += QString::number(normal.x()) + "," + QString::number(normal.y()) + "," + QString::number(normal.z());
                }
            }
            writer->writeCharacters(triangleVertexNormalList.join(" "));
            writer->writeEndElement();
        }
        
        const std::vector<QVector3D> *triangleTangents = object->triangleTangents();
        if (nullptr != triangleTangents) {
            writer->writeStartElement("triangleTangents");
            QStringList triangleTangentList;
            for (const auto &tangent: *triangleTangents) {
                triangleTangentList += QString::number(tangent.x()) + "," + QString::number(tangent.y()) + "," + QString::number(tangent.z());
            }
            writer->writeCharacters(triangleTangentList.join(" "));
            writer->writeEndElement();
        }
        
        const std::map<QUuid, std::vector<QRectF>> *partUvRects = object->partUvRects();
        if (nullptr != partUvRects) {
            writer->writeStartElement("uvAreas");
            for (const auto &it: *partUvRects) {
                for (const auto &rect: it.second) {
                    writer->writeStartElement("uvArea");
                    writer->writeAttribute("partId", it.first.toString());
                    writer->writeAttribute("left", QString::number(rect.left()));
                    writer->writeAttribute("top", QString::number(rect.top()));
                    writer->writeAttribute("width", QString::number(rect.width()));
                    writer->writeAttribute("height", QString::number(rect.height()));
                    writer->writeEndElement();
                }
            }
            writer->writeEndElement();
        }
        
        const std::vector<std::pair<std::pair<size_t, size_t>, std::pair<size_t, size_t>>> *triangleLinks = object->triangleLinks();
        if (nullptr != triangleLinks) {
            writer->writeStartElement("triangleLinks");
            QStringList triangleLinkList;
            for (const auto &link: *triangleLinks) {
                triangleLinkList += QString::number(link.first.first) + "," + QString::number(link.first.second) + "," + QString::number(link.second.first) + "," + QString::number(link.second.second);
            }
            writer->writeCharacters(triangleLinkList.join(" "));
            writer->writeEndElement();
        }
    
    writer->writeEndElement();
    writer->writeEndDocument();
}

void loadObjectFromXmlStream(Object *object, QXmlStreamReader &reader)
{
    std::map<QUuid, std::vector<QRectF>> partUvRects;
    std::vector<QString> elementNameStack;
    while (!reader.atEnd()) {
        reader.readNext();
        if (!reader.isStartElement() && !reader.isEndElement() && !reader.isCharacters()) {
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
            if (fullName == "object") {
                object->alphaEnabled = isTrueValueString(reader.attributes().value("alphaEnabled").toString());
            } else if (fullName == "object.nodes.node") {
                QString nodeId = reader.attributes().value("id").toString();
                if (nodeId.isEmpty())
                    continue;
                ObjectNode node;
                node.nodeId = QUuid(nodeId);
                node.partId = QUuid(reader.attributes().value("partId").toString());
                node.origin.setX(reader.attributes().value("x").toFloat());
                node.origin.setY(reader.attributes().value("y").toFloat());
                node.origin.setZ(reader.attributes().value("z").toFloat());
                node.radius = reader.attributes().value("radius").toFloat();
                node.color = QColor(reader.attributes().value("color").toString());
                node.colorSolubility = reader.attributes().value("colorSolubility").toFloat();
                node.metalness = reader.attributes().value("metallic").toFloat();
                node.roughness = reader.attributes().value("roughness").toFloat();
                node.materialId = QUuid(reader.attributes().value("materialId").toString());
                node.countershaded = isTrueValueString(reader.attributes().value("countershaded").toString());
                node.mirrorFromPartId = QUuid(reader.attributes().value("mirrorFromPartId").toString());
                node.mirroredByPartId = QUuid(reader.attributes().value("mirroredByPartId").toString());
                node.boneMark = BoneMarkFromString(reader.attributes().value("boneMark").toString().toUtf8().constData());
                QString joinedString = reader.attributes().value("joined").toString();
                if (!joinedString.isEmpty())
                    node.joined = isTrueValueString(joinedString);
                object->nodes.push_back(node);
            } else if (fullName == "object.edges.edge") {
                std::pair<std::pair<QUuid, QUuid>, std::pair<QUuid, QUuid>> edge;
                edge.first.first = QUuid(reader.attributes().value("fromPartId").toString());
                edge.first.second = QUuid(reader.attributes().value("fromNodeId").toString());
                edge.second.first = QUuid(reader.attributes().value("toPartId").toString());
                edge.second.second = QUuid(reader.attributes().value("toNodeId").toString());
                object->edges.push_back(edge);
            } else if (fullName == "object.uvAreas.uvArea") {
                QUuid partId = QUuid(reader.attributes().value("partId").toString());
                if (!partId.isNull()) {
                    QRectF area(reader.attributes().value("left").toFloat(),
                        reader.attributes().value("top").toFloat(),
                        reader.attributes().value("width").toFloat(),
                        reader.attributes().value("height").toFloat());
                    partUvRects[partId].push_back(area);
                }
            }
        } else if (reader.isEndElement()) {
            if (fullName == "object.uvAreas") {
                object->setPartUvRects(partUvRects);
            }
        } else if (reader.isCharacters()) {
            if (fullName == "object.vertices") {
                QStringList list = reader.text().toString().split(QRegExp("\\s+"), QString::SkipEmptyParts);
                for (const auto &item: list) {
                    auto subItems = item.split(",");
                    if (3 != subItems.size())
                        continue;
                    object->vertices.push_back({subItems[0].toFloat(), 
                        subItems[1].toFloat(), 
                        subItems[2].toFloat()});
                }
            } else if (fullName == "object.vertexSourceNodes") {
                QStringList list = reader.text().toString().split(QRegExp("\\s+"), QString::SkipEmptyParts);
                for (const auto &item: list) {
                    int index = item.toInt();
                    if (index < 0 || index >= object->nodes.size()) {
                        object->vertexSourceNodes.push_back({QUuid(), QUuid()});
                    } else {
                        const auto &node = object->nodes[index];
                        object->vertexSourceNodes.push_back({node.partId, node.nodeId});
                    }
                }
            } else if (fullName == "object.triangleAndQuads") {
                QStringList list = reader.text().toString().split(QRegExp("\\s+"), QString::SkipEmptyParts);
                for (const auto &item: list) {
                    auto subItems = item.split(",");
                    if (3 == subItems.size()) {
                        object->triangleAndQuads.push_back({(size_t)subItems[0].toInt(), 
                            (size_t)subItems[1].toInt(), 
                            (size_t)subItems[2].toInt()});
                    } else if (4 == subItems.size()) {
                        object->triangleAndQuads.push_back({(size_t)subItems[0].toInt(), 
                            (size_t)subItems[1].toInt(), 
                            (size_t)subItems[2].toInt(), 
                            (size_t)subItems[3].toInt()});
                    }
                }
            } else if (fullName == "object.triangles") {
                QStringList list = reader.text().toString().split(QRegExp("\\s+"), QString::SkipEmptyParts);
                for (const auto &item: list) {
                    auto subItems = item.split(",");
                    if (3 == subItems.size()) {
                        object->triangles.push_back({(size_t)subItems[0].toInt(), 
                            (size_t)subItems[1].toInt(), 
                            (size_t)subItems[2].toInt()});
                    }
                }
            } else if (fullName == "object.triangleNormals") {
                QStringList list = reader.text().toString().split(QRegExp("\\s+"), QString::SkipEmptyParts);
                for (const auto &item: list) {
                    auto subItems = item.split(",");
                    if (3 != subItems.size())
                        continue;
                    object->triangleNormals.push_back({subItems[0].toFloat(), 
                        subItems[1].toFloat(), 
                        subItems[2].toFloat()});
                }
            } else if (fullName == "object.triangleColors") {
                QStringList list = reader.text().toString().split(QRegExp("\\s+"), QString::SkipEmptyParts);
                for (const auto &item: list) {
                    object->triangleColors.push_back(QColor(item));
                }
            } else if (fullName == "object.triangleSourceNodes") {
                QStringList list = reader.text().toString().split(QRegExp("\\s+"), QString::SkipEmptyParts);
                std::vector<std::pair<QUuid, QUuid>> triangleSourceNodes;
                for (const auto &item: list) {
                    int index = item.toInt();
                    if (index < 0 || index >= object->nodes.size()) {
                        triangleSourceNodes.push_back({QUuid(), QUuid()});
                    } else {
                        const auto &node = object->nodes[index];
                        triangleSourceNodes.push_back({node.partId, node.nodeId});
                    }
                }
                if (triangleSourceNodes.size() == object->triangles.size())
                    object->setTriangleSourceNodes(triangleSourceNodes);
            } else if (fullName == "object.triangleVertexUvs") {
                QStringList list = reader.text().toString().split(QRegExp("\\s+"), QString::SkipEmptyParts);
                std::vector<QVector2D> uvs;
                for (const auto &item: list) {
                    auto subItems = item.split(",");
                    if (2 != subItems.size())
                        continue;
                    uvs.push_back({subItems[0].toFloat(), 
                        subItems[1].toFloat()});
                }
                std::vector<std::vector<QVector2D>> triangleVertexUvs;
                if (0 == uvs.size() % 3) {
                    for (size_t i = 0; i < uvs.size(); i += 3) {
                        triangleVertexUvs.push_back({
                            uvs[i], uvs[i + 1], uvs[i + 2]
                        });
                    }
                }
                if (triangleVertexUvs.size() == object->triangles.size())
                    object->setTriangleVertexUvs(triangleVertexUvs);
            } else if (fullName == "object.triangleVertexNormals") {
                QStringList list = reader.text().toString().split(QRegExp("\\s+"), QString::SkipEmptyParts);
                std::vector<QVector3D> normals;
                for (const auto &item: list) {
                    auto subItems = item.split(",");
                    if (3 != subItems.size())
                        continue;
                    normals.push_back({subItems[0].toFloat(), 
                        subItems[1].toFloat(),
                        subItems[2].toFloat()});
                }
                std::vector<std::vector<QVector3D>> triangleVertexNormals;
                if (0 == normals.size() % 3) {
                    for (size_t i = 0; i < normals.size(); i += 3) {
                        triangleVertexNormals.push_back({
                            normals[i], normals[i + 1], normals[i + 2]
                        });
                    }
                }
                if (triangleVertexNormals.size() == object->triangles.size())
                    object->setTriangleVertexNormals(triangleVertexNormals);
            } else if (fullName == "object.triangleTangents") {
                QStringList list = reader.text().toString().split(QRegExp("\\s+"), QString::SkipEmptyParts);
                std::vector<QVector3D> triangleTangents;
                for (const auto &item: list) {
                    auto subItems = item.split(",");
                    if (3 != subItems.size())
                        continue;
                    triangleTangents.push_back({subItems[0].toFloat(), 
                        subItems[1].toFloat(),
                        subItems[2].toFloat()});
                }
                if (triangleTangents.size() == object->triangles.size())
                    object->setTriangleTangents(triangleTangents);
            } else if (fullName == "object.triangleLinks") {
                QStringList list = reader.text().toString().split(QRegExp("\\s+"), QString::SkipEmptyParts);
                std::vector<std::pair<std::pair<size_t, size_t>, std::pair<size_t, size_t>>> triangleLinks;
                for (const auto &item: list) {
                    auto subItems = item.split(",");
                    if (4 != subItems.size())
                        continue;
                    triangleLinks.push_back({{(size_t)subItems[0].toInt(), (size_t)subItems[1].toInt()}, 
                        {(size_t)subItems[2].toInt(), (size_t)subItems[3].toInt()}});
                }
                if (triangleLinks.size() == object->triangles.size())
                    object->setTriangleLinks(triangleLinks);
            }
        }
    }
}
