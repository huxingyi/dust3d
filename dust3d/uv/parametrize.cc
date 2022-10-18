/*
 *  Copyright (c) 2016-2021 Jeremy HU <jeremy-at-dust3d dot org>. All rights reserved. 
 *
 *  Permission is hereby granted, free of charge, to any person obtaining a copy
 *  of this software and associated documentation files (the "Software"), to deal
 *  in the Software without restriction, including without limitation the rights
 *  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 *  copies of the Software, and to permit persons to whom the Software is
 *  furnished to do so, subject to the following conditions:

 *  The above copyright notice and this permission notice shall be included in all
 *  copies or substantial portions of the Software.

 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 *  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 *  SOFTWARE.
 */

#ifndef NDEBUG
#define NDEBUG
#endif
#include <dust3d/uv/parametrize.h>
#include <igl/arap.h>
#include <igl/boundary_loop.h>
#include <igl/harmonic.h>
#include <igl/lscm.h>
#include <igl/map_vertices_to_circle.h>

namespace dust3d {
namespace uv {

    void parametrizeUsingARAP(const Eigen::MatrixXd& V, const Eigen::MatrixXi& F, const Eigen::VectorXi& bnd, Eigen::MatrixXd& V_uv)
    {
        Eigen::MatrixXd initial_guess;
        Eigen::MatrixXd bnd_uv;
        igl::map_vertices_to_circle(V, bnd, bnd_uv);

        igl::harmonic(V, F, bnd, bnd_uv, 1, initial_guess);

        // Add dynamic regularization to avoid to specify boundary conditions
        igl::ARAPData arap_data;
        arap_data.with_dynamics = true;
        Eigen::VectorXi b = Eigen::VectorXi::Zero(0);
        Eigen::MatrixXd bc = Eigen::MatrixXd::Zero(0, 0);

        // Initialize ARAP
        arap_data.max_iter = 100;
        // 2 means that we're going to *solve* in 2d
        arap_precomputation(V, F, 2, b, arap_data);

        // Solve arap using the harmonic map as initial guess
        V_uv = initial_guess;

        arap_solve(bc, arap_data, V_uv);
    }

    void parametrizeUsingLSCM(const Eigen::MatrixXd& V, const Eigen::MatrixXi& F, const Eigen::VectorXi& bnd, Eigen::MatrixXd& V_uv)
    {
        Eigen::VectorXi b(2, 1);
        b(0) = bnd(0);
        b(1) = bnd(round(bnd.size() / 2));
        Eigen::MatrixXd bc(2, 2);
        bc << 0, 0, 1, 0;

        // LSCM parametrization
        igl::lscm(V, F, b, bc, V_uv);
    }

    bool extractResult(const std::vector<Vertex>& verticies, const Eigen::MatrixXd& V_uv, std::vector<TextureCoord>& vertexUvs)
    {
        vertexUvs.clear();
        auto isCoordValid = [=](float coord) {
            if (std::isnan(coord) || std::isinf(coord))
                return false;
            return true;
        };
        if ((decltype(verticies.size()))V_uv.size() < verticies.size() * 2) {
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

    bool parametrize(const std::vector<Vertex>& verticies,
        const std::vector<Face>& faces,
        std::vector<TextureCoord>& vertexUvs)
    {
        if (verticies.empty() || faces.empty())
            return false;

        Eigen::MatrixXd V(verticies.size(), 3);
        Eigen::MatrixXi F(faces.size(), 3);

        for (decltype(verticies.size()) i = 0; i < verticies.size(); i++) {
            const auto& vertex = verticies[i];
            V.row(i) << vertex.xyz[0], vertex.xyz[1], vertex.xyz[2];
        }

        for (decltype(faces.size()) i = 0; i < faces.size(); i++) {
            const auto& face = faces[i];
            F.row(i) << face.indices[0], face.indices[1], face.indices[2];
        }

        Eigen::VectorXi bnd;
        igl::boundary_loop(F, bnd);

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
}
