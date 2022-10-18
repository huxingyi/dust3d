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

#include <dust3d/mesh/hole_stitcher.h>

namespace dust3d {

HoleStitcher::~HoleStitcher()
{
    delete m_wrapper;
}

void HoleStitcher::setVertices(const std::vector<Vector3>* vertices)
{
    m_positions = vertices;
}

const std::vector<std::vector<size_t>>& HoleStitcher::newlyGeneratedFaces()
{
    if (m_wrapper)
        return m_wrapper->newlyGeneratedFaces();

    return m_newlyGeneratedFaces;
}

void HoleStitcher::getFailedEdgeLoops(std::vector<size_t>& failedEdgeLoops)
{
    if (m_wrapper)
        m_wrapper->getFailedEdgeLoops(failedEdgeLoops);
}

bool HoleStitcher::stitchByQuads(const std::vector<std::pair<std::vector<size_t>, Vector3>>& edgeLoops)
{
    if (2 != edgeLoops.size())
        return false;
    const auto& firstEdgeLoop = edgeLoops[0].first;
    const auto& secondEdgeLoop = edgeLoops[1].first;
    if (firstEdgeLoop.size() < 3 || firstEdgeLoop.size() != secondEdgeLoop.size())
        return false;
    float minDist2 = std::numeric_limits<float>::max();
    size_t choosenStartIndex = 0;
    for (size_t k = 0; k < secondEdgeLoop.size(); ++k) {
        float sumOfDist2 = 0;
        std::vector<Vector3> edges;
        for (size_t i = 0, j = k; i < firstEdgeLoop.size(); ++i, --j) {
            const auto& positionOnFirstEdgeLoop = (*m_positions)[firstEdgeLoop[i % firstEdgeLoop.size()]];
            const auto& positionOnSecondEdgeLoop = (*m_positions)[secondEdgeLoop[(j + secondEdgeLoop.size()) % secondEdgeLoop.size()]];
            auto edge = positionOnSecondEdgeLoop - positionOnFirstEdgeLoop;
            sumOfDist2 += edge.lengthSquared();
        }
        if (sumOfDist2 < minDist2) {
            minDist2 = sumOfDist2;
            choosenStartIndex = k;
        }
    }
    for (size_t i = 0, j = choosenStartIndex; i < firstEdgeLoop.size(); ++i, --j) {
        m_newlyGeneratedFaces.push_back({
            secondEdgeLoop[(j + secondEdgeLoop.size()) % secondEdgeLoop.size()],
            secondEdgeLoop[(j + secondEdgeLoop.size() - 1) % secondEdgeLoop.size()],
            firstEdgeLoop[(i + 1) % firstEdgeLoop.size()],
            firstEdgeLoop[i % firstEdgeLoop.size()],
        });
    }
    return true;
}

bool HoleStitcher::stitch(const std::vector<std::pair<std::vector<size_t>, Vector3>>& edgeLoops)
{
    if (edgeLoops.size() == 2 && edgeLoops[0].first.size() == edgeLoops[1].first.size())
        return stitchByQuads(edgeLoops);

    m_wrapper = new HoleWrapper;
    m_wrapper->setVertices(m_positions);
    m_wrapper->wrap(edgeLoops);
    if (!m_wrapper->finished()) {
        return false;
    }
    return true;
}

}
