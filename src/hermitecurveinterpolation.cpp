#include <QVector3D>
#include <QDebug>
#include "hermitecurveinterpolation.h"

void HermiteCurveInterpolation::update()
{
    std::vector<std::pair<size_t, QVector2D>> keyNodes;
    for (size_t nodeIndex = 0; nodeIndex < m_nodes.size(); ++nodeIndex) {
        auto findPerpendicularDirections = m_perpendicularDirectionsPerNode.find(nodeIndex);
        if (findPerpendicularDirections == m_perpendicularDirectionsPerNode.end())
            continue;
        QVector2D perpendicularDirection;
        for (const auto &it: findPerpendicularDirections->second)
            perpendicularDirection += it;
        perpendicularDirection.normalize();
        QVector3D direction3 = QVector3D(perpendicularDirection.x(), perpendicularDirection.y(), 0.0);
        QVector3D tangent3 = QVector3D::crossProduct(direction3, QVector3D(0, 0, 1.0));
        keyNodes.push_back({nodeIndex, QVector2D(tangent3.x(), tangent3.y())});
    }
    
    m_updatedPositions = m_nodes;
    for (size_t keyIndex = 1; keyIndex < keyNodes.size(); ++keyIndex) {
        const auto &firstKey = keyNodes[keyIndex - 1];
        const auto &secondKey = keyNodes[keyIndex];
        double distance = 0;
        for (size_t nodeIndex = firstKey.first + 1; nodeIndex <= secondKey.first; ++nodeIndex) {
            distance += (m_nodesForDistance[nodeIndex] - m_nodesForDistance[nodeIndex - 1]).length();
        }
        double t = 0;
        const auto &p1 = m_nodes[firstKey.first];
        const auto &t1 = firstKey.second;
        const auto &p2 = m_nodes[secondKey.first];
        const auto &t2 = secondKey.second;
        //qDebug() << "======================" << keyIndex << "=========================";
        //qDebug() << "P1:" << p1.x() << "," << p1.y();
        //qDebug() << "T1:" << t1.x() << "," << t1.y();
        //qDebug() << "P2:" << p2.x() << "," << p2.y();
        //qDebug() << "T2:" << t2.x() << "," << t2.y();
        for (size_t nodeIndex = firstKey.first + 1; nodeIndex < secondKey.first; ++nodeIndex) {
            t += (m_nodesForDistance[nodeIndex] - m_nodesForDistance[nodeIndex - 1]).length();
            double s = t / distance;
            double s3 = std::pow(s, 3);
            double s2 = std::pow(s, 2);
            double h1 = 2 * s3 - 3 * s2 + 1;
            double h2 = -2 * s3 + 3 * s2;
            double h3 = s3 - 2 * s2 + s;
            double h4 = s3 - s2;
            //qDebug() << "s:" << s << "t:" << t << "distance:" << distance << "h1:" << h1 << "h2:" << h2 << "h3:" << h3 << "h4:" << h4;
            //qDebug() << "HCI original s:" << s << " position:" << m_updatedPositions[nodeIndex].x() << "," << m_updatedPositions[nodeIndex].y();
            m_updatedPositions[nodeIndex] = h1 * p1 + h2 * p2 + h3 * t1 + h4 * t2;
            //qDebug() << "HCI Updated s:" << s << " position:" << m_updatedPositions[nodeIndex].x() << "," << m_updatedPositions[nodeIndex].y();
        }
    }
}
