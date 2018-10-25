#include <igl/arap.h>
#include <igl/boundary_loop.h>
#include <igl/harmonic.h>
#include <igl/map_vertices_to_circle.h>
#include <simpleuv/parametrize.h>
#include <QDebug>

namespace simpleuv
{

// Modified from the libigl example code
// https://github.com/libigl/libigl/blob/master/tutorial/503_ARAPParam/main.cpp
bool parametrize(const std::vector<Vertex> &verticies,
        const std::vector<Face> &faces, 
        std::vector<TextureCoord> &vertexUvs)
{
    if (verticies.empty() || faces.empty())
        return false;
    
    //qDebug() << "parametrize vertices:" << verticies.size() << "faces:" << faces.size();

    Eigen::MatrixXd V(verticies.size(), 3);
    Eigen::MatrixXi F(faces.size(), 3);
    Eigen::MatrixXd V_uv;
    Eigen::MatrixXd initial_guess;

    for (decltype(verticies.size()) i = 0; i < verticies.size(); i++) {
        const auto &vertex = verticies[i];
        V.row(i) << vertex.xyz[0], vertex.xyz[1], vertex.xyz[2];
    }

    for (decltype(faces.size()) i = 0; i < faces.size(); i++) {
        const auto &face = faces[i];
        F.row(i) << face.indicies[0], face.indicies[1], face.indicies[2];
    }

    // Compute the initial solution for ARAP (harmonic parametrization)
    Eigen::VectorXi bnd;
    igl::boundary_loop(F,bnd);
    Eigen::MatrixXd bnd_uv;
    igl::map_vertices_to_circle(V,bnd,bnd_uv);

    igl::harmonic(V,F,bnd,bnd_uv,1,initial_guess);

    // Add dynamic regularization to avoid to specify boundary conditions
    igl::ARAPData arap_data;
    arap_data.with_dynamics = true;
    Eigen::VectorXi b  = Eigen::VectorXi::Zero(0);
    Eigen::MatrixXd bc = Eigen::MatrixXd::Zero(0,0);

    // Initialize ARAP
    arap_data.max_iter = 100;
    // 2 means that we're going to *solve* in 2d
    arap_precomputation(V,F,2,b,arap_data);


    // Solve arap using the harmonic map as initial guess
    V_uv = initial_guess;

    arap_solve(bc,arap_data,V_uv);

    for (decltype(verticies.size()) i = 0; i < verticies.size(); i++) {
        TextureCoord coord;
        coord.uv[0] = V_uv.row(i)[0];
        coord.uv[1] = V_uv.row(i)[1];
        vertexUvs.push_back(coord);
    }
    
    return true;
}

}
