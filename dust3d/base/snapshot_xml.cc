/*
 *  Copyright (c) 2016-2021 Jeremy HU <jeremy-at-dust3d dot org>. All rights reserved. 
 *
 *  Permission is hereby granted, free of charge, to any person obtaining a copy
 *  of this software and associated documentation files (the "Software"), to deal
 *  in the Software without restriction, including without limitation the rights
 *  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 *  copies of the Software, and to permit persons to whom the Software is
 *  furnished to do so, subject to the following conditions:

 *  The above copyright notice and this permission notice shall be included in all
 *  copies or substantial portions of the Software.

 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 *  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 *  SOFTWARE.
 */

#include <dust3d/base/debug.h>
#include <dust3d/base/snapshot_xml.h>
#include <dust3d/base/string.h>
#include <dust3d/base/uuid.h>
#include <iostream>
#include <rapidxml.hpp>
#include <stack>

namespace dust3d {

static std::string loadComponentReturnChildrenList(Snapshot* snapshot, rapidxml::xml_node<>* parentNode)
{
    std::vector<std::string> children;
    for (rapidxml::xml_node<>* node = parentNode->first_node(); nullptr != node; node = node->next_sibling()) {
        rapidxml::xml_attribute<>* idAttribute = node->first_attribute("id");
        if (nullptr != idAttribute) {
            std::string componentIdString = idAttribute->value();
            children.push_back(componentIdString);
            std::map<std::string, std::string>* componentMap = &snapshot->components[componentIdString];
            for (rapidxml::xml_attribute<>* attribute = node->first_attribute();
                 attribute; attribute = attribute->next_attribute()) {
                (*componentMap)[attribute->name()] = attribute->value();
            }
            snapshot->components[componentIdString]["children"] = loadComponentReturnChildrenList(snapshot, node);
        }
    }
    return String::join(children, ",");
}

void loadSnapshotFromXmlString(Snapshot* snapshot, char* xmlString)
{
    try {
        rapidxml::xml_document<> xml;
        xml.parse<0>(xmlString);
        rapidxml::xml_node<>* canvas = xml.first_node("canvas");
        if (nullptr != canvas) {
            for (rapidxml::xml_attribute<>* attribute = canvas->first_attribute();
                 attribute; attribute = attribute->next_attribute()) {
                snapshot->canvas[attribute->name()] = attribute->value();
            }

            rapidxml::xml_node<>* nodes = canvas->first_node("nodes");
            if (nullptr != nodes) {
                for (rapidxml::xml_node<>* node = nodes->first_node(); nullptr != node; node = node->next_sibling()) {
                    rapidxml::xml_attribute<>* idAttribute = node->first_attribute("id");
                    if (nullptr != idAttribute) {
                        std::map<std::string, std::string>* nodeMap = &snapshot->nodes[idAttribute->value()];
                        for (rapidxml::xml_attribute<>* attribute = node->first_attribute();
                             attribute; attribute = attribute->next_attribute()) {
                            (*nodeMap)[attribute->name()] = attribute->value();
                        }
                    }
                }
            }

            rapidxml::xml_node<>* edges = canvas->first_node("edges");
            if (nullptr != edges) {
                for (rapidxml::xml_node<>* node = edges->first_node(); nullptr != node; node = node->next_sibling()) {
                    rapidxml::xml_attribute<>* idAttribute = node->first_attribute("id");
                    if (nullptr != idAttribute) {
                        std::map<std::string, std::string>* edgeMap = &snapshot->edges[idAttribute->value()];
                        for (rapidxml::xml_attribute<>* attribute = node->first_attribute();
                             attribute; attribute = attribute->next_attribute()) {
                            (*edgeMap)[attribute->name()] = attribute->value();
                        }
                    }
                }
            }

            rapidxml::xml_node<>* parts = canvas->first_node("parts");
            if (nullptr != parts) {
                for (rapidxml::xml_node<>* node = parts->first_node(); nullptr != node; node = node->next_sibling()) {
                    rapidxml::xml_attribute<>* idAttribute = node->first_attribute("id");
                    if (nullptr != idAttribute) {
                        std::map<std::string, std::string>* partMap = &snapshot->parts[idAttribute->value()];
                        for (rapidxml::xml_attribute<>* attribute = node->first_attribute();
                             attribute; attribute = attribute->next_attribute()) {
                            (*partMap)[attribute->name()] = attribute->value();
                        }
                    }
                }
            }

            rapidxml::xml_node<>* bones = canvas->first_node("bones");
            if (nullptr != bones) {
                for (rapidxml::xml_node<>* node = bones->first_node(); nullptr != node; node = node->next_sibling()) {
                    rapidxml::xml_attribute<>* idAttribute = node->first_attribute("id");
                    if (nullptr != idAttribute) {
                        std::map<std::string, std::string>* boneMap = &snapshot->bones[idAttribute->value()];
                        for (rapidxml::xml_attribute<>* attribute = node->first_attribute();
                             attribute; attribute = attribute->next_attribute()) {
                            (*boneMap)[attribute->name()] = attribute->value();
                        }
                    }
                }
            }

            rapidxml::xml_node<>* partIdList = canvas->first_node("partIdList");
            if (nullptr != partIdList) {
                for (rapidxml::xml_node<>* partId = partIdList->first_node(); nullptr != partId; partId = partId->next_sibling()) {
                    rapidxml::xml_attribute<>* idAttribute = partId->first_attribute("id");
                    if (nullptr != idAttribute) {
                        std::string componentId = to_string(Uuid::createUuid());
                        auto& component = snapshot->components[componentId];
                        component["id"] = componentId;
                        component["linkData"] = idAttribute->value();
                        component["linkDataType"] = "partId";
                        auto& childrenIds = snapshot->rootComponent["children"];
                        if (!childrenIds.empty())
                            childrenIds += ",";
                        childrenIds += componentId;
                    }
                }
            }

            rapidxml::xml_node<>* components = canvas->first_node("components");
            if (nullptr != components) {
                snapshot->rootComponent["children"] = loadComponentReturnChildrenList(snapshot, components);
            }
        }
    } catch (const std::runtime_error& e) {
        dust3dDebug << "Runtime error was: " << e.what();
    } catch (const rapidxml::parse_error& e) {
        dust3dDebug << "Parse error was: " << e.what();
    } catch (const std::exception& e) {
        dust3dDebug << "Error was: " << e.what();
    } catch (...) {
        dust3dDebug << "An unknown error occurred.";
    }
}

static void saveSnapshotComponent(const Snapshot& snapshot, std::string& xmlString, const std::string& componentId, int depth)
{
    const auto findComponent = snapshot.components.find(componentId);
    if (findComponent == snapshot.components.end())
        return;
    auto& component = findComponent->second;
    std::string prefix(depth, ' ');
    xmlString += prefix;
    xmlString += "  <component";
    std::map<std::string, std::string>::const_iterator componentAttributeIterator;
    std::string children;
    for (componentAttributeIterator = component.begin(); componentAttributeIterator != component.end(); componentAttributeIterator++) {
        if ("children" == componentAttributeIterator->first) {
            children = componentAttributeIterator->second;
            continue;
        }
        if (String::startsWith(componentAttributeIterator->first, "__"))
            continue;
        xmlString += " " + componentAttributeIterator->first + "=\"" + componentAttributeIterator->second + "\"";
    }
    xmlString += ">\n";
    if (!children.empty()) {
        for (const auto& childId : String::split(children, ',')) {
            if (childId.empty())
                continue;
            saveSnapshotComponent(snapshot, xmlString, childId, depth + 1);
        }
    }
    xmlString += prefix;
    xmlString += "  </component>\n";
}

void saveSnapshotToXmlString(const Snapshot& snapshot, std::string& xmlString)
{
    xmlString += "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";

    xmlString += "<canvas";
    std::map<std::string, std::string>::const_iterator canvasIterator;
    for (canvasIterator = snapshot.canvas.begin(); canvasIterator != snapshot.canvas.end(); canvasIterator++) {
        xmlString += " " + canvasIterator->first + "=\"" + canvasIterator->second + "\"";
    }
    xmlString += ">\n";

    xmlString += " <nodes>\n";
    std::map<std::string, std::map<std::string, std::string>>::const_iterator nodeIterator;
    for (nodeIterator = snapshot.nodes.begin(); nodeIterator != snapshot.nodes.end(); nodeIterator++) {
        std::map<std::string, std::string>::const_iterator nodeAttributeIterator;
        xmlString += "  <node";
        for (nodeAttributeIterator = nodeIterator->second.begin(); nodeAttributeIterator != nodeIterator->second.end(); nodeAttributeIterator++) {
            xmlString += " " + nodeAttributeIterator->first + "=\"" + nodeAttributeIterator->second + "\"";
        }
        xmlString += "/>\n";
    }
    xmlString += " </nodes>\n";

    xmlString += " <edges>\n";
    std::map<std::string, std::map<std::string, std::string>>::const_iterator edgeIterator;
    for (edgeIterator = snapshot.edges.begin(); edgeIterator != snapshot.edges.end(); edgeIterator++) {
        std::map<std::string, std::string>::const_iterator edgeAttributeIterator;
        xmlString += "  <edge";
        for (edgeAttributeIterator = edgeIterator->second.begin(); edgeAttributeIterator != edgeIterator->second.end(); edgeAttributeIterator++) {
            xmlString += " " + edgeAttributeIterator->first + "=\"" + edgeAttributeIterator->second + "\"";
        }
        xmlString += "/>\n";
    }
    xmlString += " </edges>\n";

    xmlString += " <parts>\n";
    std::map<std::string, std::map<std::string, std::string>>::const_iterator partIterator;
    for (partIterator = snapshot.parts.begin(); partIterator != snapshot.parts.end(); partIterator++) {
        std::map<std::string, std::string>::const_iterator partAttributeIterator;
        xmlString += "  <part";
        for (partAttributeIterator = partIterator->second.begin(); partAttributeIterator != partIterator->second.end(); partAttributeIterator++) {
            if (String::startsWith(partAttributeIterator->first, "__"))
                continue;
            xmlString += " " + partAttributeIterator->first + "=\"" + partAttributeIterator->second + "\"";
        }
        xmlString += "/>\n";
    }
    xmlString += " </parts>\n";

    if (!snapshot.bones.empty()) {
        xmlString += " <bones>\n";
        std::map<std::string, std::map<std::string, std::string>>::const_iterator boneIterator;
        for (boneIterator = snapshot.bones.begin(); boneIterator != snapshot.bones.end(); boneIterator++) {
            std::map<std::string, std::string>::const_iterator boneAttributeIterator;
            xmlString += "  <bone";
            for (boneAttributeIterator = boneIterator->second.begin(); boneAttributeIterator != boneIterator->second.end(); boneAttributeIterator++) {
                if (String::startsWith(boneAttributeIterator->first, "__"))
                    continue;
                xmlString += " " + boneAttributeIterator->first + "=\"" + boneAttributeIterator->second + "\"";
            }
            xmlString += "/>\n";
        }
        xmlString += " </bones>\n";
    }

    const auto& childrenIds = snapshot.rootComponent.find("children");
    if (childrenIds != snapshot.rootComponent.end()) {
        xmlString += " <components>\n";
        for (const auto& componentId : String::split(childrenIds->second, ',')) {
            if (componentId.empty())
                continue;
            saveSnapshotComponent(snapshot, xmlString, componentId, 0);
        }
        xmlString += " </components>\n";
    }

    xmlString += "</canvas>\n";
}

}
