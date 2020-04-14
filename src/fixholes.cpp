#include <unordered_set>
#include <set>
#include <QDebug>
#include <simpleuv/triangulate.h>
#include "fixholes.h"
#include "util.h"

static void buildEdgeToFaceMap(const std::vector<std::vector<size_t>> &faces, std::map<std::pair<size_t, size_t>, size_t> &edgeToFaceMap)
{
    edgeToFaceMap.clear();
    for (decltype(faces.size()) index = 0; index < faces.size(); ++index) {
        const auto &face = faces[index];
        for (size_t i = 0; i < face.size(); i++) {
            size_t j = (i + 1) % face.size();
            edgeToFaceMap[{face[i], face[j]}] = index;
        }
    }
}

void fixHoles(const std::vector<QVector3D> &verticies, std::vector<std::vector<size_t>> &faces)
{
    std::map<std::pair<size_t, size_t>, size_t> edgeToFaceMap;
    buildEdgeToFaceMap(faces, edgeToFaceMap);
    
    std::map<size_t, std::vector<size_t>> holeVertexLink;
    std::vector<size_t> startVertices;
    for (const auto &face: faces) {
        for (size_t i = 0; i < face.size(); i++) {
            size_t j = (i + 1) % face.size();
            auto findOppositeFaceResult = edgeToFaceMap.find({face[j], face[i]});
            if (findOppositeFaceResult != edgeToFaceMap.end())
                continue;
            holeVertexLink[face[j]].push_back(face[i]);
            startVertices.push_back(face[j]);
        }
    }
    
    std::vector<std::pair<std::vector<size_t>, double>> holeRings;
    auto findRing = [&](size_t startVertex) {
        while (true) {
            bool foundRing = false;
            std::vector<size_t> ring;
            std::unordered_set<size_t> visited;
            std::set<std::pair<size_t, size_t>> visitedPath;
            double ringLength = 0;
            while (!foundRing) {
                ring.clear();
                visited.clear();
                //ringLength = 0;
                auto first = startVertex;
                auto index = first;
                auto prev = first;
                ring.push_back(first);
                visited.insert(first);
                while (true) {
                    auto findLinkResult = holeVertexLink.find(index);
                    if (findLinkResult == holeVertexLink.end()) {
                        //if (index != first)
                        //    qDebug() << "fixHoles Search ring failed, index:" << index << "first:" << first;
                        return false;
                    }
                    for (const auto &item: findLinkResult->second) {
                        if (item == first) {
                            foundRing = true;
                            break;
                        }
                    }
                    if (foundRing)
                        break;
                    if (findLinkResult->second.size() > 1) {
                        bool foundNewPath = false;
                        for (const auto &item: findLinkResult->second) {
                            if (visitedPath.find({prev, item}) == visitedPath.end()) {
                                index = item;
                                foundNewPath = true;
                                break;
                            }
                        }
                        if (!foundNewPath) {
                            //qDebug() << "fixHoles No new path to try";
                            return false;
                        }
                        visitedPath.insert({prev, index});
                    } else {
                        index = *findLinkResult->second.begin();
                    }
                    if (visited.find(index) != visited.end()) {
                        while (index != *ring.begin())
                            ring.erase(ring.begin());
                        foundRing = true;
                        break;
                    }
                    ring.push_back(index);
                    visited.insert(index);
                    //ringLength += (verticies[index] - verticies[prev]).length();
                    prev = index;
                }
            }
            if (ring.size() < 3) {
                //qDebug() << "fixHoles Ring too short, size:" << ring.size();
                return false;
            }
            holeRings.push_back({ring, ringLength});
            for (size_t i = 0; i < ring.size(); ++i) {
                size_t j = (i + 1) % ring.size();
                auto findLinkResult = holeVertexLink.find(ring[i]);
                for (auto it = findLinkResult->second.begin(); it != findLinkResult->second.end(); ++it) {
                    if (*it == ring[j]) {
                        findLinkResult->second.erase(it);
                        if (findLinkResult->second.empty())
                            holeVertexLink.erase(ring[i]);
                        break;
                    }
                }
            }
            return true;
        }
        return false;
    };
    for (const auto &startVertex: startVertices)
        findRing(startVertex);
    
    std::vector<simpleuv::Vertex> simpleuvVertices(verticies.size());
    for (size_t i = 0; i < simpleuvVertices.size(); ++i) {
        const auto &src = verticies[i];
        simpleuvVertices[i] = simpleuv::Vertex {{src.x(), src.y(), src.z()}};
    }
    std::vector<simpleuv::Face> newFaces;
    for (size_t i = 0; i < holeRings.size(); ++i) {
        simpleuv::triangulate(simpleuvVertices, newFaces, holeRings[i].first);
    }
    
    //saveAsObj("fixholes_input.obj", verticies, faces);
    //std::vector<std::vector<size_t>> addedFaces;
    for (const auto &it: newFaces) {
        faces.push_back(std::vector<size_t> {it.indices[0], it.indices[1], it.indices[2]});
        //addedFaces.push_back(std::vector<size_t> {it.indices[0], it.indices[1], it.indices[2]});
    }
    //saveAsObj("fixholes_output.obj", verticies, faces);
    //saveAsObj("fixholes_added.obj", verticies, addedFaces);
    
    qDebug() << "fixHoles holeRings:" << holeRings.size() << "newFaces:" << newFaces.size();
}
