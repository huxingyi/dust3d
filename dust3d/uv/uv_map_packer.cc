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

void UvMapPacker::pack()
{
    if (m_partTriangleUvs.empty())
        return;

    std::vector<std::pair<float, float>> chartSizes(m_partTriangleUvs.size());
    for (size_t i = 0; i < m_partTriangleUvs.size(); ++i) {
        const auto& part = m_partTriangleUvs[i];
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
        if (flipped) {
            for (auto& it : part.localUv) {
                std::swap(it.second[0][0], it.second[0][1]);
                std::swap(it.second[1][0], it.second[1][1]);
                std::swap(it.second[2][0], it.second[2][1]);
            }
        }
        for (const auto& it : part.localUv) {
            layout.globalUv.insert({ it.first,
                std::array<Vector2, 3> {
                    Vector2(left + (it.second[0].x() * width) / m_packedTextureSize,
                        top + (it.second[0].y() * height) / m_packedTextureSize),
                    Vector2(left + (it.second[1].x() * width) / m_packedTextureSize,
                        top + (it.second[1].y() * height) / m_packedTextureSize),
                    Vector2(left + (it.second[2].x() * width) / m_packedTextureSize,
                        top + (it.second[2].y() * height) / m_packedTextureSize) } });
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
