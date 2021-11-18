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

#ifndef DUST3D_UV_UV_UNWRAPPER_H_
#define DUST3D_UV_UV_UNWRAPPER_H_

#include <vector>
#include <map>
#include <tuple>
#include <dust3d/uv/uv_mesh_data_type.h>

namespace dust3d 
{
namespace uv
{

class UvUnwrapper
{
public:
    void setMesh(const Mesh &mesh);
    void setTexelSize(float texelSize);
    void unwrap();
    const std::vector<FaceTextureCoords> &getFaceUvs() const;
    const std::vector<Rect> &getChartRects() const;
    const std::vector<int> &getChartSourcePartitions() const;
    float getTextureSize() const;

private:
    void partition();
    void splitPartitionToIslands(const std::vector<size_t> &group, std::vector<std::vector<size_t>> &islands);
    void unwrapSingleIsland(const std::vector<size_t> &group, int sourcePartition, bool skipCheckHoles=false);
    void parametrizeSingleGroup(const std::vector<Vertex> &verticies,
        const std::vector<Face> &faces,
        std::map<size_t, size_t> &localToGlobalFacesMap,
        size_t faceNumToChart,
        int sourcePartition);
    bool fixHolesExceptTheLongestRing(const std::vector<Vertex> &verticies, std::vector<Face> &faces, size_t *remainingHoleNum=nullptr);
    void makeSeamAndCut(const std::vector<Vertex> &verticies,
        const std::vector<Face> &faces,
        std::map<size_t, size_t> &localToGlobalFacesMap,
        std::vector<size_t> &firstGroup, std::vector<size_t> &secondGroup);
    void calculateSizeAndRemoveInvalidCharts();
    void packCharts();
    void finalizeUv();
    void buildEdgeToFaceMap(const std::vector<size_t> &group, std::map<std::pair<size_t, size_t>, size_t> &edgeToFaceMap);
    void buildEdgeToFaceMap(const std::vector<Face> &faces, std::map<std::pair<size_t, size_t>, size_t> &edgeToFaceMap);
    double distanceBetweenVertices(const Vertex &first, const Vertex &second);
    void triangulateRing(const std::vector<Vertex> &verticies,
        std::vector<Face> &faces, const std::vector<size_t> &ring);
    void calculateFaceTextureBoundingBox(const std::vector<FaceTextureCoords> &faceTextureCoords,
        float &left, float &top, float &right, float &bottom);

    Mesh m_mesh;
    std::vector<FaceTextureCoords> m_faceUvs;
    std::map<int, std::vector<size_t>> m_partitions;
    std::vector<std::pair<std::vector<size_t>, std::vector<FaceTextureCoords>>> m_charts;
    std::vector<std::pair<float, float>> m_chartSizes;
    std::vector<std::pair<float, float>> m_scaledChartSizes;
    std::vector<Rect> m_chartRects;
    std::vector<int> m_chartSourcePartitions;
    bool m_segmentByNormal = true;
    float m_segmentDotProductThreshold = 0.0;    //90 degrees
    float m_texelSizePerUnit = 1.0;
    float m_resultTextureSize = 0;
    bool m_segmentPreferMorePieces = true;
    bool m_enableRotation = true;
    static const std::vector<float> m_rotateDegrees;
};

}
}

#endif
