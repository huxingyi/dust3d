#ifndef DUST3D_SHORTEST_PATH
#define DUST3D_SHORTEST_PATH
#include <vector>
#include <tuple>

/*

Example code:

    std::vector<QString> nodeNames = {
        "A", //0
        "B", //1
        "C", //2
        "D", //3
        "E", //4
        "F"  //5
    };
    std::vector<std::pair<int, int>> edges = {
        {1,0},
        {1,3},
        {1,4},
        {3,4},
        {3,5},
        {4,5},
        {5,2},
        {0,2}
    };
    std::vector<int> weights(edges.size(), 1);
    weights[7] = 10;
    weights[4] = 10;
    std::vector<int> path;
    shortestPath(nodeNames.size(), edges, weights, 0, 5, &path);
    for (const auto &it: path) {
        qDebug() << nodeNames[it];
    }

*/

bool shortestPath(size_t nodeNum,
    const std::vector<std::pair<size_t, size_t>> &edges,
    const std::vector<int> &weights,
    size_t start,
    size_t stop,
    std::vector<size_t> *path);

#endif
