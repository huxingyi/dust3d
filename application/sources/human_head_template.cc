/*
 *  Copyright (c) 2016-2026 Jeremy HU <jeremy-at-dust3d dot org>. All rights reserved.
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

#include "human_head_template.h"
#include "glb_forever.h"
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QVector3D>
#include <cmath>
#include <dust3d/base/string.h>
#include <functional>
#include <vector>

namespace {

QVector3D vectorOf(const QJsonValue& value)
{
    QJsonArray array = value.toArray();
    return QVector3D((float)array.at(0).toDouble(), (float)array.at(1).toDouble(), (float)array.at(2).toDouble());
}

}

bool makeHumanHeadSnapshot(dust3d::Snapshot* snapshot, float originX, float originY, float originZ,
    float x, float y, float z, float height)
{
    QFile glbFile(":/resources/human_head.glb");
    QFile marksFile(":/resources/human_head_landmarks.json");
    if (!glbFile.open(QIODevice::ReadOnly) || !marksFile.open(QIODevice::ReadOnly))
        return false;
    QByteArray glbData = glbFile.readAll();
    QJsonObject marks = QJsonDocument::fromJson(marksFile.readAll()).object();
    QJsonArray bounds = marks.value("bounds").toArray();
    QJsonArray landmarks = marks.value("landmarks68").toArray();
    QJsonArray eyeballs = marks.value("eyeballs").toArray();
    if (bounds.size() != 2 || landmarks.size() != 68 || eyeballs.size() != 2)
        return false;
    dust3d::Uuid glbId = GlbForever::add(&glbData);
    if (glbId.isNull())
        return false;

    // Where the model goes: the chin `height` below the crown, the middle of it at (x, y, z).
    QVector3D lower = vectorOf(bounds.at(0)), upper = vectorOf(bounds.at(1));
    QVector3D centre = (lower + upper) * 0.5f;
    const float chinY = vectorOf(landmarks.at(8)).y();
    const float scale = height / std::max(1e-6f, upper.y() - chinY);
    const float bottom = y - (upper.y() - lower.y()) * scale * 0.5f;
    const float crown = y + (upper.y() - lower.y()) * scale * 0.5f;
    auto world = [&](const QVector3D& p) {
        return QVector3D(x + (p.x() - centre.x()) * scale, bottom + (p.y() - lower.y()) * scale, z + (p.z() - centre.z()) * scale);
    };
    // (the strokes were sized on a head this many world units per centimetre)
    const float k = scale / 0.0053f;

    std::vector<std::string> children;
    auto addPart = [&](const std::string& name, const std::vector<std::pair<QVector3D, float>>& chain, const std::string& color,
                       const std::map<std::string, std::string>& attributes) {
        std::string partId = dust3d::Uuid::createUuid().toString();
        std::string componentId = dust3d::Uuid::createUuid().toString();
        auto& part = snapshot->parts[partId];
        part["id"] = partId;
        part["visible"] = "true";
        part["subdived"] = "false";
        part["rounded"] = "false";
        part["xMirrored"] = "false";
        for (const auto& it : attributes)
            part[it.first] = it.second;
        std::string previous;
        for (const auto& node : chain) {
            std::string nodeId = dust3d::Uuid::createUuid().toString();
            auto& n = snapshot->nodes[nodeId];
            n["id"] = nodeId;
            n["partId"] = partId;
            n["radius"] = std::to_string(node.second);
            n["x"] = std::to_string(originX + node.first.x());
            n["y"] = std::to_string(originY - node.first.y());
            n["z"] = std::to_string(originZ - node.first.z());
            if (!previous.empty()) {
                std::string edgeId = dust3d::Uuid::createUuid().toString();
                auto& e = snapshot->edges[edgeId];
                e["id"] = edgeId;
                e["partId"] = partId;
                e["from"] = previous;
                e["to"] = nodeId;
            }
            previous = nodeId;
        }
        auto& component = snapshot->components[componentId];
        component["id"] = componentId;
        component["name"] = name;
        component["linkData"] = partId;
        component["linkDataType"] = "partId";
        component["color"] = color;
        component["combineMode"] = "Normal";
        children.push_back(componentId);
    };
    auto paint = [&](const std::string& name, const std::vector<std::pair<QVector3D, float>>& chain, const std::string& color, float softness) {
        addPart(name, chain, color, { { "target", "Paint" }, { "rounded", "true" }, { "paintSoftness", std::to_string(softness) } });
    };

    // the head
    const float radius = std::max(upper.x() - lower.x(), upper.z() - lower.z()) * 0.5f * scale;
    addPart("Skull", { { QVector3D(x, bottom, z), radius }, { QVector3D(x, crown, z), radius } }, "#d7a88a",
        { { "target", "ImportedModel" }, { "importedModelId", glbId.toString() }, { "textureDetail", "2" } });

    // the eyes: the white, the iris a little up, the pupil
    const char* sides[2] = { "Left", "Right" };
    for (int i = 0; i < 2; ++i) {
        QJsonObject eye = eyeballs.at(i).toObject();
        QVector3D c = world(vectorOf(eye.value("centre")));
        float r = (float)eye.value("radius").toDouble() * scale;
        QVector3D front = c + QVector3D(0.0f, 0.08f, 1.0f).normalized() * r;
        paint(std::string(sides[i]) + " Eye", { { c, r * 0.995f } }, "#ece2da", 0.0f);
        paint(std::string(sides[i]) + " Iris", { { front, r * 0.40f } }, "#3b2a20", 0.3f);
        paint(std::string(sides[i]) + " Pupil", { { front, r * 0.15f } }, "#1d1a19", 0.4f);
    }
    auto mark = [&](int index) { return world(vectorOf(landmarks.at(index))); };
    // a dark line along each upper lid
    const int lids[2][4] = { { 42, 43, 44, 45 }, { 36, 37, 38, 39 } };
    const float lidWidths[4] = { 0.0006f, 0.0011f, 0.0011f, 0.0005f };
    for (int i = 0; i < 2; ++i) {
        std::vector<std::pair<QVector3D, float>> chain;
        for (int j = 0; j < 4; ++j)
            chain.push_back({ mark(lids[i][j]) + QVector3D(0.0f, 0.0004f, 0.0004f) * k, lidWidths[j] * k });
        paint(std::string(sides[i]) + " Lid", chain, "#2a2422", 0.55f);
    }
    // the brows, along the brow landmarks from the nose out
    const int brows[2][5] = { { 22, 23, 24, 25, 26 }, { 21, 20, 19, 18, 17 } };
    const float browWidths[5] = { 0.0011f, 0.0017f, 0.0021f, 0.0023f, 0.0022f };
    for (int i = 0; i < 2; ++i) {
        std::vector<std::pair<QVector3D, float>> chain;
        for (int j = 0; j < 5; ++j)
            chain.push_back({ mark(brows[i][j]) + QVector3D(0.0f, 0.0f, 0.0012f) * k, browWidths[j] * k });
        paint(std::string(sides[i]) + " Brow", chain, "#3a2a22", 0.5f);
    }
    // a little colour in the lips
    std::vector<std::pair<QVector3D, float>> upperLip, lowerLip;
    for (int index : { 48, 50, 51, 52, 54 })
        upperLip.push_back({ mark(index) + QVector3D(0.0f, -0.0008f, 0.0005f) * k, 0.0016f * k });
    for (int index : { 54, 56, 57, 58, 48 })
        lowerLip.push_back({ mark(index) + QVector3D(0.0f, 0.0012f, 0.0005f) * k, 0.0019f * k });
    paint("Upper Lip", upperLip, "#c0806e", 0.6f);
    paint("Lower Lip", lowerLip, "#c0806e", 0.6f);

    // all of it one group: the face is painted only on the head
    std::string groupId = dust3d::Uuid::createUuid().toString();
    auto& group = snapshot->components[groupId];
    group["id"] = groupId;
    group["name"] = "Human Head";
    group["expanded"] = "false";
    group["combineMode"] = "Normal";
    group["children"] = dust3d::String::join(children, ",");
    std::string rootChildren = dust3d::String::valueOrEmpty(snapshot->rootComponent, "children");
    snapshot->rootComponent["children"] = rootChildren.empty() ? groupId : rootChildren + "," + groupId;
    return true;
}
