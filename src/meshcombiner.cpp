#include <CGAL/Polygon_mesh_processing/corefinement.h>
#include <CGAL/Polygon_mesh_processing/repair.h>
#include <CGAL/Polygon_mesh_processing/triangulate_faces.h>
#include <QDebug>
#include <map>
#include "meshcombiner.h"
#include "positionkey.h"
#include "booleanmesh.h"

typedef CGAL::Exact_predicates_inexact_constructions_kernel CgalKernel;
typedef CGAL::Surface_mesh<CgalKernel::Point_3> CgalMesh;

MeshCombiner::Mesh::Mesh(const std::vector<QVector3D> &vertices, const std::vector<std::vector<size_t>> &faces, bool removeSelfIntersects)
{
    CgalMesh *cgalMesh = nullptr;
    if (!faces.empty()) {
        cgalMesh = buildCgalMesh<CgalKernel>(vertices, faces);
        if (!CGAL::is_valid_polygon_mesh(*cgalMesh)) {
            qDebug() << "Mesh is not valid polygon";
            delete cgalMesh;
            cgalMesh = nullptr;
        } else {
            if (CGAL::Polygon_mesh_processing::triangulate_faces(*cgalMesh)) {
                if (CGAL::Polygon_mesh_processing::does_self_intersect(*cgalMesh)) {
                    m_isSelfIntersected = true;
                    if (removeSelfIntersects) {
                        if (!CGAL::Polygon_mesh_processing::remove_self_intersections(*cgalMesh)) {
                            qDebug() << "Mesh remove_self_intersections failed";
                            delete cgalMesh;
                            cgalMesh = nullptr;
                        }
                    } else {
                        qDebug() << "Mesh does_self_intersect";
                        delete cgalMesh;
                        cgalMesh = nullptr;
                    }
                }
            } else {
                qDebug() << "Mesh triangulate failed";
                delete cgalMesh;
                cgalMesh = nullptr;
            }
        }
    }
    m_privateData = cgalMesh;
    validate();
}

MeshCombiner::Mesh::Mesh(const Mesh &other)
{
    if (other.m_privateData) {
        m_privateData = new CgalMesh(*(CgalMesh *)other.m_privateData);
        validate();
    }
}

MeshCombiner::Mesh::~Mesh()
{
    CgalMesh *cgalMesh = (CgalMesh *)m_privateData;
    delete cgalMesh;
}

void MeshCombiner::Mesh::fetch(std::vector<QVector3D> &vertices, std::vector<std::vector<size_t>> &faces) const
{
    CgalMesh *exactMesh = (CgalMesh *)m_privateData;
    if (nullptr == exactMesh)
        return;
    
    fetchFromCgalMesh<CgalKernel>(exactMesh, vertices, faces);
}

bool MeshCombiner::Mesh::isNull() const
{
    return nullptr == m_privateData;
}

bool MeshCombiner::Mesh::isSelfIntersected() const
{
    return m_isSelfIntersected;
}

MeshCombiner::Mesh *MeshCombiner::combine(const Mesh &firstMesh, const Mesh &secondMesh, Method method,
    std::vector<std::pair<Source, size_t>> *combinedVerticesComeFrom)
{
    CgalMesh *resultCgalMesh = nullptr;
    CgalMesh *firstCgalMesh = (CgalMesh *)firstMesh.m_privateData;
    CgalMesh *secondCgalMesh = (CgalMesh *)secondMesh.m_privateData;
    std::map<PositionKey, std::pair<Source, size_t>> verticesSourceMap;
    
    auto addToSourceMap = [&](CgalMesh *mesh, Source source) {
        size_t vertexIndex = 0;
        for (auto vertexIt = mesh->vertices_begin(); vertexIt != mesh->vertices_end(); vertexIt++) {
            auto point = mesh->point(*vertexIt);
            float x = (float)CGAL::to_double(point.x());
            float y = (float)CGAL::to_double(point.y());
            float z = (float)CGAL::to_double(point.z());
            auto insertResult = verticesSourceMap.insert({{x, y, z}, {source, vertexIndex}});
            //if (!insertResult.second) {
            //    qDebug() << "Position key conflict:" << QVector3D {x, y, z} << "with:" << insertResult.first->first.position();
            //}
            ++vertexIndex;
        }
    };
    if (nullptr != combinedVerticesComeFrom) {
        addToSourceMap(firstCgalMesh, Source::First);
        addToSourceMap(secondCgalMesh, Source::Second);
    }
    
    if (Method::Union == method) {
        resultCgalMesh = new CgalMesh;
        try {
            if (!CGAL::Polygon_mesh_processing::corefine_and_compute_union(*firstCgalMesh, *secondCgalMesh, *resultCgalMesh)) {
                delete resultCgalMesh;
                resultCgalMesh = nullptr;
            }
        } catch (...) {
            delete resultCgalMesh;
            resultCgalMesh = nullptr;
        }
    } else if (Method::Diff == method) {
        resultCgalMesh = new CgalMesh;
        try {
            if (!CGAL::Polygon_mesh_processing::corefine_and_compute_difference(*firstCgalMesh, *secondCgalMesh, *resultCgalMesh)) {
                delete resultCgalMesh;
                resultCgalMesh = nullptr;
            }
        } catch (...) {
            delete resultCgalMesh;
            resultCgalMesh = nullptr;
        }
    }

    if (nullptr != combinedVerticesComeFrom) {
        combinedVerticesComeFrom->clear();
        if (nullptr != resultCgalMesh) {
            for (auto vertexIt = resultCgalMesh->vertices_begin(); vertexIt != resultCgalMesh->vertices_end(); vertexIt++) {
                auto point = resultCgalMesh->point(*vertexIt);
                float x = (float)CGAL::to_double(point.x());
                float y = (float)CGAL::to_double(point.y());
                float z = (float)CGAL::to_double(point.z());
                auto findSource = verticesSourceMap.find(PositionKey(x, y, z));
                if (findSource == verticesSourceMap.end()) {
                    combinedVerticesComeFrom->push_back({Source::None, 0});
                } else {
                    combinedVerticesComeFrom->push_back(findSource->second);
                }
            }
        }
    }
    
    if (nullptr == resultCgalMesh)
        return nullptr;
    
    Mesh *mesh = new Mesh;
    mesh->m_privateData = resultCgalMesh;
    return mesh;
}

void MeshCombiner::Mesh::validate()
{
    if (nullptr == m_privateData)
        return;
    
    CgalMesh *exactMesh = (CgalMesh *)m_privateData;
    if (isNullCgalMesh<CgalKernel>(exactMesh)) {
        delete exactMesh;
        m_privateData = nullptr;
    }
}
