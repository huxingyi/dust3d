/*
 *  Copyright (c) 2016-2022 Jeremy HU <jeremy-at-dust3d dot org>. All rights reserved. 
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

#include <dust3d/base/debug.h>
#include <dust3d/uv/chart_packer.h>
#include <dust3d/uv/uv_map_packer.h>

namespace dust3d {

UvMapPacker::UvMapPacker()
{
}

void UvMapPacker::addPart(const Part& part)
{
    m_partTriangleUvs.push_back(part);
}

void UvMapPacker::addSeams(const std::vector<std::map<std::array<PositionKey, 3>, std::array<Vector2, 3>>>& seamTriangleUvs)
{
    for (const auto& it : seamTriangleUvs)
        m_seams.push_back(it);
}

void UvMapPacker::resolveSeamUvs()
{
    struct TriangleUv {
        size_t partIndex;
        std::array<Vector2, 3> uv;
    };

    std::map<std::array<PositionKey, 2>, TriangleUv> halfEdgeToUvMap;
    for (size_t partIndex = 0; partIndex < m_partTriangleUvs.size(); ++partIndex) {
        const auto& part = m_partTriangleUvs[partIndex];
        for (const auto& it : part.localUv) {
            halfEdgeToUvMap.insert(std::make_pair(std::array<PositionKey, 2> {
                                                      it.first[1], it.first[0] },
                TriangleUv { partIndex, it.second }));
            halfEdgeToUvMap.insert(std::make_pair(std::array<PositionKey, 2> {
                                                      it.first[2], it.first[1] },
                TriangleUv { partIndex, it.second }));
            halfEdgeToUvMap.insert(std::make_pair(std::array<PositionKey, 2> {
                                                      it.first[0], it.first[2] },
                TriangleUv { partIndex, it.second }));
        }
    }

    for (size_t seamIndex = 0; seamIndex < m_seams.size(); ++seamIndex) {
        const auto& seam = m_seams[seamIndex];
        double seamUvMapWidth = 0.0;
        double seamUvMapHeight = 0.0;
        for (const auto& triangle : seam) {
            auto findUv = halfEdgeToUvMap.find({ triangle.first[0], triangle.first[1] });
            if (findUv == halfEdgeToUvMap.end())
                continue;
            const auto& triangleUv = findUv->second;
            const auto& part = m_partTriangleUvs[triangleUv.partIndex];
            seamUvMapWidth += std::abs(triangleUv.uv[0].x() - triangleUv.uv[1].x()) * part.width;
            seamUvMapHeight += std::abs(triangleUv.uv[0].y() - triangleUv.uv[1].y()) * part.height;
        }
        // dust3dDebug << "Seam uv map size:" << seamUvMapWidth << seamUvMapHeight;
    }
    // TODO:
}

void UvMapPacker::pack()
{
    if (m_partTriangleUvs.empty())
        return;

    resolveSeamUvs();

    std::vector<std::pair<float, float>> chartSizes(m_partTriangleUvs.size());
    for (size_t i = 0; i < m_partTriangleUvs.size(); ++i) {
        const auto& part = m_partTriangleUvs[i];
        //dust3dDebug << "part.width:" << part.width << "part.height:" << part.height;
        chartSizes[i] = { part.width, part.height };
    }

    ChartPacker chartPacker;
    chartPacker.setCharts(chartSizes);
    m_packedTextureSize = chartPacker.pack();
    const auto& packedResult = chartPacker.getResult();
    for (size_t i = 0; i < packedResult.size(); ++i) {
        auto& part = m_partTriangleUvs[i];
        const auto& result = packedResult[i];
        auto& left = std::get<0>(result);
        auto& top = std::get<1>(result);
        auto& width = std::get<2>(result);
        auto& height = std::get<3>(result);
        auto& flipped = std::get<4>(result);
        //dust3dDebug << "left:" << left << "top:" << top << "width:" << width << "height:" << height << "flipped:" << flipped;
        Layout layout;
        layout.id = part.id;
        layout.flipped = flipped;
        if (flipped) {
            layout.left = left;
            layout.top = top;
            layout.width = height;
            layout.height = width;
        } else {
            layout.left = left;
            layout.top = top;
            layout.width = width;
            layout.height = height;
        }
        auto partWidth = part.width;
        auto partHeight = part.height;
        if (flipped) {
            for (auto& it : part.localUv) {
                std::swap(it.second[0][0], it.second[0][1]);
                std::swap(it.second[1][0], it.second[1][1]);
                std::swap(it.second[2][0], it.second[2][1]);
            }
            std::swap(partWidth, partHeight);
        }
        for (const auto& it : part.localUv) {
            layout.globalUv.insert({ it.first,
                std::array<Vector2, 3> {
                    Vector2((left * m_packedTextureSize + it.second[0].x() * partWidth) / m_packedTextureSize,
                        (top * m_packedTextureSize + it.second[0].y() * partHeight) / m_packedTextureSize),
                    Vector2((left * m_packedTextureSize + it.second[1].x() * partWidth) / m_packedTextureSize,
                        (top * m_packedTextureSize + it.second[1].y() * partHeight) / m_packedTextureSize),
                    Vector2((left * m_packedTextureSize + it.second[2].x() * partWidth) / m_packedTextureSize,
                        (top * m_packedTextureSize + it.second[2].y() * partHeight) / m_packedTextureSize) } });
        }
        m_packedLayouts.emplace_back(layout);
    }
}

double UvMapPacker::packedTextureSize()
{
    return m_packedTextureSize;
}

const std::vector<UvMapPacker::Layout>& UvMapPacker::packedLayouts()
{
    return m_packedLayouts;
}
}
