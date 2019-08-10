#include <igl/arap.h>
#include <igl/lscm.h>
#include <igl/boundary_loop.h>
#include <igl/harmonic.h>
#include <igl/map_vertices_to_circle.h>
#include <simpleuv/parametrize.h>

namespace simpleuv
{

void parametrizeUsingARAP(const Eigen::MatrixXd &V, const Eigen::MatrixXi &F, const Eigen::VectorXi &bnd, Eigen::MatrixXd &V_uv)
{
    Eigen::MatrixXd initial_guess;
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
}

void parametrizeUsingLSCM(const Eigen::MatrixXd &V, const Eigen::MatrixXi &F, const Eigen::VectorXi &bnd, Eigen::MatrixXd &V_uv)
{
    Eigen::VectorXi b(2,1);
    b(0) = bnd(0);
    b(1) = bnd(round(bnd.size()/2));
    Eigen::MatrixXd bc(2,2);
    bc<<0,0,1,0;

    // LSCM parametrization
    igl::lscm(V,F,b,bc,V_uv);
}

bool extractResult(const std::vector<Vertex> &verticies, const Eigen::MatrixXd &V_uv, std::vector<TextureCoord> &vertexUvs)
{
    vertexUvs.clear();
    auto isCoordValid = [=](float coord) {
        if (std::isnan(coord) || std::isinf(coord))
            return false;
        return true;
    };
    if ((decltype(verticies.size()))V_uv.size() < verticies.size() * 2) {
        //qDebug() << "Invalid V_uv.size:" << V_uv.size() << "Expected:" << verticies.size() * 2;
        return false;
    }
    for (decltype(verticies.size()) i = 0; i < verticies.size(); i++) {
        TextureCoord coord;
        coord.uv[0] = V_uv.row(i)[0];
        coord.uv[1] = V_uv.row(i)[1];
        if (isCoordValid(coord.uv[0]) && isCoordValid(coord.uv[1])) {
            vertexUvs.push_back(coord);
            continue;
        }
        vertexUvs.clear();
        return false;
    }
    return true;
}

// Modified from the libigl example code
// https://github.com/libigl/libigl/blob/master/tutorial/503_ARAPParam/main.cpp
bool parametrize(const std::vector<Vertex> &verticies,
        const std::vector<Face> &faces, 
        std::vector<TextureCoord> &vertexUvs)
{
    if (verticies.empty() || faces.empty())
        return false;
    
    Eigen::MatrixXd V(verticies.size(), 3);
    Eigen::MatrixXi F(faces.size(), 3);

    for (decltype(verticies.size()) i = 0; i < verticies.size(); i++) {
        const auto &vertex = verticies[i];
        V.row(i) << vertex.xyz[0], vertex.xyz[1], vertex.xyz[2];
    }

    for (decltype(faces.size()) i = 0; i < faces.size(); i++) {
        const auto &face = faces[i];
        F.row(i) << face.indices[0], face.indices[1], face.indices[2];
    }

    Eigen::VectorXi bnd;
    igl::boundary_loop(F,bnd);
    
    {
        Eigen::MatrixXd V_uv;
        parametrizeUsingARAP(V, F, bnd, V_uv);
        if (extractResult(verticies, V_uv, vertexUvs))
            return true;
    }

    {
        Eigen::MatrixXd V_uv;
        parametrizeUsingLSCM(V, F, bnd, V_uv);
        if (extractResult(verticies, V_uv, vertexUvs))
            return true;
    }
    
    return false;
}

}
