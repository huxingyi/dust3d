#include <unordered_set>
#include <queue>
#include "triangleislandsresolve.h"

static void buildEdgeToFaceMap(const Outcome &outcome,
        std::map<std::pair<size_t, size_t>, size_t> &edgeToFaceMap)
{
    edgeToFaceMap.clear();
    for (size_t index = 0; index < outcome.triangles.size(); ++index) {
        const auto &indices = outcome.triangles[index];
        for (size_t i = 0; i < 3; i++) {
            size_t j = (i + 1) % 3;
            edgeToFaceMap[{indices[i], indices[j]}] = index;
        }
    }
}

void triangleIslandsResolve(const Outcome &outcome,
        const std::vector<size_t> &group,
        std::vector<std::vector<size_t>> &islands)
{
    const std::vector<std::pair<QUuid, QUuid>> *sourceNodes = outcome.triangleSourceNodes();
    if (nullptr == sourceNodes)
        return;
    std::map<std::pair<size_t, size_t>, size_t> edgeToFaceMap;
    buildEdgeToFaceMap(outcome, edgeToFaceMap);
    std::unordered_set<size_t> processedFaces;
    std::queue<size_t> waitFaces;
    for (const auto &remainingIndex: group) {
        if (processedFaces.find(remainingIndex) != processedFaces.end())
            continue;
        waitFaces.push(remainingIndex);
        std::vector<size_t> island;
        while (!waitFaces.empty()) {
            size_t index = waitFaces.front();
            waitFaces.pop();
            if (processedFaces.find(index) != processedFaces.end())
                continue;
            const auto &indices = outcome.triangles[index];
            for (size_t i = 0; i < 3; i++) {
                size_t j = (i + 1) % 3;
                auto findOppositeFaceResult = edgeToFaceMap.find({indices[j], indices[i]});
                if (findOppositeFaceResult == edgeToFaceMap.end())
                    continue;
                waitFaces.push(findOppositeFaceResult->second);
            }
            island.push_back(index);
            processedFaces.insert(index);
        }
        if (island.empty())
            continue;
        islands.push_back(island);
    }
}
