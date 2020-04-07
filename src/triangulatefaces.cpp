#include <QDebug>
#include <CGAL/Exact_predicates_inexact_constructions_kernel.h>
#include <CGAL/Polygon_mesh_processing/triangulate_faces.h>
#include <QVector2D>
#include <QVector3D>
#include "booleanmesh.h"
#include "triangulatefaces.h"
#include "util.h"

typedef CGAL::Exact_predicates_inexact_constructions_kernel InexactKernel;
typedef CGAL::Surface_mesh<InexactKernel::Point_3> InexactMesh;

bool triangulateFacesWithoutKeepVertices(std::vector<QVector3D> &vertices, const std::vector<std::vector<size_t>> &faces, std::vector<std::vector<size_t>> &triangles)
{
    auto cgalMesh = buildCgalMesh<InexactKernel>(vertices, faces);
    bool isSuccessful = CGAL::Polygon_mesh_processing::triangulate_faces(*cgalMesh);
    if (isSuccessful) {
        vertices.clear();
        fetchFromCgalMesh<InexactKernel>(cgalMesh, vertices, triangles);
        delete cgalMesh;
        return true;
    }
    delete cgalMesh;
    
    // fallback to our own imeplementation

    isSuccessful = true;
    std::vector<std::vector<size_t>> rings;
    for (const auto &face: faces) {
        if (face.size() > 3) {
            rings.push_back(face);
        } else {
            triangles.push_back(face);
        }
    }
    for (const auto &ring: rings) {
        std::vector<size_t> fillRing = ring;
        QVector3D direct = polygonNormal(vertices, fillRing);
        while (fillRing.size() > 3) {
            bool newFaceGenerated = false;
            for (decltype(fillRing.size()) i = 0; i < fillRing.size(); ++i) {
                auto j = (i + 1) % fillRing.size();
                auto k = (i + 2) % fillRing.size();
                const auto &enter = vertices[fillRing[i]];
                const auto &cone = vertices[fillRing[j]];
                const auto &leave = vertices[fillRing[k]];
                auto angle = angleInRangle360BetweenTwoVectors(cone - enter, leave - cone, direct);
                if (angle >= 1.0 && angle <= 179.0) {
                    bool isEar = true;
                    for (size_t x = 0; x < fillRing.size() - 3; ++x) {
                        auto fourth = vertices[(i + 3 + k) % fillRing.size()];
                        if (pointInTriangle(enter, cone, leave, fourth)) {
                            isEar = false;
                            break;
                        }
                    }
                    if (isEar) {
                        std::vector<size_t> newFace = {
                            fillRing[i],
                            fillRing[j],
                            fillRing[k],
                        };
                        triangles.push_back(newFace);
                        fillRing.erase(fillRing.begin() + j);
                        newFaceGenerated = true;
                        break;
                    }
                }
            }
            if (!newFaceGenerated)
                break;
        }
        if (fillRing.size() == 3) {
            std::vector<size_t> newFace = {
                fillRing[0],
                fillRing[1],
                fillRing[2],
            };
            triangles.push_back(newFace);
        } else {
            qDebug() << "Triangulate failed, ring size:" << fillRing.size();
            isSuccessful = false;
        }
    }
    return isSuccessful;
}
