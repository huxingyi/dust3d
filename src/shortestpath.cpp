#include <boost/config.hpp>
#include <iostream>
#include <fstream>
#include <boost/graph/graph_traits.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/dijkstra_shortest_paths.hpp>
#include <QDebug>
#include "shortestpath.h"

using namespace boost;

bool shortestPath(size_t nodeNum,
    const std::vector<std::pair<size_t, size_t>> &edges,
    const std::vector<int> &weights,
    size_t start,
    size_t stop,
    std::vector<size_t> *path)
{
    typedef adjacency_list < listS, vecS, undirectedS,
        no_property, property < edge_weight_t, int > > graph_t;
    typedef graph_traits < graph_t >::vertex_descriptor vertex_descriptor;
    typedef graph_traits < graph_t >::edge_descriptor edge_descriptor;
    typedef std::pair<size_t, size_t> Edge;
    
    size_t edgeNum = edges.size();
#if defined(BOOST_MSVC) && BOOST_MSVC <= 1300
    graph_t g(nodeNum);
    property_map<graph_t, edge_weight_t>::type weightmap = get(edge_weight, g);
    for (std::size_t j = 0; j < edgeNum; ++j) {
        edge_descriptor e; bool inserted;
        tie(e, inserted) = add_edge(edges[j].first, edges[j].second, g);
        weightmap[e] = weights[j];
    }
#else
    graph_t g(edges.data(), edges.data() + edgeNum, weights.data(), nodeNum);
    property_map<graph_t, edge_weight_t>::type weightmap = get(edge_weight, g);
#endif

    std::vector<vertex_descriptor> p(num_vertices(g));
    std::vector<size_t> d(num_vertices(g));
    vertex_descriptor s = vertex(start, g);

#if defined(BOOST_MSVC) && BOOST_MSVC <= 1300
    // VC++ has trouble with the named parameters mechanism
    property_map<graph_t, vertex_index_t>::type indexmap = get(vertex_index, g);
    dijkstra_shortest_paths(g, s, &p[0], &d[0], weightmap, indexmap,
                          std::less<size_t>(), closed_plus<size_t>(),
                          (std::numeric_limits<size_t>::max)(), 0,
                          default_dijkstra_visitor());
#else
    dijkstra_shortest_paths(g, s, predecessor_map(&p[0]).distance_map(&d[0]));
#endif

    auto current = stop;
    while (current != start) {
        path->push_back(current);
        size_t next = p[current];
        if (next == current)
            return false;
        current = next;
    }
    path->push_back(current);
    
    return true;
}
