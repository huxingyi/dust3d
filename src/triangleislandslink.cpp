#include <CGAL/convex_hull_3.h>
#include <CGAL/Polygon_mesh_processing/corefinement.h>
#include <QVector3D>
#include <vector>
#include <map>
#include <cmath>
#include <unordered_set>
#include <QString>
#include "triangleislandslink.h"
#include "triangleislandsresolve.h"
#include "booleanmesh.h"

template <class InputIterator, class Kernel>
bool precheckForConvexHull(InputIterator first, InputIterator beyond)
{
    typedef typename CGAL::internal::Convex_hull_3::Default_traits_for_Chull_3<typename Kernel::Point_3, typename CGAL::Surface_mesh<typename Kernel::Point_3>>::type Traits;
    typedef typename Traits::Point_3                Point_3;
    typedef std::list<Point_3>                      Point_3_list;
    typedef typename Point_3_list::iterator         P3_iterator;
    Traits traits;

    Point_3_list points(first, beyond);
    if (!(points.size() > 3))
        return false;

    // at least 4 points
    typename Traits::Collinear_3 collinear = traits.collinear_3_object();
    typename Traits::Equal_3 equal = traits.equal_3_object();

    P3_iterator point1_it = points.begin();
    P3_iterator point2_it = points.begin();
    point2_it++;

    // find three that are not collinear
    while (point2_it != points.end() && equal(*point1_it,*point2_it))
        ++point2_it;

    if (!(point2_it != points.end()))
        return false;

    P3_iterator point3_it = point2_it;
    ++point3_it;

    if (!(point3_it != points.end()))
        return false;

    while (point3_it != points.end() && collinear(*point1_it,*point2_it,*point3_it))
        ++point3_it;

    if (!(point3_it != points.end()))
        return false;

    return true;
}

template <class Kernel>
typename CGAL::Surface_mesh<typename Kernel::Point_3> *buildConvexCgalMesh(const std::vector<QVector3D> &positions, const std::vector<std::vector<size_t>> &indices)
{
    typename CGAL::Surface_mesh<typename Kernel::Point_3> *mesh = new typename CGAL::Surface_mesh<typename Kernel::Point_3>;
    std::vector<typename Kernel::Point_3> points;
    std::unordered_set<size_t> addedIndices;
    for (const auto &face: indices) {
        for (const auto &index: face) {
            if (addedIndices.find(index) != addedIndices.end())
                continue;
            addedIndices.insert(index);
            const auto &position = positions[index];
            points.push_back(typename Kernel::Point_3(position.x(), position.y(), position.z()));
        }
    }
    try {
        if (precheckForConvexHull<typename std::vector<typename Kernel::Point_3>::iterator, Kernel>(points.begin(), points.end())) {
            CGAL::convex_hull_3(points.begin(), points.end(), *mesh);
        } else {
            delete mesh;
            return nullptr;
        }
    } catch (...) {
        delete mesh;
        return nullptr;
    }
    return mesh;
}

static CgalMesh *doIntersection(CgalMesh *firstCgalMesh, CgalMesh *secondCgalMesh)
{
    if (nullptr == firstCgalMesh || nullptr == secondCgalMesh)
        return nullptr;
    CgalMesh *resultCgalMesh = new CgalMesh;
    try {
        if (!CGAL::Polygon_mesh_processing::corefine_and_compute_intersection(*firstCgalMesh, *secondCgalMesh, *resultCgalMesh)) {
            delete resultCgalMesh;
            resultCgalMesh = nullptr;
        }
    } catch (...) {
        delete resultCgalMesh;
        resultCgalMesh = nullptr;
    }
    return resultCgalMesh;
}

static CgalMesh *doUnion(CgalMesh *firstCgalMesh, CgalMesh *secondCgalMesh)
{
    if (nullptr == firstCgalMesh || nullptr == secondCgalMesh)
        return nullptr;
    CgalMesh *resultCgalMesh = new CgalMesh;
    try {
        if (!CGAL::Polygon_mesh_processing::corefine_and_compute_union(*firstCgalMesh, *secondCgalMesh, *resultCgalMesh)) {
            delete resultCgalMesh;
            resultCgalMesh = nullptr;
        }
    } catch (...) {
        delete resultCgalMesh;
        resultCgalMesh = nullptr;
    }
    return resultCgalMesh;
}

static bool fetchCgalMeshCenter(CgalMesh *cgalMesh, QVector3D &center)
{
    if (nullptr == cgalMesh)
        return false;
    
    std::vector<QVector3D> vertices;
    std::vector<std::vector<size_t>> faces;
    fetchFromCgalMesh<CgalKernel>(cgalMesh, vertices, faces);
    if (vertices.empty() || faces.empty())
        return false;
    
    QVector3D sumOfPositions;
    for (const auto &face: faces) {
        QVector3D sumOfTrianglePositions;
        for (const auto &vertex: face) {
            sumOfTrianglePositions += vertices[vertex];
        }
        sumOfPositions += sumOfTrianglePositions / face.size();
    }
    center = sumOfPositions / faces.size();
    
    return true;
}

static std::pair<size_t, size_t> findNearestTriangleEdge(const Outcome &outcome,
        const std::vector<size_t> &group,
        const QVector3D &point)
{
    float minLength2 = std::numeric_limits<float>::max();
    std::pair<size_t, size_t> choosenEdge = std::make_pair(0, 0);
    for (const auto &triangleIndex: group) {
        const auto &indices = outcome.triangles[triangleIndex];
        for (size_t i = 0; i < indices.size(); ++i) {
            size_t j = (i + 1) % indices.size();
            QVector3D edgeMiddle = (outcome.vertices[indices[i]] + outcome.vertices[indices[j]]) / 2;
            float length2 = (point - edgeMiddle).lengthSquared();
            if (length2 < minLength2) {
                minLength2 = length2;
                choosenEdge = std::make_pair(indices[i], indices[j]);
            }
        }
    }
    return choosenEdge;
}

static bool mergeIntersectedConvexMeshesAndLinkTriangles(const Outcome &outcome,
        std::map<QString, std::pair<CgalMesh *, std::vector<size_t>>> &convexMeshes,
        std::vector<std::pair<std::pair<size_t, size_t>, std::pair<size_t, size_t>>> &links)
{
    if (convexMeshes.size() <= 1)
        return false;
    auto head = *convexMeshes.begin();
    for (const auto &it: convexMeshes) {
        if (it.first == head.first)
            continue;
        CgalMesh *intersectionMesh = doIntersection(head.second.first, it.second.first);
        QVector3D center;
        bool fetchSuccess = fetchCgalMeshCenter(intersectionMesh, center);
        delete intersectionMesh;
        if (fetchSuccess) {
            QString firstGroupName = head.first;
            QString secondGroupName = it.first;
            std::vector<size_t> firstGroupTriangleIndices = head.second.second;
            std::vector<size_t> secondGroupTriangleIndices = it.second.second;
            std::pair<size_t, size_t> firstGroupChoosenIndex = findNearestTriangleEdge(outcome, firstGroupTriangleIndices, center);
            std::pair<size_t, size_t> secondGroupChoosenIndex = findNearestTriangleEdge(outcome, secondGroupTriangleIndices, center);
            links.push_back(std::make_pair(firstGroupChoosenIndex, secondGroupChoosenIndex));
            std::vector<size_t> triangleIndices(firstGroupTriangleIndices);
            triangleIndices.insert(triangleIndices.end(), secondGroupTriangleIndices.begin(), secondGroupTriangleIndices.end());
            CgalMesh *unionMesh = doUnion(head.second.first, it.second.first);
            delete head.second.first;
            delete it.second.first;
            convexMeshes.erase(firstGroupName);
            convexMeshes.erase(secondGroupName);
            convexMeshes[firstGroupName + "+" + secondGroupName] = std::make_pair(
                unionMesh,
                triangleIndices);
            return true;
        }
    }
    return false;
}

void triangleIslandsLink(const Outcome &outcome,
        std::vector<std::pair<std::pair<size_t, size_t>, std::pair<size_t, size_t>>> &links)
{
    std::vector<size_t> group;
    std::vector<std::vector<size_t>> islands;
    size_t triangleCount = outcome.triangles.size();
    for (size_t i = 0; i < triangleCount; ++i)
        group.push_back(i);
    triangleIslandsResolve(outcome, group, islands);
    if (islands.size() <= 2)
        return;
    const std::vector<QVector3D> &positions = outcome.vertices;
    std::map<QString, std::pair<CgalMesh *, std::vector<size_t>>> convexMeshes;
    for (size_t islandIndex = 0; islandIndex < islands.size(); ++islandIndex) {
        const auto &island = islands[islandIndex];
        std::vector<std::vector<size_t>> indices;
        for (const auto &triangleIndex: island) {
            indices.push_back(outcome.triangles[triangleIndex]);
        }
        convexMeshes[QString::number(islandIndex)] = std::make_pair(
            buildConvexCgalMesh<CgalKernel>(positions, indices),
            island);
    }
    while (convexMeshes.size() >= 2) {
        if (!mergeIntersectedConvexMeshesAndLinkTriangles(outcome,
                convexMeshes, links))
            break;
    }
    for (auto &it: convexMeshes) {
        delete it.second.first;
    }
}
