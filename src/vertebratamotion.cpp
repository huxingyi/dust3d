#include "vertebratamotion.h"
#include "genericspineandpseudophysics.h"
#include "hermitecurveinterpolation.h"
#include "chainsimulator.h"
#include "ccdikresolver.h"
#include <QDebug>

void VertebrataMotion::prepareLegHeights()
{
    GenericSpineAndPseudoPhysics physics;
    physics.calculateFootHeights(m_parameters.preferredHeight, 
        m_parameters.stanceTime, m_parameters.swingTime, &m_legHeights, &m_legMoveOffsets);
}

void VertebrataMotion::prepareLegs()
{
    m_legs.clear();
    for (size_t spineNodeIndex = 0; 
            spineNodeIndex < m_spineNodes.size(); ++spineNodeIndex) {
        std::vector<std::vector<Node>> nodesBySide = {
            std::vector<Node>(),
            std::vector<Node>(),
            std::vector<Node>()
        };
        bool foundLeg = false;
        for (int side = 0; side < 3; ++side) {
            auto findLegNodes = m_legNodes.find({spineNodeIndex, (Side)side});
            if (findLegNodes == m_legNodes.end())
                continue;
            nodesBySide[side] = findLegNodes->second;
            foundLeg = true;
        }
        if (!foundLeg)
            continue;
        
        Leg leg;
        leg.nodes = leg.updatedNodes = nodesBySide;
        leg.spineNodeIndex = spineNodeIndex;
        m_legs.push_back(leg);
    }
}

void VertebrataMotion::prepareLegHeightIndices()
{
    int balancedStart = 0;
    for (int side = 0; side < 3; ++side) {
        bool foundSide = false;
        int heightIndex = balancedStart;
        for (size_t legIndex = 0; legIndex < m_legs.size(); ++legIndex) {
            auto &leg = m_legs[legIndex];
            if (leg.nodes[side].empty())
                continue;
            foundSide = true;
            leg.heightIndices[side] = heightIndex;
            heightIndex += m_parameters.legSideIntval;
        }
        if (!foundSide)
            continue;
        balancedStart += m_parameters.legBalanceIntval;
    }
}

void VertebrataMotion::calculateLegMoves(size_t heightIndex)
{
    auto calculateAverageTop = [&](size_t legIndex) {
        double sumTop = 0.0;
        size_t countForAverageTop = 0;
        auto &leg = m_legs[legIndex];
        for (int side = 0; side < 3; ++side) {
            if (leg.nodes[side].empty())
                continue;
            sumTop += m_legHeights[(heightIndex + leg.heightIndices[side]) % m_legHeights.size()];
            ++countForAverageTop;
        }
        if (0 == countForAverageTop)
            return 0.0;
        return sumTop / countForAverageTop;
    };
    
    HermiteCurveInterpolation spineInterpolation;
    
    m_updatedSpineNodes = m_spineNodes;
    
    if (m_spineNodes.size() >= 2) {
        spineInterpolation.addPerpendicularDirection(0, QVector2D(0.0, -1.0));
        spineInterpolation.addPerpendicularDirection(m_spineNodes.size() - 1, QVector2D(0.0, -1.0));
    }
            
    for (size_t legIndex = 0; legIndex < m_legs.size(); ++legIndex) {
        auto &leg = m_legs[legIndex];
        double averageTop = calculateAverageTop(legIndex);
        for (int side = 0; side < 3; ++side) {
            if (leg.nodes[side].empty())
                continue;
            double offset = averageTop - leg.nodes[side][0].position.y();
            leg.updatedNodes[side] = leg.nodes[side];
            auto &nodes = leg.updatedNodes[side];
            for (auto &node: nodes)
                node.position.setY(node.position.y() + offset);
            double bottom = std::max(averageTop - m_parameters.preferredHeight, m_groundY);
            
            double legLength = (nodes[0].position - nodes[nodes.size() - 1].position).length();
            auto moveOffset = m_legMoveOffsets[(heightIndex + leg.heightIndices[side]) % m_legHeights.size()] * 0.5 * legLength;
            
            CcdIkSolver ccdIkSolver;
            for (const auto &node: nodes) {
                int nodeIndex = ccdIkSolver.addNodeInOrder(node.position);
                if (0 == nodeIndex)
                    continue;
                if (Side::Left != (Side)side && Side::Right != (Side)side)
                    continue;
                if (nodeIndex == nodes.size() - 2) {
                    ccdIkSolver.setNodeHingeConstraint(nodeIndex, QVector3D(1.0, 0.0, 0.0), 90 - 15, 90 + 15);
                    continue;
                }
                if (nodeIndex == nodes.size() - 3) {
                    ccdIkSolver.setNodeHingeConstraint(nodeIndex, QVector3D(1.0, 0.0, 0.0), 270 - 15, 270 + 15);
                    continue;
                }
                ccdIkSolver.setNodeHingeConstraint(nodeIndex, QVector3D(1.0, 0.0, 0.0), 0, 360);
            }
            const auto &topNodePosition = nodes[0].position;
            const auto &bottomNodePosition = nodes[nodes.size() - 1].position;
            ccdIkSolver.solveTo(QVector3D(bottomNodePosition.x(), bottom, topNodePosition.z() + moveOffset));
            for (size_t i = 0; i < nodes.size(); ++i) {
                nodes[i].position = ccdIkSolver.getNodeSolvedPosition(i);
            }
            
            QVector3D legDirection = nodes[nodes.size() - 1].position - nodes[0].position;
            legDirection = legDirection.normalized() * (1.0 - m_parameters.spineStability) + 
                QVector3D(0.0, -1.0, 0.0) * m_parameters.spineStability;
            QVector2D legDirection2 = QVector2D(legDirection.z(), legDirection.y());
            spineInterpolation.addPerpendicularDirection(leg.spineNodeIndex, legDirection2.normalized());
        }
        
        if (0 == legIndex && 0 != leg.spineNodeIndex) {
            m_updatedSpineNodes[0].position.setY(m_updatedSpineNodes[0].position.y() +
                averageTop - m_updatedSpineNodes[leg.spineNodeIndex].position.y());
        }
        if (m_legs.size() - 1 == legIndex && m_updatedSpineNodes.size() - 1 != leg.spineNodeIndex) {
            m_updatedSpineNodes[m_updatedSpineNodes.size() - 1].position.setY(m_updatedSpineNodes[m_updatedSpineNodes.size() - 1].position.y() +
                averageTop - m_updatedSpineNodes[leg.spineNodeIndex].position.y());
        }
        m_updatedSpineNodes[leg.spineNodeIndex].position.setY(averageTop);
    }
    
    for (size_t nodeIndex = 0; nodeIndex < m_spineNodes.size(); ++nodeIndex) {
        const auto &node = m_updatedSpineNodes[nodeIndex];
        const auto &originalNode = m_spineNodes[nodeIndex];
        spineInterpolation.addNode(QVector2D(node.position.z(), node.position.y()), originalNode.position);
    }
    
    spineInterpolation.update();
    
    for (size_t spineNodexIndex = 0; spineNodexIndex < m_spineNodes.size(); ++spineNodexIndex) {
        auto &node = m_updatedSpineNodes[spineNodexIndex];
        const auto &updatedPosition2 = spineInterpolation.getUpdatedPosition(spineNodexIndex);
        node.position.setZ(updatedPosition2.x());
        node.position.setY(updatedPosition2.y());
    }
}
    
void VertebrataMotion::generate()
{
    prepareLegHeights();
    prepareLegs();
    prepareLegHeightIndices();
    
    ChainSimulator *tailSimulator = nullptr;
    
    int tailNodeIndex = -1;
    
    if (!m_legs.empty())
        tailNodeIndex = m_legs[m_legs.size() - 1].spineNodeIndex;
    QVector3D tailSpineOffset;
    
    for (size_t cycle = 0; cycle < m_parameters.cycles; ++cycle) {
        for (size_t heightIndex = 0; heightIndex < m_legHeights.size(); ++heightIndex) {
            calculateLegMoves(heightIndex);

            if (nullptr == tailSimulator && -1 != tailNodeIndex) {
                std::vector<QVector3D> ropeVertices(m_spineNodes.size() - tailNodeIndex);
                for (size_t nodeIndex = tailNodeIndex; nodeIndex < m_spineNodes.size(); ++nodeIndex) {
                    ropeVertices[nodeIndex - tailNodeIndex] = m_spineNodes[nodeIndex].position;
                }
                tailSimulator = new ChainSimulator(&ropeVertices);
                tailSimulator->setExternalForce(QVector3D(0.0, -9.80665 / 10 + 9.80665, -9.80665 * 4));
                tailSimulator->fixVertexPosition(0);
                tailSimulator->start();
                
                tailSpineOffset = m_updatedSpineNodes[tailNodeIndex].position - m_spineNodes[tailNodeIndex].position;
            }
            
            if (nullptr != tailSimulator) {
                tailSimulator->updateVertexPosition(0, m_updatedSpineNodes[tailNodeIndex].position - tailSpineOffset);
                tailSimulator->simulate(1.0 / 60);
                for (size_t nodeIndex = tailNodeIndex; nodeIndex < m_spineNodes.size(); ++nodeIndex) {
                    m_updatedSpineNodes[nodeIndex].position = tailSimulator->getVertexMotion(nodeIndex - tailNodeIndex).position + tailSpineOffset;
                }
            }

            auto frame = m_updatedSpineNodes;
            for (const auto &it: m_legs) {
                for (const auto &nodes: it.updatedNodes)
                    for (const auto &node: nodes)
                        frame.push_back(node);
            }
            
            m_frames.push_back(frame);
        }
    }
    
    delete tailSimulator;
}
