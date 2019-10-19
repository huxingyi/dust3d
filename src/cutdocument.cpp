#include <QDebug>
#include <QApplication>
#include "cutdocument.h"

const float CutDocument::m_nodeRadius = 0.05;
const float CutDocument::m_nodeScaleFactor = 0.5;

bool CutDocument::hasPastableNodesInClipboard() const
{
    return false;
}

bool CutDocument::originSettled() const
{
    return false;
}

bool CutDocument::isNodeEditable(QUuid nodeId) const
{
    Q_UNUSED(nodeId);
    return true;
}

bool CutDocument::isEdgeEditable(QUuid edgeId) const
{
    Q_UNUSED(edgeId);
    return true;
}

void CutDocument::copyNodes(std::set<QUuid> nodeIdSet) const
{
    Q_UNUSED(nodeIdSet);
}

void CutDocument::saveHistoryItem()
{
    CutHistoryItem item;
    toCutTemplate(item.cutTemplate);
    m_undoItems.push_back(item);
}

bool CutDocument::undoable() const
{
    return m_undoItems.size() >= 2;
}

bool CutDocument::redoable() const
{
    return !m_redoItems.empty();
}

void CutDocument::undo()
{
    if (!undoable())
        return;
    m_redoItems.push_back(m_undoItems.back());
    m_undoItems.pop_back();
    const auto &item = m_undoItems.back();
    fromCutTemplate(item.cutTemplate);
}

void CutDocument::redo()
{
    if (m_redoItems.empty())
        return;
    m_undoItems.push_back(m_redoItems.back());
    const auto &item = m_redoItems.back();
    fromCutTemplate(item.cutTemplate);
    m_redoItems.pop_back();
}

void CutDocument::paste()
{
    // void
}

void CutDocument::reset()
{
    nodeMap.clear();
    edgeMap.clear();
    partMap.clear();
    m_cutNodeIds.clear();
    emit cleanup();
    emit cutTemplateChanged();
}

void CutDocument::clearHistories()
{
    m_undoItems.clear();
    m_redoItems.clear();
}

void CutDocument::toCutTemplate(std::vector<QVector2D> &cutTemplate)
{
    for (const auto &nodeId: m_cutNodeIds) {
        auto findNode = nodeMap.find(nodeId);
        if (findNode == nodeMap.end())
            continue;
        QVector2D position = nodeToCutPosition({findNode->second.getX(),
            findNode->second.getY(),
            findNode->second.getZ()
        });
        cutTemplate.push_back(position);
    }
}

QVector2D CutDocument::nodeToCutPosition(const QVector3D &nodePosition)
{
    return {(nodePosition.x() * 2 - (float)1) / m_nodeScaleFactor,
        (nodePosition.y() * 2 - (float)1) / m_nodeScaleFactor};
}

QVector3D CutDocument::cutToNodePosition(const QVector2D &cutPosition)
{
    return {(cutPosition.x() * m_nodeScaleFactor + 1) * (float)0.5,
        (cutPosition.y() * m_nodeScaleFactor + 1) * (float)0.5,
        (float)0};
}

void CutDocument::fromCutTemplate(const std::vector<QVector2D> &cutTemplate)
{
    reset();
    
    std::set<QUuid> newAddedNodeIds;
    std::set<QUuid> newAddedEdgeIds;
    
    m_partId = QUuid::createUuid();
    auto &part = partMap[m_partId];
    part.id = m_partId;
    
    for (const auto &position: cutTemplate) {
        SkeletonNode node;
        node.partId = m_partId;
        node.id = QUuid::createUuid();
        node.setRadius(m_nodeRadius);
        auto nodePosition = cutToNodePosition(position);
        node.setX(nodePosition.x());
        node.setY(nodePosition.y());
        node.setZ(nodePosition.z());
        nodeMap[node.id] = node;
        newAddedNodeIds.insert(node.id);
        m_cutNodeIds.push_back(node.id);
    }
    
    for (size_t i = 0; i < m_cutNodeIds.size(); ++i) {
        size_t j = (i + 1) % m_cutNodeIds.size();
        const QUuid &firstNodeId = m_cutNodeIds[i];
        const QUuid &secondNodeId = m_cutNodeIds[j];
        
        SkeletonEdge edge;
        edge.partId = m_partId;
        edge.id = QUuid::createUuid();
        edge.nodeIds.push_back(firstNodeId);
        edge.nodeIds.push_back(secondNodeId);
        edgeMap[edge.id] = edge;
        newAddedEdgeIds.insert(edge.id);
        nodeMap[firstNodeId].edgeIds.push_back(edge.id);
        nodeMap[secondNodeId].edgeIds.push_back(edge.id);
    }
    
    for (const auto &nodeIt: newAddedNodeIds) {
        emit nodeAdded(nodeIt);
    }
    for (const auto &edgeIt: newAddedEdgeIds) {
        emit edgeAdded(edgeIt);
    }
    
    emit cutTemplateChanged();
}

void CutDocument::moveNodeBy(QUuid nodeId, float x, float y, float z)
{
    auto it = nodeMap.find(nodeId);
    if (it == nodeMap.end()) {
        qDebug() << "Find node failed:" << nodeId;
        return;
    }
    it->second.addX(x);
    it->second.addY(y);
    it->second.addZ(z);
    emit nodeOriginChanged(it->first);
    emit cutTemplateChanged();
}

void CutDocument::setNodeOrigin(QUuid nodeId, float x, float y, float z)
{
    auto it = nodeMap.find(nodeId);
    if (it == nodeMap.end()) {
        qDebug() << "Find node failed:" << nodeId;
        return;
    }
    it->second.setX(x);
    it->second.setY(y);
    it->second.setZ(z);
    auto part = partMap.find(it->second.partId);
    if (part != partMap.end())
        part->second.dirty = true;
    emit nodeOriginChanged(nodeId);
    emit cutTemplateChanged();
}

