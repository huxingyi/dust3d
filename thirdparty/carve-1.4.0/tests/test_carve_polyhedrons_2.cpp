#include <carve/csg.hpp>
#include <carve/poly.hpp>
#include <carve/geom.hpp>

#include <vector>
#include <iostream>

typedef carve::poly::Vertex<3> vertex_t;
typedef carve::poly::Edge<3> edge_t;
typedef carve::poly::Face<3> face_t;


int main()
{
  //create a tetrahedron
  std::vector<carve::poly::Vertex<3> > tet_verts;
  std::vector<carve::poly::Face<3> > tet_faces;
  std::vector<const vertex_t *> corners;

  tet_verts.push_back(carve::poly::Vertex<3>(carve::geom::VECTOR(0.0, 0.0, 0.0)));
  tet_verts.push_back(carve::poly::Vertex<3>(carve::geom::VECTOR(1.0, 0.0, 0.0)));
  tet_verts.push_back(carve::poly::Vertex<3>(carve::geom::VECTOR(0.0, 1.0, 0.0)));
  tet_verts.push_back(carve::poly::Vertex<3>(carve::geom::VECTOR(0.0, 0.0, 1.0)));
 
  corners.push_back(&tet_verts[0]);
  corners.push_back(&tet_verts[2]);
  corners.push_back(&tet_verts[1]);
  tet_faces.push_back(face_t(corners));

  corners.clear();
  corners.push_back(&tet_verts[0]);
  corners.push_back(&tet_verts[1]);
  corners.push_back(&tet_verts[3]);
  tet_faces.push_back(face_t(corners));

  corners.clear();
  corners.push_back(&tet_verts[0]);
  corners.push_back(&tet_verts[3]);
  corners.push_back(&tet_verts[2]);
  tet_faces.push_back(face_t(corners));

  corners.clear();
  corners.push_back(&tet_verts[1]);
  corners.push_back(&tet_verts[2]);
  corners.push_back(&tet_verts[3]);
  tet_faces.push_back(face_t(corners));

   carve::poly::Polyhedron  tetrahedron(tet_faces);

  //create a triangle 
  std::vector<carve::poly::Vertex<3> > tri_verts;
  std::vector<carve::poly::Face<3> > tri_faces;

  //Vertices
  //crashes if last coordinate set to 1e-8, but ok for 1e-7
  tri_verts.push_back(carve::poly::Vertex<3>(carve::geom::VECTOR(-0.3, 0.0, 1e-8)));
  tri_verts.push_back(carve::poly::Vertex<3>(carve::geom::VECTOR(1.0, 0.0, 1.1e-8)));
  tri_verts.push_back(carve::poly::Vertex<3>(carve::geom::VECTOR(-0.3, 1.0, 1.1e-8)));

  //Face
  corners.clear();
  corners.push_back(&tri_verts[0]);
  corners.push_back(&tri_verts[2]);
  corners.push_back(&tri_verts[1]);
  tri_faces.push_back(face_t(corners));

//  corners.clear();
//  corners.push_back(&tri_verts[0]);
//  corners.push_back(&tri_verts[1]);
//  corners.push_back(&tri_verts[2]);
//  tri_faces.push_back(face_t(corners));

  carve::poly::Polyhedron triangle(tri_faces);

  //cut triangle with tetrahedron.
  carve::poly::Polyhedron * is_poly = carve::csg::CSG().compute(&tetrahedron,&triangle,
			    carve::csg::CSG::INTERSECTION);

  std::cout << "Tetrahedron is ... \n" << tetrahedron;
  std::cout << "Triangle is ... \n" << triangle;
  std::cout << "Intersection is ... \n" << *is_poly;

  return 0;
}
