#include <map>
#include <QDebug>
#include <queue>
#include "meshsplitter.h"

bool MeshSplitter::split(const std::set<MeshSplitterTriangle> &input,
    const std::vector<std::pair<std::pair<size_t, size_t>, std::pair<size_t, size_t>>> &triangleLinks,
    std::set<MeshSplitterTriangle> &splitter,
    std::set<MeshSplitterTriangle> &firstGroup,
    std::set<MeshSplitterTriangle> &secondGroup,
    bool expandSplitter)
{
    firstGroup.clear();
    secondGroup.clear();
    
    // Make the edge to triangle map, this map will be used to find the neighbor triangles
    std::map<std::pair<int, int>, MeshSplitterTriangle> edgeToTriangleMap;
    for (const auto &triangle: input) {
        for (int i = 0; i < 3; i++) {
            int next = (i + 1) % 3;
            edgeToTriangleMap[std::make_pair(triangle.indices[i], triangle.indices[next])] = triangle;
        }
    }
    
    std::map<std::pair<int, int>, MeshSplitterTriangle> edgeToLinkedTriangleMap;
    for (const auto &it: triangleLinks) {
        auto firstEdge = std::make_pair((int)it.first.first, (int)it.first.second);
        auto secondEdge = std::make_pair((int)it.second.first, (int)it.second.second);
        auto findFirstTriangle = edgeToTriangleMap.find(firstEdge);
        auto findSecondTriangle = edgeToTriangleMap.find(secondEdge);
        if (findFirstTriangle == edgeToTriangleMap.end())
            continue;
        if (findSecondTriangle == edgeToTriangleMap.end())
            continue;
        edgeToLinkedTriangleMap[firstEdge] = findSecondTriangle->second;
        edgeToLinkedTriangleMap[std::make_pair(firstEdge.second, firstEdge.first)] = findSecondTriangle->second;
        edgeToLinkedTriangleMap[secondEdge] = findFirstTriangle->second;
        edgeToLinkedTriangleMap[std::make_pair(secondEdge.second, secondEdge.first)] = findFirstTriangle->second;
    }
    
    // Expand the splitter if needed
    if (expandSplitter) {
        std::vector<MeshSplitterTriangle> expandedTriangles;
        for (const auto &triangle: splitter) {
            for (int i = 0; i < 3; i++) {
                int next = (i + 1) % 3;
                auto oppositeEdge = std::make_pair(triangle.indices[next], triangle.indices[i]);
                auto oppositeTriangle = edgeToTriangleMap.find(oppositeEdge);
                if (oppositeTriangle != edgeToTriangleMap.end()) {
                    if (splitter.find(oppositeTriangle->second) == splitter.end()) {
                        expandedTriangles.push_back(oppositeTriangle->second);
                    }
                }
            }
        }
        size_t addTriangles = 0;
        for (const auto &triangle: expandedTriangles) {
            auto insertResult = splitter.insert(triangle);
            if (insertResult.second)
                ++addTriangles;
        }
        if (0 == addTriangles) {
            qDebug() << "Expanded without new triangles added";
        } else {
            qDebug() << "Expanded with new triangles added:" << addTriangles;
        }
    }
    
    // Find one triangle which is direct neighbor of one splitter
    MeshSplitterTriangle startTriangle;
    bool foundStartTriangle = false;
    for (const auto &triangle: splitter) {
        for (int i = 0; i < 3; i++) {
            int next = (i + 1) % 3;
            auto oppositeEdge = std::make_pair(triangle.indices[next], triangle.indices[i]);
            auto oppositeTriangle = edgeToTriangleMap.find(oppositeEdge);
            if (oppositeTriangle == edgeToTriangleMap.end()) {
                qDebug() << "Find opposite edge failed:" << oppositeEdge.first << oppositeEdge.second;
                continue;
            }
            if (splitter.find(oppositeTriangle->second) == splitter.end()) {
                foundStartTriangle = true;
                startTriangle = oppositeTriangle->second;
                break;
            }
        }
    }
    if (!foundStartTriangle) {
        qDebug() << "Find start triangle for splitter failed";
        return false;
    }
    
    // Recursively join all the neighbors of the first found triangle to the first group
    std::set<MeshSplitterTriangle> processedTriangles;
    for (const auto &triangle: splitter) {
        processedTriangles.insert(triangle);
    }
    std::queue<MeshSplitterTriangle> waitQueue;
    waitQueue.push(startTriangle);
    while (!waitQueue.empty()) {
        MeshSplitterTriangle triangle = waitQueue.front();
        waitQueue.pop();
        firstGroup.insert(triangle);
        if (!processedTriangles.insert(triangle).second)
            continue;
        for (int i = 0; i < 3; i++) {
            int next = (i + 1) % 3;
            auto oppositeEdge = std::make_pair(triangle.indices[next], triangle.indices[i]);
            auto oppositeTriangle = edgeToTriangleMap.find(oppositeEdge);
            if (oppositeTriangle != edgeToTriangleMap.end()) {
                if (processedTriangles.find(oppositeTriangle->second) == processedTriangles.end()) {
                    waitQueue.push(oppositeTriangle->second);
                }
            }
            auto linkedTriangle = edgeToLinkedTriangleMap.find(oppositeEdge);
            if (linkedTriangle != edgeToLinkedTriangleMap.end()) {
                if (processedTriangles.find(linkedTriangle->second) == processedTriangles.end()) {
                    waitQueue.push(linkedTriangle->second);
                }
            }
        }
    }
    
    // Now, the remains should be put in the second group
    for (const auto &triangle: input) {
        if (processedTriangles.find(triangle) != processedTriangles.end())
            continue;
        secondGroup.insert(triangle);
    }
    
    // Any of these two groups is empty means split failed
    if (firstGroup.empty() || secondGroup.empty()) {
        qDebug() << "At lease one group is empty";
        return false;
    }

    return true;
}
