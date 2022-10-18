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

#ifndef DUST3D_UV_CHART_PACKER_H_
#define DUST3D_UV_CHART_PACKER_H_

#include <cstdlib>
#include <tuple>
#include <vector>

namespace dust3d {
namespace uv {

    class ChartPacker {
    public:
        void setCharts(const std::vector<std::pair<float, float>>& chartSizes);
        const std::vector<std::tuple<float, float, float, float, bool>>& getResult();
        float pack();
        bool tryPack(float textureSize);

    private:
        double calculateTotalArea();

        std::vector<std::pair<float, float>> m_chartSizes;
        std::vector<std::tuple<float, float, float, float, bool>> m_result;
        float m_initialAreaGuessFactor = 1.1;
        float m_textureSizeGrowFactor = 0.05;
        float m_floatToIntFactor = 10000;
        size_t m_tryNum = 0;
        float m_textureSizeFactor = 1.0;
        float m_paddingSize = 0.005;
        size_t m_maxTryNum = 100;
    };

}
}

#endif
