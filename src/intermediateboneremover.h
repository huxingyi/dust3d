#ifndef INTERMEDIATE_BONE_REMOVER_H
#define INTERMEDIATE_BONE_REMOVER_H
#include <vector>
#include <map>
#include <set>

struct IntermediateBoneNode
{
    int partId;
    int nodeId;
    int attachedFromPartId;
    int attachedFromNodeId;
    int attachedToPartId;
    int attachedToNodeId;
};

class IntermediateBoneRemover
{
public:
    void addEdge(int fromPartId, int fromNodeId, int toPartId, int toNodeId);
    void markNodeAsStart(int partId, int nodeId);
    void markNodeAsEssential(int partId, int nodeId);
    void markNodeAsTrivial(int partId, int nodeId);
    void solveMarkedFrom(int partId, int nodeId);
    void solveTrivialFrom(int partId, int nodeId);
    void solveFrom(int partId, int nodeId);
    const std::map<std::pair<int, int>, IntermediateBoneNode> &intermediateNodes();
    const std::vector<std::tuple<int, int, int, int>> &newEdges();
private:
    void solveMarkedFromPairAndSaveTraceTo(std::pair<int, int> node, std::vector<std::pair<int, int>> &history, std::set<std::pair<int, int>> &solvedSet);
    void solveTrivialFromPairAndSaveTraceTo(std::pair<int, int> node, std::vector<std::pair<int, int>> &history, std::set<std::pair<int, int>> &solvedSet);
    void addNeighborsOfIntermediateNodeFrom(std::pair<int, int> node, const IntermediateBoneNode &source);
    void addIntermediateNodesFromHistory(std::vector<std::pair<int, int>> &history, int start, std::pair<int, int> endNode, bool addChildren=true);
private:
    std::map<std::pair<int, int>, IntermediateBoneNode> m_intermediateNodes;
    std::set<std::pair<int, int>> m_startNodeSet;
    std::set<std::pair<int, int>> m_essentialNodeSet;
    std::set<std::pair<int, int>> m_trivialNodeSet;
    std::map<std::pair<int, int>, std::vector<std::pair<int, int>>> m_neighborMap;
    std::vector<std::tuple<int, int, int, int>> m_newEdges;
    std::set<std::pair<int, int>> m_addedSet;
};

#endif

