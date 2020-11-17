#ifndef SIMPLEUV_CHART_PACKER_H
#define SIMPLEUV_CHART_PACKER_H
#include <vector>
#include <cstdlib>
#include <tuple>

namespace simpleuv
{

class ChartPacker
{
public:
    void setCharts(const std::vector<std::pair<float, float>> &chartSizes);
    const std::vector<std::tuple<float, float, float, float, bool>> &getResult();
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

#endif

