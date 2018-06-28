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

void IntermediateBoneRemover::markNodeAsTrivial(int partId, int nodeId)
{
    m_trivialNodeSet.insert(std::make_pair(partId, nodeId));
}

void IntermediateBoneRemover::solveFrom(int partId, int nodeId)
{
    solveMarkedFrom(partId, nodeId);
    solveTrivialFrom(partId, nodeId);
}

void IntermediateBoneRemover::solveMarkedFrom(int partId, int nodeId)
{
    std::pair<int, int> pair = std::make_pair(partId, nodeId);
    std::vector<std::pair<int, int>> trace;
    std::set<std::pair<int, int>> solvedSet;
    solveMarkedFromPairAndSaveTraceTo(pair, trace, solvedSet);
}

void IntermediateBoneRemover::solveTrivialFrom(int partId, int nodeId)
{
    std::pair<int, int> pair = std::make_pair(partId, nodeId);
    std::vector<std::pair<int, int>> trace;
    std::set<std::pair<int, int>> solvedSet;
    solveTrivialFromPairAndSaveTraceTo(pair, trace, solvedSet);
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

void IntermediateBoneRemover::addIntermediateNodesFromHistory(std::vector<std::pair<int, int>> &history, int start, std::pair<int, int> endNode, bool addChildren)
{
    IntermediateBoneNode intermediate;
    intermediate.attachedFromPartId = history[start].first;
    intermediate.attachedFromNodeId = history[start].second;
    intermediate.attachedToPartId = endNode.first;
    intermediate.attachedToNodeId = endNode.second;
    int removedNodeNum = 0;
    for (int j = start + 1; j <= (int)history.size() - 1; j++) {
        intermediate.partId = history[j].first;
        intermediate.nodeId = history[j].second;
        removedNodeNum++;
        m_intermediateNodes[std::make_pair(intermediate.partId, intermediate.nodeId)] = intermediate;
        if (addChildren)
            addNeighborsOfIntermediateNodeFrom(std::make_pair(intermediate.partId, intermediate.nodeId), intermediate);
    }
    if (removedNodeNum > 0) {
        m_newEdges.push_back(std::make_tuple(intermediate.attachedFromPartId,
            intermediate.attachedFromNodeId,
            intermediate.attachedToPartId,
            intermediate.attachedToNodeId));
    }
}

void IntermediateBoneRemover::solveMarkedFromPairAndSaveTraceTo(std::pair<int, int> node, std::vector<std::pair<int, int>> &history, std::set<std::pair<int, int>> &solvedSet)
{
    if (solvedSet.find(node) != solvedSet.end())
        return;
    solvedSet.insert(node);
    if (m_essentialNodeSet.find(node) != m_essentialNodeSet.end()) {
        for (int i = history.size() - 1; i >= 0; i--) {
            if (m_essentialNodeSet.find(history[i]) != m_essentialNodeSet.end() ||
                    m_startNodeSet.find(history[i]) != m_startNodeSet.end()) {
                bool addChildren = true;
                addIntermediateNodesFromHistory(history, i, node, addChildren);
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
        solveMarkedFromPairAndSaveTraceTo(neighbor, subHistory, solvedSet);
    }
}

void IntermediateBoneRemover::solveTrivialFromPairAndSaveTraceTo(std::pair<int, int> node, std::vector<std::pair<int, int>> &history, std::set<std::pair<int, int>> &solvedSet)
{
    if (solvedSet.find(node) != solvedSet.end())
        return;
    solvedSet.insert(node);
    if (m_trivialNodeSet.find(node) == m_trivialNodeSet.end()) {
        if (!history.empty() && m_trivialNodeSet.find(history[history.size() - 1]) != m_trivialNodeSet.end() &&
                m_intermediateNodes.find(history[history.size() - 1]) == m_intermediateNodes.end()) {
            for (int i = history.size() - 1; i >= 0; i--) {
                if (m_trivialNodeSet.find(history[i]) == m_trivialNodeSet.end()) {
                    bool addChildren = false;
                    addIntermediateNodesFromHistory(history, i, node, addChildren);
                    break;
                }
            }
        }
    }
    history.push_back(node);
    const auto &it = m_neighborMap.find(node);
    if (it == m_neighborMap.end())
        return;
    for (const auto &neighbor: it->second) {
        std::vector<std::pair<int, int>> subHistory = history;
        solveTrivialFromPairAndSaveTraceTo(neighbor, subHistory, solvedSet);
    }
}

