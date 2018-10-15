#ifndef SIMPLEUV_CHART_PACKER_H
#define SIMPLEUV_CHART_PACKER_H
#include <vector>

namespace simpleuv
{

class ChartPacker
{
public:
    void setCharts(const std::vector<std::pair<float, float>> &chartSizes);
    const std::vector<std::tuple<float, float, float, float, bool>> &getResult();
    void pack();
    bool tryPack(float textureSize);

private:
    double calculateTotalArea();

    std::vector<std::pair<float, float>> m_chartSizes;
    std::vector<std::tuple<float, float, float, float, bool>> m_result;
    float m_initialAreaGuessFactor = 1.05;
    float m_textureSizeGrowFactor = 0.01;
    float m_floatToIntFactor = 10000;
    size_t m_tryNum = 0;
    float m_textureSizeFactor = 1.0;
};

}

#endif

