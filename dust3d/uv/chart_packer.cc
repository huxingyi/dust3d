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

#include <cmath>
#include <dust3d/uv/chart_packer.h>
#include <dust3d/uv/max_rectangles.h>

namespace dust3d {
namespace uv {

    void ChartPacker::setCharts(const std::vector<std::pair<float, float>>& chartSizes)
    {
        m_chartSizes = chartSizes;
    }

    const std::vector<std::tuple<float, float, float, float, bool>>& ChartPacker::getResult()
    {
        return m_result;
    }

    double ChartPacker::calculateTotalArea()
    {
        double totalArea = 0;
        for (const auto& chartSize : m_chartSizes) {
            totalArea += chartSize.first * chartSize.second;
        }
        return totalArea;
    }

    bool ChartPacker::tryPack(float textureSize)
    {
        std::vector<MaxRectanglesSize> rects;
        int width = textureSize * m_floatToIntFactor;
        int height = width;
        float paddingSize = m_paddingSize * width;
        float paddingSize2 = paddingSize + paddingSize;
        for (const auto& chartSize : m_chartSizes) {
            MaxRectanglesSize r;
            r.width = chartSize.first * m_floatToIntFactor + paddingSize2;
            r.height = chartSize.second * m_floatToIntFactor + paddingSize2;
            rects.push_back(r);
        }
        const MaxRectanglesFreeRectChoiceHeuristic methods[] = {
            kMaxRectanglesBestShortSideFit,
            kMaxRectanglesBestLongSideFit,
            kMaxRectanglesBestAreaFit,
            kMaxRectanglesBottomLeftRule,
            kMaxRectanglesContactPointRule
        };
        float occupancy = 0;
        float bestOccupancy = 0;
        std::vector<MaxRectanglesPosition> bestResult;
        for (size_t i = 0; i < sizeof(methods) / sizeof(methods[0]); ++i) {
            std::vector<MaxRectanglesPosition> result(rects.size());
            if (0 != maxRectangles(width, height, rects.size(), rects.data(), methods[i], true, result.data(), &occupancy)) {
                continue;
            }
            if (occupancy > bestOccupancy) {
                bestResult = result;
                bestOccupancy = occupancy;
            }
        }
        if (bestResult.size() != rects.size())
            return false;
        m_result.resize(bestResult.size());
        for (decltype(bestResult.size()) i = 0; i < bestResult.size(); ++i) {
            const auto& result = bestResult[i];
            const auto& rect = rects[i];
            auto& dest = m_result[i];
            std::get<0>(dest) = (float)(result.left + paddingSize) / width;
            std::get<1>(dest) = (float)(result.top + paddingSize) / height;
            std::get<2>(dest) = (float)(rect.width - paddingSize2) / width;
            std::get<3>(dest) = (float)(rect.height - paddingSize2) / height;
            std::get<4>(dest) = result.rotated;
        }
        return true;
    }

    float ChartPacker::pack()
    {
        float textureSize = 0;
        float initialGuessSize = std::sqrt(calculateTotalArea() * m_initialAreaGuessFactor);
        while (true) {
            textureSize = initialGuessSize * m_textureSizeFactor;
            ++m_tryNum;
            if (tryPack(textureSize))
                break;
            m_textureSizeFactor += m_textureSizeGrowFactor;
            if (m_tryNum >= m_maxTryNum) {
                break;
            }
        }
        return textureSize;
    }

}
}
