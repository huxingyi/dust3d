#include <map>
#include <QDebug>
#include "planemesh.h"

void PlaneMesh::build()
{
    delete m_resultVertices;
    m_resultVertices = new std::vector<QVector3D>;
    
    delete m_resultQuads;
    m_resultQuads = new std::vector<std::vector<size_t>>;
    
    std::map<std::pair<size_t, size_t>, size_t> *columnAndRowToIndexMap = new std::map<std::pair<size_t, size_t>, size_t>;
    
    auto addVertex = [=](const std::pair<size_t, size_t> columnAndRow, const QVector3D &position) {
        auto insertResult = columnAndRowToIndexMap->insert({columnAndRow, m_resultVertices->size()});
        if (insertResult.second) {
            m_resultVertices->push_back(position);
        }
        return insertResult.first->second;
    };
    
    QVector3D perpunicularAxis = QVector3D::crossProduct(m_axis, m_normal);
    QVector3D stepWidth = m_axis * m_radius;
    QVector3D stepHeight = perpunicularAxis * m_radius;
    //qDebug() << "stepWidth:" << stepWidth.x() << "," << stepWidth.y() << "," << stepWidth.z();
    //qDebug() << "stepHeight:" << stepHeight.x() << "," << stepHeight.y() << "," << stepHeight.z();
    for (size_t row = 0; row < m_halfRows; ++row) {
        for (size_t column = 0; column < m_halfColumns; ++column) {
            QVector3D bottomLeft = m_origin + stepHeight * (double)row + stepWidth * (double)column;
            std::vector<QVector3D> positions = {
                bottomLeft,
                bottomLeft + stepHeight,
                bottomLeft + stepHeight + stepWidth,
                bottomLeft + stepWidth
            };
            //qDebug() << "column:" << column << "row:" << row;
            //for (size_t i = 0; i < 4; ++i) {
            //    qDebug() << "position[" << i << "]:" << positions[i].x() << "," << positions[i].y() << "," << positions[i].z();
            //}
            m_resultQuads->push_back({
                addVertex({column + 0, row + 0}, positions[0]),
                addVertex({column + 0, row + 1}, positions[1]),
                addVertex({column + 1, row + 1}, positions[2]),
                addVertex({column + 1, row + 0}, positions[3])
            });
        }
    }
    
    delete columnAndRowToIndexMap;
    
    delete m_resultFaces;
    m_resultFaces = new std::vector<std::vector<size_t>>;
    m_resultFaces->reserve(m_resultQuads->size() * 2);
    for (const auto &quad: *m_resultQuads) {
        m_resultFaces->push_back({quad[0], quad[1], quad[2]});
        m_resultFaces->push_back({quad[2], quad[3], quad[0]});
    }
}
