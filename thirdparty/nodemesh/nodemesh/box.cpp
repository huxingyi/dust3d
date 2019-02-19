#include <nodemesh/box.h>
#include <nodemesh/misc.h>
#include <nodemesh/cgalmesh.h>
#include <CGAL/Simple_cartesian.h>
#include <CGAL/subdivision_method_3.h>

typedef CGAL::Simple_cartesian<double>              SimpleKernel;
typedef CGAL::Surface_mesh<SimpleKernel::Point_3>   PolygonMesh;

namespace nodemesh
{

void box(const QVector3D position, float radius, size_t subdivideTimes, std::vector<QVector3D> &vertices, std::vector<std::vector<size_t>> &faces)
{
    std::vector<QVector3D> beginPlane = {
        {-radius, -radius,  radius},
        { radius, -radius,  radius},
        { radius,  radius,  radius},
        {-radius,  radius,  radius},
    };
    std::vector<QVector3D> endPlane = {
        beginPlane[0],
        beginPlane[3],
        beginPlane[2],
        beginPlane[1]
    };
    for (auto &vertex: endPlane) {
        vertex.setZ(vertex.z() - radius - radius);
    }
    for (const auto &vertex: beginPlane) {
        vertices.push_back(vertex);
    }
    for (const auto &vertex: endPlane) {
        vertices.push_back(vertex);
    }
    std::vector<size_t> beginLoop = {
        0, 1, 2, 3
    };
    std::vector<size_t> endLoop = {
        4, 5, 6, 7
    };
    std::vector<size_t> alignedEndLoop = {
        4, 7, 6, 5
    };
    faces.push_back(beginLoop);
    faces.push_back(endLoop);
    for (size_t i = 0; i < beginLoop.size(); ++i) {
        size_t j = (i + 1) % beginLoop.size();
        faces.push_back({
            beginLoop[j],
            beginLoop[i],
            alignedEndLoop[i],
            alignedEndLoop[j]
        });
    }
    for (auto &vertex: vertices) {
        vertex += position;
    }
    
    if (subdivideTimes > 0) {
        std::vector<std::vector<size_t>> triangles;
        triangulate(vertices, faces, triangles);
        PolygonMesh *cgalMesh = buildCgalMesh<SimpleKernel>(vertices, triangles);
        if (nullptr != cgalMesh) {
            CGAL::Subdivision_method_3::CatmullClark_subdivision(*cgalMesh, subdivideTimes);
            vertices.clear();
            faces.clear();
            fetchFromCgalMesh<SimpleKernel>(cgalMesh, vertices, faces);
            delete cgalMesh;
        }
    }
}

}
