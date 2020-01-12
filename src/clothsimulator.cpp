#include <MassSpringSolver.h>
#include <set>
#include <CGAL/Simple_cartesian.h>
#include <CGAL/AABB_tree.h>
#include <CGAL/AABB_traits.h>
#include <CGAL/Polyhedron_3.h>
#include <CGAL/Polyhedron_incremental_builder_3.h>
#include <CGAL/boost/graph/graph_traits_Polyhedron_3.h>
#include <CGAL/AABB_face_graph_triangle_primitive.h>
#include <CGAL/algorithm.h>
#include <CGAL/Side_of_triangle_mesh.h>
#include "clothsimulator.h"
#include "booleanmesh.h"

typedef CGAL::Simple_cartesian<double> K;
typedef K::Point_3 Point;
typedef K::Triangle_3 Triangle;
typedef CGAL::Polyhedron_3<K> Polyhedron;
typedef Polyhedron::HalfedgeDS HalfedgeDS;
typedef CGAL::AABB_face_graph_triangle_primitive<Polyhedron> Primitive;
typedef CGAL::AABB_traits<K, Primitive> Traits;
typedef CGAL::AABB_tree<Traits> Tree;
typedef CGAL::Side_of_triangle_mesh<Polyhedron, K> Point_inside;

template <class HDS>
class Build_mesh : public CGAL::Modifier_base<HDS> {
public:
    Build_mesh(const std::vector<QVector3D> *vertices,
            const std::vector<std::vector<size_t>> *faces) :
        m_vertices(vertices),
        m_faces(faces)
    {
    };
    void operator()(HDS& hds)
    {
        // Postcondition: hds is a valid polyhedral surface.
        CGAL::Polyhedron_incremental_builder_3<HDS> B(hds, false);
        B.begin_surface(m_vertices->size(), m_faces->size());
        typedef typename HDS::Vertex   Vertex;
        typedef typename Vertex::Point Point;
        for (const auto &it: *m_vertices)
            B.add_vertex(Point(it.x(), it.y(), it.z()));
        for (const auto &it: *m_faces) {
            B.begin_facet();
            B.add_vertex_to_facet(it[0]);
            B.add_vertex_to_facet(it[1]);
            B.add_vertex_to_facet(it[2]);
            B.end_facet();
        }
        B.end_surface();
    };
private:
    const std::vector<QVector3D> *m_vertices = nullptr;
    const std::vector<std::vector<size_t>> *m_faces = nullptr;
};

// System parameters
//namespace SystemParam {
//    static const int n = 61; // must be odd, n * n = n_vertices | 61
//    static const float h = 0.001f; // time step, smaller for better results | 0.008f = 0.016f/2
//    static const float m = 0.25f / (n * n); // point mass | 0.25f
//    static const float a = 0.993f; // damping, close to 1.0 | 0.993f
//    static const float g = 9.8f * m; // gravitational force | 9.8f
//}

// Point - mesh collision node
class CgMeshCollisionNode : public CgPointNode {
private:
    Tree *m_aabbTree = nullptr;
    Point_inside *m_insideTester = nullptr;
    Polyhedron m_polyhedron;
public:
    CgMeshCollisionNode(mass_spring_system *system, float *vbuff,
            const std::vector<QVector3D> &collisionVertices,
            const std::vector<std::vector<size_t>> &collisionTriangles) :
        CgPointNode(system, vbuff)
    {
        if (!collisionTriangles.empty()) {
            Build_mesh<HalfedgeDS> mesh(&collisionVertices, &collisionTriangles);
            m_polyhedron.delegate(mesh);
            m_aabbTree = new Tree(faces(m_polyhedron).first, faces(m_polyhedron).second, m_polyhedron);
            m_aabbTree->accelerate_distance_queries();
            m_insideTester = new Point_inside(*m_aabbTree);
        }
    }
    
    ~CgMeshCollisionNode()
    {
        delete m_insideTester;
        delete m_aabbTree;
    };
    
    bool query(unsigned int i) const
    {
        return false;
    };
    
    void satisfy()
    {
        for (unsigned int i = 0; i < system->n_points; i++) {
            auto offset = 3 * i;
            Point point(vbuff[offset + 0],
                vbuff[offset + 1],
                vbuff[offset + 2]);
            if (nullptr != m_insideTester &&
                    (*m_insideTester)(point) != CGAL::ON_UNBOUNDED_SIDE) {
                Point closestPoint = m_aabbTree->closest_point(point);
                vbuff[offset + 0] = closestPoint.x();
                vbuff[offset + 1] = closestPoint.y();
                vbuff[offset + 2] = closestPoint.z();
            }
        }
    }
    
    void fixPoints(CgPointFixNode *fixNode)
    {
        for (unsigned int i = 0; i < system->n_points; i++) {
            auto offset = 3 * i;
            Point point(vbuff[offset + 0],
                vbuff[offset + 1],
                vbuff[offset + 2]);
            if (nullptr != m_insideTester &&
                    (*m_insideTester)(point) != CGAL::ON_UNBOUNDED_SIDE) {
                fixNode->fixPoint(i);
            }
        }
    }
    
    void collectErrorPoints(std::vector<size_t> *points)
    {
        for (unsigned int i = 0; i < system->n_points; i++) {
            auto offset = 3 * i;
            Point point(vbuff[offset + 0],
                vbuff[offset + 1],
                vbuff[offset + 2]);
            if (nullptr != m_insideTester &&
                    (*m_insideTester)(point) != CGAL::ON_UNBOUNDED_SIDE) {
                points->push_back(i);
            }
        }
    }
};

ClothSimulator::ClothSimulator(const std::vector<QVector3D> &vertices,
        const std::vector<std::vector<size_t>> &faces,
        const std::vector<QVector3D> &collisionVertices,
        const std::vector<std::vector<size_t>> &collisionTriangles,
        const std::vector<QVector3D> &externalForces) :
    m_vertices(vertices),
    m_faces(faces),
    m_collisionVertices(collisionVertices),
    m_collisionTriangles(collisionTriangles),
    m_externalForces(externalForces)
{
}

ClothSimulator::~ClothSimulator()
{
    delete m_massSpringSystem;
    delete m_massSpringSolver;
    delete m_rootNode;
    delete m_deformationNode;
    delete m_meshCollisionNode;
    delete m_fixNode;
}

void ClothSimulator::setStiffness(float stiffness)
{
    m_stiffness = 1.0f + 5.0f * stiffness;
}

void ClothSimulator::convertMeshToCloth()
{
    m_clothPointSources.reserve(m_vertices.size());
    m_clothPointBuffer.reserve(m_vertices.size() * 3);

    std::map<size_t, size_t> oldVertexToNewMap;
    auto addPoint = [&](size_t index) {
        auto findNew = oldVertexToNewMap.find(index);
        if (findNew != oldVertexToNewMap.end())
            return findNew->second;
        const auto &position = m_vertices[index];
        m_clothPointBuffer.push_back(position.x());
        m_clothPointBuffer.push_back(position.y());
        m_clothPointBuffer.push_back(position.z());
        size_t newIndex = m_clothPointSources.size();
        m_clothPointSources.push_back(index);
        oldVertexToNewMap.insert({index, newIndex});
        return newIndex;
    };

    std::set<std::pair<size_t, size_t>> oldEdges;
    for (const auto &it: m_faces) {
        for (size_t i = 0; i < it.size(); ++i) {
            size_t j = (i + 1) % it.size();
            if (oldEdges.find(std::make_pair(it[i], it[j])) != oldEdges.end())
                continue;
            m_clothSprings.push_back({addPoint(it[i]), addPoint(it[j])});
            oldEdges.insert({it[i], it[j]});
            oldEdges.insert({it[j], it[i]});
        }
    }
}

void ClothSimulator::getCurrentVertices(std::vector<QVector3D> *currentVertices)
{
    *currentVertices = m_vertices;
    for (size_t newIndex = 0; newIndex < m_clothPointSources.size(); ++newIndex) {
        size_t oldIndex = m_clothPointSources[newIndex];
        auto offset = newIndex * 3;
        (*currentVertices)[oldIndex] = QVector3D(m_clothPointBuffer[offset + 0],
            m_clothPointBuffer[offset + 1],
            m_clothPointBuffer[offset + 2]);
    }
}

void ClothSimulator::step()
{
    if (nullptr == m_massSpringSolver)
        return;
        
    m_massSpringSolver->solve(5);
    m_massSpringSolver->solve(5);
    
    CgSatisfyVisitor visitor;
    visitor.satisfy(*m_rootNode);
}

void ClothSimulator::create()
{
    convertMeshToCloth();
    
    if (m_clothPointSources.empty())
        return;
    
    float mass = 0.25f / m_clothPointSources.size();
    float gravitationalForce = 9.8f * mass;
    float damping = 0.993f; // damping, close to 1.0 | 0.993f;
    float timeStep = 0.001f; //smaller for better results | 0.008f = 0.016f/2;
    
    mass_spring_system::VectorXf masses(mass * mass_spring_system::VectorXf::Ones((unsigned int)m_clothSprings.size()));
    
    mass_spring_system::EdgeList springList(m_clothSprings.size());
    mass_spring_system::VectorXf restLengths(m_clothSprings.size());
    mass_spring_system::VectorXf stiffnesses(m_clothSprings.size());
    
    for (size_t i = 0; i < m_clothSprings.size(); ++i) {
        const auto &source = m_clothSprings[i];
        springList[i] = mass_spring_system::Edge(source.first, source.second);
        restLengths[i] = (m_vertices[m_clothPointSources[source.first]] - m_vertices[m_clothPointSources[source.second]]).length() * 0.8;
        stiffnesses[i] = m_stiffness;
    }
    
    mass_spring_system::VectorXf fext(m_clothPointSources.size() * 3);
    for (size_t i = 0; i < m_clothPointSources.size(); ++i) {
        const auto &externalForce = m_externalForces[i] * gravitationalForce;
        auto offset = i * 3;
        fext[offset + 0] = externalForce.x();
        fext[offset + 1] = externalForce.y();
        fext[offset + 2] = externalForce.z();
    }
    
    m_massSpringSystem = new mass_spring_system(m_clothPointSources.size(), m_clothSprings.size(), timeStep, springList, restLengths,
        stiffnesses, masses, fext, damping);

    m_massSpringSolver = new MassSpringSolver(m_massSpringSystem, m_clothPointBuffer.data());
    
    // deformation constraint parameters
    const float tauc = 0.12f; // critical spring deformation | 0.12f
    const unsigned int deformIter = 15; // number of iterations | 15
    
    std::vector<unsigned int> structSprintIndexList;
    structSprintIndexList.reserve(springList.size());
    m_deformationNode =
        new CgSpringDeformationNode(m_massSpringSystem, m_clothPointBuffer.data(), tauc, deformIter);
    m_deformationNode->addSprings(structSprintIndexList);
    
    m_rootNode = new CgRootNode(m_massSpringSystem, m_clothPointBuffer.data());
    m_rootNode->addChild(m_deformationNode);

    m_meshCollisionNode = new CgMeshCollisionNode(m_massSpringSystem, m_clothPointBuffer.data(),
        m_collisionVertices,
        m_collisionTriangles);
    
    m_fixNode = new CgPointFixNode(m_massSpringSystem, m_clothPointBuffer.data());
    m_meshCollisionNode->fixPoints(m_fixNode);
    m_deformationNode->addChild(m_fixNode);
    
    m_rootNode->addChild(m_meshCollisionNode);
}
