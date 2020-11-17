#include "object.h"

void Object::buildInterpolatedNodes(const std::vector<ObjectNode> &nodes,
        const std::vector<std::pair<std::pair<QUuid, QUuid>, std::pair<QUuid, QUuid>>> &edges,
        std::vector<std::tuple<QVector3D, float, size_t>> *targetNodes)
{
    targetNodes->clear();
    std::map<std::pair<QUuid, QUuid>, size_t> nodeMap;
    for (size_t nodeIndex = 0; nodeIndex < nodes.size(); ++nodeIndex) {
        const auto &it = nodes[nodeIndex];
        nodeMap.insert({{it.partId, it.nodeId}, nodeIndex});
        targetNodes->push_back(std::make_tuple(it.origin, it.radius, nodeIndex));
    }
    for (const auto &it: edges) {
        auto findFirst = nodeMap.find(it.first);
        if (findFirst == nodeMap.end())
            continue;
        auto findSecond = nodeMap.find(it.second);
        if (findSecond == nodeMap.end())
            continue;
        const auto &firstNode = nodes[findFirst->second];
        const auto &secondNode = nodes[findSecond->second];
        float length = (firstNode.origin - secondNode.origin).length();
        float segments = length / 0.02;
        if (qFuzzyIsNull(segments))
            continue;
        if (segments > 100)
            segments = 100;
        float segmentLength = 1.0f / segments;
        float offset = segmentLength;
        while (offset < 1.0f) {
            float radius = firstNode.radius * (1.0f - offset) + secondNode.radius * offset;
            targetNodes->push_back(std::make_tuple(
                firstNode.origin * (1.0f - offset) + secondNode.origin * offset,
                radius,
                offset <= 0.5 ? findFirst->second : findSecond->second
            ));
            offset += segmentLength;
        }
    }
}
