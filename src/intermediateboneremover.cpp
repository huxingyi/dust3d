#include "intermediateboneremover.h"

void IntermediateBoneRemover::addEdge(int fromPartId, int fromNodeId, int toPartId, int toNodeId)
{
    auto &neighborMap = m_neighborMap[std::make_pair(fromPartId, fromNodeId)];
    neighborMap.push_back(std::make_pair(toPartId, toNodeId));
}

const std::map<std::pair<int, int>, IntermediateBoneNode> &IntermediateBoneRemover::intermediateNodes()
{
    return m_intermediateNodes;
}

const std::vector<std::tuple<int, int, int, int>> &IntermediateBoneRemover::newEdges()
{
    return m_newEdges;
}

void IntermediateBoneRemover::markNodeAsStart(int partId, int nodeId)
{
    m_startNodeSet.insert(std::make_pair(partId, nodeId));
}

void IntermediateBoneRemover::markNodeAsEssential(int partId, int nodeId)
{
    m_essentialNodeSet.insert(std::make_pair(partId, nodeId));
}

void IntermediateBoneRemover::solveFrom(int partId, int nodeId)
{
    std::pair<int, int> pair = std::make_pair(partId, nodeId);
    std::vector<std::pair<int, int>> trace;
    solveFromPairAndSaveTraceTo(pair, trace);
}

void IntermediateBoneRemover::addNeighborsOfIntermediateNodeFrom(std::pair<int, int> node, const IntermediateBoneNode &source)
{
    if (m_addedSet.find(node) != m_addedSet.end())
        return;
    m_addedSet.insert(node);
    if (m_essentialNodeSet.find(node) != m_essentialNodeSet.end() ||
            m_startNodeSet.find(node) != m_startNodeSet.end())
        return;
    IntermediateBoneNode intermediate = source;
    intermediate.partId = node.first;
    intermediate.nodeId = node.second;
    m_intermediateNodes[std::make_pair(intermediate.partId, intermediate.nodeId)] = intermediate;
    const auto &it = m_neighborMap.find(node);
    if (it == m_neighborMap.end())
        return;
    for (const auto &neighbor: it->second) {
        addNeighborsOfIntermediateNodeFrom(neighbor, source);
    }
}

void IntermediateBoneRemover::solveFromPairAndSaveTraceTo(std::pair<int, int> node, std::vector<std::pair<int, int>> &history)
{
    if (m_solvedSet.find(node) != m_solvedSet.end())
        return;
    m_solvedSet.insert(node);
    if (m_essentialNodeSet.find(node) != m_essentialNodeSet.end()) {
        for (int i = history.size() - 1; i >= 0; i--) {
            if (m_essentialNodeSet.find(history[i]) != m_essentialNodeSet.end() ||
                    m_startNodeSet.find(history[i]) != m_startNodeSet.end()) {
                IntermediateBoneNode intermediate;
                intermediate.attachedFromPartId = history[i].first;
                intermediate.attachedFromNodeId = history[i].second;
                intermediate.attachedToPartId = node.first;
                intermediate.attachedToNodeId = node.second;
                int removedNodeNum = 0;
                for (int j = i + 1; j <= (int)history.size() - 1; j++) {
                    intermediate.partId = history[j].first;
                    intermediate.nodeId = history[j].second;
                    removedNodeNum++;
                    m_intermediateNodes[std::make_pair(intermediate.partId, intermediate.nodeId)] = intermediate;
                    addNeighborsOfIntermediateNodeFrom(std::make_pair(intermediate.partId, intermediate.nodeId), intermediate);
                }
                if (removedNodeNum > 0) {
                    m_newEdges.push_back(std::make_tuple(intermediate.attachedFromPartId,
                        intermediate.attachedFromNodeId,
                        intermediate.attachedToPartId,
                        intermediate.attachedToNodeId));
                }
                break;
            }
        }
    }
    history.push_back(node);
    const auto &it = m_neighborMap.find(node);
    if (it == m_neighborMap.end())
        return;
    for (const auto &neighbor: it->second) {
        std::vector<std::pair<int, int>> subHistory = history;
        solveFromPairAndSaveTraceTo(neighbor, subHistory);
    }
}

