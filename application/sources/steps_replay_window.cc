#include "steps_replay_window.h"
#include "animation_manage_widget.h"
#include "bone_manage_widget.h"
#include "component_property_widget.h"
#include "document.h"
#include "document_window.h"
#include "glb_forever.h"
#include "skeleton_graphics_widget.h"
#include <QApplication>
#include <QDebug>
#include <QDockWidget>
#include <QHBoxLayout>
#include <QScreen>
#include <QVBoxLayout>
#include <dust3d/base/part_target.h>
#include <map>
#include <memory>
#include <set>

StepsReplayWindow::StepsReplayWindow(Document* sourceDocument, QWidget* parent)
    : QWidget(parent, Qt::Tool | Qt::WindowStaysOnTopHint | Qt::FramelessWindowHint)
    , m_sourceDocument(sourceDocument)
{
    setAttribute(Qt::WA_DeleteOnClose);
    setFixedWidth(320);

    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(8, 6, 8, 6);
    layout->setSpacing(4);

    m_stepTitleLabel = new QLabel(tr("Ready to start replay"), this);
    m_stepTitleLabel->setAlignment(Qt::AlignCenter);
    m_stepTitleLabel->setWordWrap(true);
    QFont titleFont = m_stepTitleLabel->font();
    titleFont.setPointSize(11);
    titleFont.setBold(true);
    m_stepTitleLabel->setFont(titleFont);
    layout->addWidget(m_stepTitleLabel);

    m_stepProgressLabel = new QLabel(this);
    m_stepProgressLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(m_stepProgressLabel);

    QHBoxLayout* controlLayout = new QHBoxLayout();
    controlLayout->setSpacing(4);

    m_playPauseButton = new QPushButton(tr("Play"), this);
    m_playPauseButton->setFixedWidth(50);
    connect(m_playPauseButton, &QPushButton::clicked, this, &StepsReplayWindow::togglePlayPause);
    controlLayout->addWidget(m_playPauseButton);

    m_nextButton = new QPushButton(tr("Next"), this);
    m_nextButton->setFixedWidth(50);
    connect(m_nextButton, &QPushButton::clicked, this, &StepsReplayWindow::executeNextStep);
    controlLayout->addWidget(m_nextButton);

    QLabel* speedTextLabel = new QLabel(tr("Speed:"), this);
    controlLayout->addWidget(speedTextLabel);

    m_speedLabel = new QLabel(tr("1.0x"), this);
    m_speedLabel->setFixedWidth(45);
    controlLayout->addWidget(m_speedLabel);

    m_speedSlider = new QSlider(Qt::Horizontal, this);
    m_speedSlider->setRange(1, 20);
    m_speedSlider->setValue(20);
    connect(m_speedSlider, &QSlider::valueChanged, this, &StepsReplayWindow::speedChanged);
    controlLayout->addWidget(m_speedSlider);

    QPushButton* closeButton = new QPushButton(tr("Close"), this);
    closeButton->setFixedWidth(50);
    connect(closeButton, &QPushButton::clicked, this, [this]() {
        m_autoPlayTimer->stop();
        m_playing = false;
        close();
    });
    controlLayout->addWidget(closeButton);

    layout->addLayout(controlLayout);

    m_autoPlayTimer = new QTimer(this);
    m_autoPlayTimer->setInterval(1500);
    connect(m_autoPlayTimer, &QTimer::timeout, this, &StepsReplayWindow::executeNextStep);

    speedChanged(20);

    m_demoWindow = DocumentWindow::createDocumentWindow();
    m_demoWindow->setWindowTitle(tr("Replay Steps - Rebuilding Document"));
    m_demoWindow->show();
    m_demoDocument = m_demoWindow->document();

    connect(m_demoWindow, &QObject::destroyed, this, [this]() {
        m_autoPlayTimer->stop();
        m_demoWindow = nullptr;
        m_demoDocument = nullptr;
        close();
    });

    connect(parent, &QObject::destroyed, this, [this]() {
        m_autoPlayTimer->stop();
        m_sourceDocument = nullptr;
        close();
    });

    // Copy the origin from the source document so the canvas anchor is correct
    m_demoDocument->setOriginX(m_sourceDocument->getOriginX());
    m_demoDocument->setOriginY(m_sourceDocument->getOriginY());
    m_demoDocument->setOriginZ(m_sourceDocument->getOriginZ());

    buildSteps();

    // Auto-execute background image setup steps so the overlay is already hidden
    while (m_currentStep + 1 < (int)m_steps.size()) {
        const auto& nextStep = m_steps[m_currentStep + 1];
        if (nextStep.title.contains(tr("background image"), Qt::CaseInsensitive)
            || nextStep.title.contains(tr("Examining"), Qt::CaseInsensitive)) {
            m_currentStep++;
            m_steps[m_currentStep].execute();
        } else {
            break;
        }
    }

    updateStepDisplay();
    positionSelf();
}

StepsReplayWindow::~StepsReplayWindow()
{
    m_autoPlayTimer->stop();
    closePropertyWidget();
}

void StepsReplayWindow::positionSelf()
{
    QScreen* screen = QApplication::primaryScreen();
    if (screen) {
        QRect geo = screen->availableGeometry();
        move(geo.right() - width() - 10, geo.bottom() - height() - 50);
    }
}

void StepsReplayWindow::closePropertyWidget()
{
    if (m_currentPropertyWidget) {
        m_currentPropertyWidget->close();
        delete m_currentPropertyWidget;
        m_currentPropertyWidget = nullptr;
    }
}

dust3d::Uuid StepsReplayWindow::findDemoPartId(const dust3d::Uuid& sourcePartId)
{
    for (const auto& it : m_demoDocument->partMap) {
        for (const auto& nid : it.second.nodeIds) {
            auto srcIt = m_sourceDocument->nodeMap.find(nid);
            if (srcIt != m_sourceDocument->nodeMap.end() && srcIt->second.partId == sourcePartId)
                return it.first;
        }
    }
    return dust3d::Uuid();
}

dust3d::Uuid StepsReplayWindow::findDemoComponentId(const dust3d::Uuid& demoPartId)
{
    for (const auto& it : m_demoDocument->componentMap) {
        if (it.second.linkToPartId == demoPartId)
            return it.first;
    }
    return dust3d::Uuid();
}

dust3d::Uuid StepsReplayWindow::findDemoEdgeByNodes(const dust3d::Uuid& nodeId1, const dust3d::Uuid& nodeId2)
{
    for (const auto& it : m_demoDocument->edgeMap) {
        if (it.second.nodeIds.size() == 2) {
            if ((it.second.nodeIds[0] == nodeId1 && it.second.nodeIds[1] == nodeId2)
                || (it.second.nodeIds[0] == nodeId2 && it.second.nodeIds[1] == nodeId1)) {
                return it.first;
            }
        }
    }
    return dust3d::Uuid();
}

void StepsReplayWindow::showPropertyWidgetForComponent(const dust3d::Uuid& demoComponentId)
{
    closePropertyWidget();
    if (demoComponentId.isNull())
        return;
    std::vector<dust3d::Uuid> componentIds = { demoComponentId };
    m_currentPropertyWidget = new ComponentPropertyWidget(m_demoDocument, componentIds);
    m_currentPropertyWidget->setWindowFlag(Qt::Tool);
    m_currentPropertyWidget->setWindowFlag(Qt::WindowStaysOnTopHint);
    m_currentPropertyWidget->setWindowTitle(tr("Properties"));

    // Position at the right top of the canvas
    SkeletonGraphicsWidget* canvas = m_demoWindow ? m_demoWindow->canvasGraphicsWidget() : nullptr;
    if (canvas) {
        QPoint canvasTopRight = canvas->mapToGlobal(QPoint(canvas->width(), 0));
        int x = canvasTopRight.x() - m_currentPropertyWidget->sizeHint().width() - 10;
        int y = canvasTopRight.y() + 10;
        m_currentPropertyWidget->move(x, y);
    } else if (m_demoWindow) {
        QRect demoGeo = m_demoWindow->geometry();
        m_currentPropertyWidget->move(demoGeo.right() - 300, demoGeo.top() + 60);
    }

    m_currentPropertyWidget->show();
}

void StepsReplayWindow::onRigReady()
{
    if (!m_waitingForRig)
        return;
    m_waitingForRig = false;
    if (m_demoDocument)
        disconnect(m_demoDocument, &Document::rigTypeChanged, this, nullptr);

    // Resume playback
    m_stepTitleLabel->setText(tr("Rig generation complete"));
    if (m_playing)
        m_autoPlayTimer->start();
    m_nextButton->setEnabled(true);
}

void StepsReplayWindow::buildSteps()
{
    m_steps.clear();

    // Default part values
    const float defaultDeformThickness = 1.0f;
    const float defaultDeformWidth = 1.0f;
    const float defaultCutRotation = 0.0f;
    const float defaultMetalness = 0.0f;
    const float defaultRoughness = 1.0f;
    const float defaultHollowThickness = 0.0f;
    const dust3d::CutFace defaultCutFace = dust3d::CutFace::Quad;
    const dust3d::PartTarget defaultTarget = dust3d::PartTarget::Model;
    const dust3d::CombineMode defaultCombineMode = dust3d::CombineMode::Normal;
    const float defaultBackCloseDepthRatio = 1.0f;
    const float defaultBackCloseSharpness = 0.0f;
    const size_t defaultTargetSegments = 0;
    const float defaultSmoothCutoffDegrees = 0.0f;

    // Phase 1: Background image
    if (!m_sourceDocument->turnaround.isNull()) {
        m_steps.push_back({ tr("Examining background image from document"), [this]() {
                               closePropertyWidget();
                           } });
        m_steps.push_back({ tr("Setting background image"), [this]() {
                               m_demoDocument->updateTurnaround(m_sourceDocument->turnaround);
                           } });
    }

    // Phase 2: Create parts in component tree order
    struct PartEntry {
        dust3d::Uuid partId;
        dust3d::Uuid srcComponentId;
        QString name;
        std::vector<dust3d::Uuid> nodeIds;
    };
    std::vector<PartEntry> parts;

    std::function<void(const Document::Component&)> collectParts = [&](const Document::Component& component) {
        if (!component.linkToPartId.isNull()) {
            auto partIt = m_sourceDocument->partMap.find(component.linkToPartId);
            if (partIt != m_sourceDocument->partMap.end()) {
                PartEntry entry;
                entry.partId = partIt->first;
                entry.srcComponentId = component.id;
                entry.name = partIt->second.name;
                entry.nodeIds = partIt->second.nodeIds;
                parts.push_back(entry);
            }
        }
        for (const auto& childId : component.childrenIds) {
            auto childIt = m_sourceDocument->componentMap.find(childId);
            if (childIt != m_sourceDocument->componentMap.end())
                collectParts(childIt->second);
        }
    };
    collectParts(m_sourceDocument->rootComponent);

    // Deferred steps for cutFaceLinkedId - the linked part may not exist yet
    // Each entry: { dependsOnSourcePartId, step }
    std::vector<std::pair<dust3d::Uuid, ReplayStep>> deferredCutFaceLinkedSteps;
    std::set<dust3d::Uuid> createdSourcePartIds;

    for (size_t pi = 0; pi < parts.size(); ++pi) {
        const auto& entry = parts[pi];
        const auto& srcPart = m_sourceDocument->partMap.find(entry.partId)->second;
        const auto* srcComponent = (m_sourceDocument->componentMap.find(entry.srcComponentId) != m_sourceDocument->componentMap.end())
            ? &m_sourceDocument->componentMap.find(entry.srcComponentId)->second
            : nullptr;
        QString partName = entry.name.isEmpty() ? QString("Part %1").arg(pi + 1) : entry.name;

        // Order nodes by walking the chain using undirected adjacency
        // Edge directions are fixed afterwards by the reverseEdge step
        std::vector<dust3d::Uuid> orderedNodes;
        bool isClosedLoop = false;
        {
            if (!entry.nodeIds.empty()) {
                // Build undirected adjacency for this part's edges
                std::map<dust3d::Uuid, std::vector<dust3d::Uuid>> adjacency;
                for (const auto& edgeIt : m_sourceDocument->edgeMap) {
                    if (edgeIt.second.partId != entry.partId)
                        continue;
                    if (edgeIt.second.nodeIds.size() != 2)
                        continue;
                    dust3d::Uuid n0 = edgeIt.second.nodeIds[0];
                    dust3d::Uuid n1 = edgeIt.second.nodeIds[1];
                    adjacency[n0].push_back(n1);
                    adjacency[n1].push_back(n0);
                }

                // Find start node: prefer endpoint (degree 1 in part)
                // For closed loop all nodes have degree 2
                dust3d::Uuid startNode;
                for (const auto& nodeId : entry.nodeIds) {
                    auto adjIt = adjacency.find(nodeId);
                    size_t degree = (adjIt != adjacency.end()) ? adjIt->second.size() : 0;
                    if (degree <= 1) {
                        startNode = nodeId;
                        break;
                    }
                }
                if (startNode.isNull()) {
                    // All nodes have degree >= 2: closed loop
                    startNode = entry.nodeIds[0];
                    isClosedLoop = (entry.nodeIds.size() >= 3);
                }

                // Debug: log adjacency
                qDebug() << "=== Part:" << partName << "nodes:" << entry.nodeIds.size() << "isClosedLoop:" << isClosedLoop;
                for (const auto& adj : adjacency) {
                    QString neighbors;
                    for (const auto& n : adj.second)
                        neighbors += QString::fromStdString(n.toString()).left(8) + " ";
                    qDebug() << "  node" << QString::fromStdString(adj.first.toString()).left(8) << "degree:" << adj.second.size() << "neighbors:" << neighbors;
                }
                qDebug() << "  startNode:" << QString::fromStdString(startNode.toString()).left(8);

                // Walk the chain: always pick the neighbor we haven't visited yet
                std::set<dust3d::Uuid> visited;
                dust3d::Uuid current = startNode;
                visited.insert(current);
                orderedNodes.push_back(current);
                while (true) {
                    auto adjIt = adjacency.find(current);
                    if (adjIt == adjacency.end())
                        break;
                    dust3d::Uuid nextNode;
                    for (const auto& neighbor : adjIt->second) {
                        if (visited.find(neighbor) == visited.end()) {
                            nextNode = neighbor;
                            break;
                        }
                    }
                    if (nextNode.isNull())
                        break;
                    visited.insert(nextNode);
                    orderedNodes.push_back(nextNode);
                    current = nextNode;
                }

                // Debug: log walk result
                QString walkStr;
                for (const auto& n : orderedNodes)
                    walkStr += QString::fromStdString(n.toString()).left(8) + " ";
                qDebug() << "  walk:" << walkStr;
            }
        }

        // Add nodes
        if (!orderedNodes.empty()) {
            dust3d::Uuid firstNodeId = orderedNodes[0];
            m_steps.push_back({ tr("Creating %1 - Node 1/%2").arg(partName).arg(orderedNodes.size()), [this, firstNodeId]() {
                                   closePropertyWidget();
                                   const auto& srcNode = m_sourceDocument->nodeMap.find(firstNodeId)->second;
                                   m_demoDocument->addNodeWithId(firstNodeId, srcNode.getX(), srcNode.getY(), srcNode.getZ(), srcNode.radius, dust3d::Uuid());
                               } });

            for (size_t ni = 1; ni < orderedNodes.size(); ++ni) {
                dust3d::Uuid nodeId = orderedNodes[ni];
                dust3d::Uuid prevNodeId = orderedNodes[ni - 1];
                m_steps.push_back({ tr("Creating %1 - Node %2/%3").arg(partName).arg(ni + 1).arg(orderedNodes.size()), [this, nodeId, prevNodeId]() {
                                       const auto& srcNode = m_sourceDocument->nodeMap.find(nodeId)->second;
                                       m_demoDocument->addNodeWithId(nodeId, srcNode.getX(), srcNode.getY(), srcNode.getZ(), srcNode.radius, prevNodeId);
                                   } });
            }

            // Close the loop if this part has a circular chain of nodes
            if (isClosedLoop && orderedNodes.size() >= 3) {
                dust3d::Uuid lastNodeId = orderedNodes.back();
                dust3d::Uuid loopFirstNodeId = orderedNodes[0];
                m_steps.push_back({ tr("Closing loop for %1").arg(partName), [this, lastNodeId, loopFirstNodeId]() {
                                       m_demoDocument->addEdge(lastNodeId, loopFirstNodeId);
                                   } });
            }

            // Collect source edge directions for this part
            struct EdgeDirection {
                dust3d::Uuid nodeId0;
                dust3d::Uuid nodeId1;
            };
            std::vector<EdgeDirection> srcEdgeDirections;
            for (const auto& edgeIt : m_sourceDocument->edgeMap) {
                if (edgeIt.second.partId == entry.partId && edgeIt.second.nodeIds.size() == 2) {
                    EdgeDirection dir;
                    dir.nodeId0 = edgeIt.second.nodeIds[0];
                    dir.nodeId1 = edgeIt.second.nodeIds[1];
                    srcEdgeDirections.push_back(dir);
                }
            }

            // Verify node positions after all nodes are placed
            {
                std::vector<dust3d::Uuid> allNodes = orderedNodes;
                m_steps.push_back({ tr("Verifying node positions for %1").arg(partName), [this, allNodes]() {
                                       for (const auto& nodeId : allNodes) {
                                           auto srcIt = m_sourceDocument->nodeMap.find(nodeId);
                                           if (srcIt == m_sourceDocument->nodeMap.end())
                                               continue;
                                           auto demoIt = m_demoDocument->nodeMap.find(nodeId);
                                           if (demoIt == m_demoDocument->nodeMap.end())
                                               continue;
                                           float sx = srcIt->second.getX(), sy = srcIt->second.getY(), sz = srcIt->second.getZ();
                                           float sr = srcIt->second.radius;
                                           if (demoIt->second.getX() != sx || demoIt->second.getY() != sy || demoIt->second.getZ() != sz)
                                               m_demoDocument->setNodeOrigin(nodeId, sx, sy, sz);
                                           if (demoIt->second.radius != sr)
                                               m_demoDocument->setNodeRadius(nodeId, sr);
                                       }
                                   } });
            }

            // Apply per-node cut face settings
            for (const auto& nodeId : orderedNodes) {
                auto srcNodeIt = m_sourceDocument->nodeMap.find(nodeId);
                if (srcNodeIt == m_sourceDocument->nodeMap.end())
                    continue;
                const auto& srcNode = srcNodeIt->second;
                if (!srcNode.hasCutFaceSettings)
                    continue;
                dust3d::CutFace nodeCutFace = srcNode.cutFace;
                dust3d::Uuid nodeCutFaceLinkedId = srcNode.cutFaceLinkedId;
                dust3d::Uuid nid = nodeId;
                m_steps.push_back({ tr("%1: Set node cut face").arg(partName), [this, nid, nodeCutFace]() {
                                       m_demoDocument->setNodeCutFace(nid, nodeCutFace);
                                   } });
                if (!nodeCutFaceLinkedId.isNull()) {
                    dust3d::Uuid srcLinkedPartId = nodeCutFaceLinkedId;
                    ReplayStep linkStep = { tr("%1: Link node cut face to part").arg(partName), [this, nid, srcLinkedPartId]() {
                                               dust3d::Uuid demoLinkedPartId = findDemoPartId(srcLinkedPartId);
                                               if (demoLinkedPartId.isNull())
                                                   return;
                                               m_demoDocument->setNodeCutFaceLinkedId(nid, demoLinkedPartId);
                                           } };
                    if (createdSourcePartIds.find(srcLinkedPartId) != createdSourcePartIds.end()) {
                        m_steps.push_back(std::move(linkStep));
                    } else {
                        deferredCutFaceLinkedSteps.push_back({ srcLinkedPartId, std::move(linkStep) });
                    }
                }
            }

            // Fix edge directions after all nodes are placed
            if (!srcEdgeDirections.empty()) {
                m_steps.push_back({ tr("Fixing edge directions for %1").arg(partName), [this, srcEdgeDirections]() {
                                       for (const auto& srcDir : srcEdgeDirections) {
                                           dust3d::Uuid demoEdgeId = findDemoEdgeByNodes(srcDir.nodeId0, srcDir.nodeId1);
                                           if (demoEdgeId.isNull())
                                               continue;
                                           const auto& demoEdge = m_demoDocument->edgeMap.find(demoEdgeId)->second;
                                           if (demoEdge.nodeIds.size() == 2
                                               && demoEdge.nodeIds[0] == srcDir.nodeId1
                                               && demoEdge.nodeIds[1] == srcDir.nodeId0) {
                                               // Direction is reversed, fix it
                                               m_demoDocument->reverseEdge(demoEdgeId);
                                           }
                                       }
                                   } });
            }
        }

        // Part target
        dust3d::Uuid sourcePartId = entry.partId;

        if (srcPart.target != defaultTarget) {
            dust3d::PartTarget target = srcPart.target;
            dust3d::Uuid importedModelId = srcPart.importedModelId;
            m_steps.push_back({ tr("Setting target for %1: %2").arg(partName).arg(QString::fromStdString(dust3d::PartTargetToDispName(target))), [this, sourcePartId, target, importedModelId]() {
                                   dust3d::Uuid demoPartId = findDemoPartId(sourcePartId);
                                   if (demoPartId.isNull())
                                       return;
                                   showPropertyWidgetForComponent(findDemoComponentId(demoPartId));
                                   m_demoDocument->setPartTarget(demoPartId, target);
                                   if (target == dust3d::PartTarget::ImportedModel && !importedModelId.isNull()) {
                                       const QByteArray* glbData = GlbForever::get(importedModelId);
                                       if (glbData) {
                                           dust3d::Uuid newModelId = GlbForever::add(glbData, importedModelId);
                                           m_demoDocument->setPartImportedModelId(demoPartId, newModelId);
                                       }
                                   }
                               } });

            // After ImportedModel, wait 1 second then verify combine mode matches
            if (target == dust3d::PartTarget::ImportedModel && srcComponent) {
                dust3d::CombineMode expectedMode = srcComponent->combineMode;
                m_steps.push_back({ tr("%1: Verifying combine mode after import").arg(partName), [this, sourcePartId, expectedMode]() {
                                       // Pause playback, wait 1 second, then check
                                       m_autoPlayTimer->stop();
                                       m_nextButton->setEnabled(false);
                                       QTimer::singleShot(1000, this, [this, sourcePartId, expectedMode]() {
                                           if (!m_demoDocument)
                                               return;
                                           dust3d::Uuid demoPartId = findDemoPartId(sourcePartId);
                                           if (!demoPartId.isNull()) {
                                               dust3d::Uuid demoCompId = findDemoComponentId(demoPartId);
                                               if (!demoCompId.isNull()) {
                                                   auto compIt = m_demoDocument->componentMap.find(demoCompId);
                                                   if (compIt != m_demoDocument->componentMap.end()) {
                                                       if (compIt->second.combineMode != expectedMode) {
                                                           showPropertyWidgetForComponent(demoCompId);
                                                           m_demoDocument->setComponentCombineMode(demoCompId, expectedMode);
                                                       }
                                                   }
                                               }
                                           }
                                           // Resume
                                           m_nextButton->setEnabled(true);
                                           if (m_playing)
                                               m_autoPlayTimer->start();
                                       });
                                   } });
            }
        }

        // Part boolean properties
        if (srcPart.xMirrored) {
            m_steps.push_back({ tr("%1: Enable X Mirror").arg(partName), [this, sourcePartId]() {
                                   dust3d::Uuid demoPartId = findDemoPartId(sourcePartId);
                                   if (demoPartId.isNull())
                                       return;
                                   showPropertyWidgetForComponent(findDemoComponentId(demoPartId));
                                   m_demoDocument->setPartXmirrorState(demoPartId, true);
                               } });
        }
        if (srcPart.rounded) {
            m_steps.push_back({ tr("%1: Enable Rounded").arg(partName), [this, sourcePartId]() {
                                   dust3d::Uuid demoPartId = findDemoPartId(sourcePartId);
                                   if (demoPartId.isNull())
                                       return;
                                   showPropertyWidgetForComponent(findDemoComponentId(demoPartId));
                                   m_demoDocument->setPartRoundState(demoPartId, true);
                               } });
        }
        if (srcPart.chamfered) {
            m_steps.push_back({ tr("%1: Enable Chamfered").arg(partName), [this, sourcePartId]() {
                                   dust3d::Uuid demoPartId = findDemoPartId(sourcePartId);
                                   if (demoPartId.isNull())
                                       return;
                                   showPropertyWidgetForComponent(findDemoComponentId(demoPartId));
                                   m_demoDocument->setPartChamferState(demoPartId, true);
                               } });
        }
        if (srcPart.subdived) {
            m_steps.push_back({ tr("%1: Enable Subdivide").arg(partName), [this, sourcePartId]() {
                                   dust3d::Uuid demoPartId = findDemoPartId(sourcePartId);
                                   if (demoPartId.isNull())
                                       return;
                                   showPropertyWidgetForComponent(findDemoComponentId(demoPartId));
                                   m_demoDocument->setPartSubdivState(demoPartId, true);
                               } });
        }
        if (srcPart.fillLoopInterior) {
            m_steps.push_back({ tr("%1: Enable Fill Loop Interior").arg(partName), [this, sourcePartId]() {
                                   dust3d::Uuid demoPartId = findDemoPartId(sourcePartId);
                                   if (demoPartId.isNull())
                                       return;
                                   showPropertyWidgetForComponent(findDemoComponentId(demoPartId));
                                   m_demoDocument->setPartFillLoopInteriorState(demoPartId, true);
                               } });
        }

        // Part float properties
        if (srcPart.deformThickness != defaultDeformThickness) {
            float val = srcPart.deformThickness;
            m_steps.push_back({ tr("%1: Set Deform Thickness = %2").arg(partName).arg(val), [this, sourcePartId, val]() {
                                   dust3d::Uuid demoPartId = findDemoPartId(sourcePartId);
                                   if (demoPartId.isNull())
                                       return;
                                   showPropertyWidgetForComponent(findDemoComponentId(demoPartId));
                                   m_demoDocument->setPartDeformThickness(demoPartId, val);
                               } });
        }
        if (srcPart.deformWidth != defaultDeformWidth) {
            float val = srcPart.deformWidth;
            m_steps.push_back({ tr("%1: Set Deform Width = %2").arg(partName).arg(val), [this, sourcePartId, val]() {
                                   dust3d::Uuid demoPartId = findDemoPartId(sourcePartId);
                                   if (demoPartId.isNull())
                                       return;
                                   showPropertyWidgetForComponent(findDemoComponentId(demoPartId));
                                   m_demoDocument->setPartDeformWidth(demoPartId, val);
                               } });
        }
        if (srcPart.deformUnified) {
            m_steps.push_back({ tr("%1: Enable Deform Unified").arg(partName), [this, sourcePartId]() {
                                   dust3d::Uuid demoPartId = findDemoPartId(sourcePartId);
                                   if (demoPartId.isNull())
                                       return;
                                   showPropertyWidgetForComponent(findDemoComponentId(demoPartId));
                                   m_demoDocument->setPartDeformUnified(demoPartId, true);
                               } });
        }
        if (srcPart.cutFace != defaultCutFace || !srcPart.cutFaceLinkedId.isNull()) {
            dust3d::CutFace cutFace = srcPart.cutFace;
            dust3d::Uuid cutFaceLinkedId = srcPart.cutFaceLinkedId;
            m_steps.push_back({ tr("%1: Set Cut Face").arg(partName), [this, sourcePartId, cutFace]() {
                                   dust3d::Uuid demoPartId = findDemoPartId(sourcePartId);
                                   if (demoPartId.isNull())
                                       return;
                                   showPropertyWidgetForComponent(findDemoComponentId(demoPartId));
                                   m_demoDocument->setPartCutFace(demoPartId, cutFace);
                               } });
            if (!cutFaceLinkedId.isNull()) {
                dust3d::Uuid srcLinkedPartId = cutFaceLinkedId;
                ReplayStep linkStep = { tr("%1: Link Cut Face to part").arg(partName), [this, sourcePartId, srcLinkedPartId]() {
                                           dust3d::Uuid demoPartId = findDemoPartId(sourcePartId);
                                           if (demoPartId.isNull())
                                               return;
                                           dust3d::Uuid demoLinkedPartId = findDemoPartId(srcLinkedPartId);
                                           if (demoLinkedPartId.isNull())
                                               return;
                                           showPropertyWidgetForComponent(findDemoComponentId(demoPartId));
                                           m_demoDocument->setPartCutFaceLinkedId(demoPartId, demoLinkedPartId);
                                       } };
                if (createdSourcePartIds.find(srcLinkedPartId) != createdSourcePartIds.end()) {
                    m_steps.push_back(std::move(linkStep));
                } else {
                    deferredCutFaceLinkedSteps.push_back({ srcLinkedPartId, std::move(linkStep) });
                }
            }
        }
        if (srcPart.cutRotation != defaultCutRotation) {
            float val = srcPart.cutRotation;
            m_steps.push_back({ tr("%1: Set Cut Rotation = %2").arg(partName).arg(val), [this, sourcePartId, val]() {
                                   dust3d::Uuid demoPartId = findDemoPartId(sourcePartId);
                                   if (demoPartId.isNull())
                                       return;
                                   showPropertyWidgetForComponent(findDemoComponentId(demoPartId));
                                   m_demoDocument->setPartCutRotation(demoPartId, val);
                               } });
        }
        if (srcPart.hollowThickness != defaultHollowThickness) {
            float val = srcPart.hollowThickness;
            m_steps.push_back({ tr("%1: Set Hollow Thickness = %2").arg(partName).arg(val), [this, sourcePartId, val]() {
                                   dust3d::Uuid demoPartId = findDemoPartId(sourcePartId);
                                   if (demoPartId.isNull())
                                       return;
                                   showPropertyWidgetForComponent(findDemoComponentId(demoPartId));
                                   m_demoDocument->setPartHollowThickness(demoPartId, val);
                               } });
        }
        if (srcPart.metalness != defaultMetalness) {
            float val = srcPart.metalness;
            m_steps.push_back({ tr("%1: Set Metalness = %2").arg(partName).arg(val), [this, sourcePartId, val]() {
                                   dust3d::Uuid demoPartId = findDemoPartId(sourcePartId);
                                   if (demoPartId.isNull())
                                       return;
                                   showPropertyWidgetForComponent(findDemoComponentId(demoPartId));
                                   m_demoDocument->setPartMetalness(demoPartId, val);
                               } });
        }
        if (srcPart.roughness != defaultRoughness) {
            float val = srcPart.roughness;
            m_steps.push_back({ tr("%1: Set Roughness = %2").arg(partName).arg(val), [this, sourcePartId, val]() {
                                   dust3d::Uuid demoPartId = findDemoPartId(sourcePartId);
                                   if (demoPartId.isNull())
                                       return;
                                   showPropertyWidgetForComponent(findDemoComponentId(demoPartId));
                                   m_demoDocument->setPartRoughness(demoPartId, val);
                               } });
        }
        if (srcPart.disabled) {
            m_steps.push_back({ tr("%1: Disable Part").arg(partName), [this, sourcePartId]() {
                                   dust3d::Uuid demoPartId = findDemoPartId(sourcePartId);
                                   if (demoPartId.isNull())
                                       return;
                                   showPropertyWidgetForComponent(findDemoComponentId(demoPartId));
                                   m_demoDocument->setPartDisableState(demoPartId, true);
                               } });
        }

        // Component-level properties
        if (srcComponent) {
            if (srcComponent->combineMode != defaultCombineMode) {
                dust3d::CombineMode mode = srcComponent->combineMode;
                m_steps.push_back({ tr("%1: Set Combine Mode").arg(partName), [this, sourcePartId, mode]() {
                                       dust3d::Uuid demoPartId = findDemoPartId(sourcePartId);
                                       if (demoPartId.isNull())
                                           return;
                                       dust3d::Uuid demoCompId = findDemoComponentId(demoPartId);
                                       showPropertyWidgetForComponent(demoCompId);
                                       m_demoDocument->setComponentCombineMode(demoCompId, mode);
                                   } });
            }
            if (srcComponent->hasColor) {
                QColor color = srcComponent->color;
                m_steps.push_back({ tr("%1: Set Color").arg(partName), [this, sourcePartId, color]() {
                                       dust3d::Uuid demoPartId = findDemoPartId(sourcePartId);
                                       if (demoPartId.isNull())
                                           return;
                                       dust3d::Uuid demoCompId = findDemoComponentId(demoPartId);
                                       showPropertyWidgetForComponent(demoCompId);
                                       m_demoDocument->setComponentColorState(demoCompId, true, color);
                                   } });
            }
            if (!srcComponent->colorImageId.isNull()) {
                dust3d::Uuid imageId = srcComponent->colorImageId;
                m_steps.push_back({ tr("%1: Set Color Image").arg(partName), [this, sourcePartId, imageId]() {
                                       dust3d::Uuid demoPartId = findDemoPartId(sourcePartId);
                                       if (demoPartId.isNull())
                                           return;
                                       dust3d::Uuid demoCompId = findDemoComponentId(demoPartId);
                                       showPropertyWidgetForComponent(demoCompId);
                                       m_demoDocument->setComponentColorImage(demoCompId, imageId);
                                   } });
            }
            if (srcComponent->sideClosed) {
                m_steps.push_back({ tr("%1: Enable Side Closed").arg(partName), [this, sourcePartId]() {
                                       dust3d::Uuid demoPartId = findDemoPartId(sourcePartId);
                                       if (demoPartId.isNull())
                                           return;
                                       dust3d::Uuid demoCompId = findDemoComponentId(demoPartId);
                                       showPropertyWidgetForComponent(demoCompId);
                                       m_demoDocument->setComponentSideCloseState(demoCompId, true);
                                   } });
            }
            if (srcComponent->frontClosed) {
                m_steps.push_back({ tr("%1: Enable Front Closed").arg(partName), [this, sourcePartId]() {
                                       dust3d::Uuid demoPartId = findDemoPartId(sourcePartId);
                                       if (demoPartId.isNull())
                                           return;
                                       dust3d::Uuid demoCompId = findDemoComponentId(demoPartId);
                                       showPropertyWidgetForComponent(demoCompId);
                                       m_demoDocument->setComponentFrontCloseState(demoCompId, true);
                                   } });
            }
            if (srcComponent->backClosed) {
                float depthRatio = srcComponent->backCloseDepthRatio;
                float sharpness = srcComponent->backCloseSharpness;
                m_steps.push_back({ tr("%1: Enable Back Closed").arg(partName), [this, sourcePartId, depthRatio, sharpness, defaultBackCloseDepthRatio, defaultBackCloseSharpness]() {
                                       dust3d::Uuid demoPartId = findDemoPartId(sourcePartId);
                                       if (demoPartId.isNull())
                                           return;
                                       dust3d::Uuid demoCompId = findDemoComponentId(demoPartId);
                                       showPropertyWidgetForComponent(demoCompId);
                                       m_demoDocument->setComponentBackCloseState(demoCompId, true);
                                       if (depthRatio != defaultBackCloseDepthRatio)
                                           m_demoDocument->setComponentBackCloseDepthRatio(demoCompId, depthRatio);
                                       if (sharpness != defaultBackCloseSharpness)
                                           m_demoDocument->setComponentBackCloseSharpness(demoCompId, sharpness);
                                   } });
            }
            if (srcComponent->targetSegments != defaultTargetSegments) {
                size_t segments = srcComponent->targetSegments;
                m_steps.push_back({ tr("%1: Set Target Segments = %2").arg(partName).arg(segments), [this, sourcePartId, segments]() {
                                       dust3d::Uuid demoPartId = findDemoPartId(sourcePartId);
                                       if (demoPartId.isNull())
                                           return;
                                       dust3d::Uuid demoCompId = findDemoComponentId(demoPartId);
                                       showPropertyWidgetForComponent(demoCompId);
                                       m_demoDocument->setComponentTargetSegments(demoCompId, segments);
                                   } });
            }
            if (srcComponent->smoothCutoffDegrees != defaultSmoothCutoffDegrees) {
                float degrees = srcComponent->smoothCutoffDegrees;
                m_steps.push_back({ tr("%1: Set Smooth Cutoff = %2deg").arg(partName).arg(degrees), [this, sourcePartId, degrees]() {
                                       dust3d::Uuid demoPartId = findDemoPartId(sourcePartId);
                                       if (demoPartId.isNull())
                                           return;
                                       dust3d::Uuid demoCompId = findDemoComponentId(demoPartId);
                                       showPropertyWidgetForComponent(demoCompId);
                                       m_demoDocument->setComponentSmoothCutoffDegrees(demoCompId, degrees);
                                   } });
            }
        }

        // Mark this source part as created
        createdSourcePartIds.insert(entry.partId);

        // Flush any deferred cutFaceLinkedId steps whose dependency is now satisfied
        for (auto it = deferredCutFaceLinkedSteps.begin(); it != deferredCutFaceLinkedSteps.end();) {
            if (createdSourcePartIds.find(it->first) != createdSourcePartIds.end()) {
                m_steps.push_back(std::move(it->second));
                it = deferredCutFaceLinkedSteps.erase(it);
            } else {
                ++it;
            }
        }
    }

    // Apply any remaining deferred cut face linked ID steps
    for (auto& step : deferredCutFaceLinkedSteps) {
        m_steps.push_back(std::move(step.second));
    }

    // Close property widget before organizing component tree
    m_steps.push_back({ tr("Parts complete - organizing component tree"), [this]() {
                           closePropertyWidget();
                       } });

    // Recreate component group hierarchy from source document
    // We traverse the source component tree and recreate groups, then move part components into them
    {
        // Map from source group component ID to demo group component ID (shared across lambdas)
        auto groupIdMap = std::make_shared<std::map<dust3d::Uuid, dust3d::Uuid>>();

        // Recursive function to collect group creation steps in tree order
        std::function<void(const Document::Component&, const dust3d::Uuid&)> buildGroupSteps;
        buildGroupSteps = [&, groupIdMap](const Document::Component& srcParent, const dust3d::Uuid& srcParentId) {
            for (const auto& childId : srcParent.childrenIds) {
                auto childIt = m_sourceDocument->componentMap.find(childId);
                if (childIt == m_sourceDocument->componentMap.end())
                    continue;
                const auto& srcChild = childIt->second;

                if (srcChild.linkToPartId.isNull()) {
                    // This is a group component - create it in the demo
                    QString groupName = srcChild.name;
                    dust3d::Uuid srcGroupId = srcChild.id;
                    dust3d::Uuid srcParentIdCopy = srcParentId;

                    m_steps.push_back({ tr("Creating group: %1").arg(groupName.isEmpty() ? tr("Unnamed Group") : groupName),
                        [this, srcGroupId, srcParentIdCopy, groupName, groupIdMap]() {
                            // Find the demo parent: either root (null) or a previously created group
                            dust3d::Uuid demoParentId;
                            if (!srcParentIdCopy.isNull()) {
                                auto mapIt = groupIdMap->find(srcParentIdCopy);
                                if (mapIt != groupIdMap->end())
                                    demoParentId = mapIt->second;
                            }
                            m_demoDocument->addComponent(demoParentId);

                            // Find the newly created group - it's the last child of the parent
                            const Document::Component* parentComp = nullptr;
                            if (demoParentId.isNull()) {
                                parentComp = &m_demoDocument->rootComponent;
                            } else {
                                auto pIt = m_demoDocument->componentMap.find(demoParentId);
                                if (pIt != m_demoDocument->componentMap.end())
                                    parentComp = &pIt->second;
                            }
                            if (parentComp && !parentComp->childrenIds.empty()) {
                                dust3d::Uuid newGroupId = parentComp->childrenIds.back();
                                (*groupIdMap)[srcGroupId] = newGroupId;
                                if (!groupName.isEmpty())
                                    m_demoDocument->renameComponent(newGroupId, groupName);
                            }
                        } });

                    // Apply group component properties if they differ from defaults
                    {
                        dust3d::Uuid srcGrpId = srcChild.id;
                        if (srcChild.combineMode != dust3d::CombineMode::Normal) {
                            dust3d::CombineMode mode = srcChild.combineMode;
                            m_steps.push_back({ tr("Group %1: Set Combine Mode").arg(groupName), [this, srcGrpId, mode, groupIdMap]() {
                                                   auto mapIt = groupIdMap->find(srcGrpId);
                                                   if (mapIt == groupIdMap->end())
                                                       return;
                                                   m_demoDocument->setComponentCombineMode(mapIt->second, mode);
                                               } });
                        }
                        if (srcChild.hasColor) {
                            QColor color = srcChild.color;
                            m_steps.push_back({ tr("Group %1: Set Color").arg(groupName), [this, srcGrpId, color, groupIdMap]() {
                                                   auto mapIt = groupIdMap->find(srcGrpId);
                                                   if (mapIt == groupIdMap->end())
                                                       return;
                                                   m_demoDocument->setComponentColorState(mapIt->second, true, color);
                                               } });
                        }
                        if (!srcChild.colorImageId.isNull()) {
                            dust3d::Uuid imageId = srcChild.colorImageId;
                            m_steps.push_back({ tr("Group %1: Set Color Image").arg(groupName), [this, srcGrpId, imageId, groupIdMap]() {
                                                   auto mapIt = groupIdMap->find(srcGrpId);
                                                   if (mapIt == groupIdMap->end())
                                                       return;
                                                   m_demoDocument->setComponentColorImage(mapIt->second, imageId);
                                               } });
                        }
                        if (srcChild.sideClosed) {
                            m_steps.push_back({ tr("Group %1: Enable Side Closed").arg(groupName), [this, srcGrpId, groupIdMap]() {
                                                   auto mapIt = groupIdMap->find(srcGrpId);
                                                   if (mapIt == groupIdMap->end())
                                                       return;
                                                   m_demoDocument->setComponentSideCloseState(mapIt->second, true);
                                               } });
                        }
                        if (srcChild.frontClosed) {
                            m_steps.push_back({ tr("Group %1: Enable Front Closed").arg(groupName), [this, srcGrpId, groupIdMap]() {
                                                   auto mapIt = groupIdMap->find(srcGrpId);
                                                   if (mapIt == groupIdMap->end())
                                                       return;
                                                   m_demoDocument->setComponentFrontCloseState(mapIt->second, true);
                                               } });
                        }
                        if (srcChild.backClosed) {
                            float depthRatio = srcChild.backCloseDepthRatio;
                            float sharpness = srcChild.backCloseSharpness;
                            m_steps.push_back({ tr("Group %1: Enable Back Closed").arg(groupName), [this, srcGrpId, depthRatio, sharpness, groupIdMap]() {
                                                   auto mapIt = groupIdMap->find(srcGrpId);
                                                   if (mapIt == groupIdMap->end())
                                                       return;
                                                   m_demoDocument->setComponentBackCloseState(mapIt->second, true);
                                                   if (depthRatio != 1.0f)
                                                       m_demoDocument->setComponentBackCloseDepthRatio(mapIt->second, depthRatio);
                                                   if (sharpness != 0.0f)
                                                       m_demoDocument->setComponentBackCloseSharpness(mapIt->second, sharpness);
                                               } });
                        }
                        if (srcChild.targetSegments != 0) {
                            size_t segments = srcChild.targetSegments;
                            m_steps.push_back({ tr("Group %1: Set Target Segments = %2").arg(groupName).arg(segments), [this, srcGrpId, segments, groupIdMap]() {
                                                   auto mapIt = groupIdMap->find(srcGrpId);
                                                   if (mapIt == groupIdMap->end())
                                                       return;
                                                   m_demoDocument->setComponentTargetSegments(mapIt->second, segments);
                                               } });
                        }
                        if (srcChild.smoothCutoffDegrees != 0.0f) {
                            float degrees = srcChild.smoothCutoffDegrees;
                            m_steps.push_back({ tr("Group %1: Set Smooth Cutoff = %2deg").arg(groupName).arg(degrees), [this, srcGrpId, degrees, groupIdMap]() {
                                                   auto mapIt = groupIdMap->find(srcGrpId);
                                                   if (mapIt == groupIdMap->end())
                                                       return;
                                                   m_demoDocument->setComponentSmoothCutoffDegrees(mapIt->second, degrees);
                                               } });
                        }
                    }

                    // Recurse into group children
                    buildGroupSteps(srcChild, srcGroupId);
                } else {
                    // This is a part component - move it into correct parent group if needed
                    dust3d::Uuid srcPartId = srcChild.linkToPartId;
                    dust3d::Uuid srcParentIdCopy = srcParentId;

                    if (!srcParentIdCopy.isNull()) {
                        // Part needs to be moved from root into a group
                        m_steps.push_back({ tr("Moving part into group"),
                            [this, srcPartId, srcParentIdCopy, groupIdMap]() {
                                dust3d::Uuid demoPartId = findDemoPartId(srcPartId);
                                if (demoPartId.isNull())
                                    return;
                                dust3d::Uuid demoCompId = findDemoComponentId(demoPartId);
                                if (demoCompId.isNull())
                                    return;
                                auto mapIt = groupIdMap->find(srcParentIdCopy);
                                if (mapIt == groupIdMap->end())
                                    return;
                                m_demoDocument->moveComponent(demoCompId, mapIt->second);
                            } });
                    }
                }
            }
        };
        buildGroupSteps(m_sourceDocument->rootComponent, dust3d::Uuid());
    }

    // Phase 3: Bones - collect source edge bone bindings by node pairs
    // Since edge IDs differ between source and demo, we match by node pair
    struct BoneBinding {
        dust3d::Uuid srcNodeId1;
        dust3d::Uuid srcNodeId2;
        QString boneName;
    };
    std::map<QString, std::vector<BoneBinding>> boneBindings;
    for (const auto& it : m_sourceDocument->edgeMap) {
        if (it.second.boneName.isEmpty())
            continue;
        if (it.second.nodeIds.size() != 2)
            continue;
        BoneBinding binding;
        binding.srcNodeId1 = it.second.nodeIds[0];
        binding.srcNodeId2 = it.second.nodeIds[1];
        binding.boneName = it.second.boneName;
        boneBindings[binding.boneName].push_back(binding);
    }

    bool hasBoneBindings = !boneBindings.empty();

    if (hasBoneBindings) {
        m_steps.push_back({ tr("Switching to Bones panel"), [this]() {
                               QList<QDockWidget*> docks = m_demoWindow->findChildren<QDockWidget*>();
                               for (auto* dock : docks) {
                                   if (dock->windowTitle() == tr("Bones")) {
                                       dock->show();
                                       dock->raise();
                                       break;
                                   }
                               }
                           } });

        QString rigType = m_sourceDocument->getRigType();
        m_steps.push_back({ tr("Setting rig type: %1").arg(rigType), [this, rigType]() {
                               m_demoDocument->setRigType(rigType);
                           } });

        for (const auto& boneEntry : boneBindings) {
            QString boneName = boneEntry.first;
            std::vector<BoneBinding> bindings = boneEntry.second;

            // Step 1: Select edges on the canvas (find by node pairs)
            m_steps.push_back({ tr("Selecting edges for bone: %1").arg(boneName), [this, bindings]() {
                                   m_demoDocument->setEditMode(Document::EditMode::Select);
                                   SkeletonGraphicsWidget* canvas = m_demoWindow->canvasGraphicsWidget();
                                   if (!canvas)
                                       return;
                                   canvas->unselectAll();
                                   for (const auto& binding : bindings) {
                                       dust3d::Uuid demoEdgeId = findDemoEdgeByNodes(binding.srcNodeId1, binding.srcNodeId2);
                                       if (!demoEdgeId.isNull())
                                           canvas->addSelectEdge(demoEdgeId);
                                   }
                               } });

            // Step 2: Select bone in the tree
            m_steps.push_back({ tr("Selecting bone: %1").arg(boneName), [this, boneName]() {
                                   BoneManageWidget* boneWidget = m_demoWindow->boneManageWidget();
                                   if (!boneWidget)
                                       return;
                                   boneWidget->selectBoneByName(boneName);
                               } });

            // Step 3: Assign via UI, then ensure all edges are bound directly
            m_steps.push_back({ tr("Assigning edges to bone: %1").arg(boneName), [this, bindings, boneName]() {
                                   BoneManageWidget* boneWidget = m_demoWindow->boneManageWidget();
                                   if (boneWidget)
                                       boneWidget->assignSelectedEdgesToBone();
                                   // Ensure all edges are bound even if UI selection missed some
                                   for (const auto& binding : bindings) {
                                       dust3d::Uuid demoEdgeId = findDemoEdgeByNodes(binding.srcNodeId1, binding.srcNodeId2);
                                       if (!demoEdgeId.isNull()) {
                                           auto edgeIt = m_demoDocument->edgeMap.find(demoEdgeId);
                                           if (edgeIt != m_demoDocument->edgeMap.end() && edgeIt->second.boneName != boneName)
                                               m_demoDocument->setEdgeBoneName(demoEdgeId, boneName);
                                       }
                                   }
                               } });
        }

        // After all bone assignments, clear canvas selection
        m_steps.push_back({ tr("Bone assignments complete"), [this]() {
                               SkeletonGraphicsWidget* canvas = m_demoWindow->canvasGraphicsWidget();
                               if (canvas)
                                   canvas->unselectAll();
                           } });
    }

    // Phase 4: Animations - need to wait for rig generation to complete
    std::vector<dust3d::Uuid> animationIds;
    m_sourceDocument->getAllAnimationIds(animationIds);

    if (!animationIds.empty()) {
        // Wait for rig step
        ReplayStep waitStep;
        waitStep.title = tr("Waiting for rig generation to complete...");
        waitStep.waitForRig = true;
        waitStep.execute = [this]() {
            // Check if rig is already ready
            if (!m_demoDocument->isRigGenerating()) {
                m_waitingForRig = false;
                return;
            }
            // Pause and wait for rig
            m_waitingForRig = true;
            m_autoPlayTimer->stop();
            m_nextButton->setEnabled(false);
            connect(m_demoDocument, &Document::resultTextureChanged, this, &StepsReplayWindow::onRigReady, Qt::UniqueConnection);
        };
        m_steps.push_back(waitStep);

        m_steps.push_back({ tr("Switching to Animations panel"), [this]() {
                               QList<QDockWidget*> docks = m_demoWindow->findChildren<QDockWidget*>();
                               for (auto* dock : docks) {
                                   if (dock->windowTitle() == tr("Animations")) {
                                       dock->show();
                                       dock->raise();
                                       break;
                                   }
                               }
                           } });

        for (const auto& animId : animationIds) {
            const auto* anim = m_sourceDocument->findAnimation(animId);
            if (!anim)
                continue;
            QString animName = anim->name;
            QString animType = anim->type;
            std::map<std::string, std::string> animParams = anim->params;
            dust3d::Uuid capturedAnimId = animId;

            // Create animation
            m_steps.push_back({ tr("Adding animation: %1").arg(animName), [this, capturedAnimId, animName, animType]() {
                                   m_demoDocument->createAnimation(capturedAnimId, animName, animType);
                               } });

            // Select animation
            m_steps.push_back({ tr("Selecting animation: %1").arg(animName), [this, capturedAnimId]() {
                                   AnimationManageWidget* animWidget = m_demoWindow->animationManageWidget();
                                   if (animWidget)
                                       animWidget->selectAnimationById(capturedAnimId);
                               } });

            // Set each parameter to target value in one step
            for (const auto& paramEntry : animParams) {
                std::string paramName = paramEntry.first;
                std::string paramValueStr = paramEntry.second;

                if (paramName == "surfaceMaterial")
                    continue;

                m_steps.push_back({ tr("%1: Set %2 = %3").arg(animName).arg(QString::fromStdString(paramName)).arg(QString::fromStdString(paramValueStr)),
                    [this, capturedAnimId, paramName, paramValueStr]() {
                        const auto* currentAnim = m_demoDocument->findAnimation(capturedAnimId);
                        if (!currentAnim)
                            return;
                        std::map<std::string, std::string> params = currentAnim->params;
                        params[paramName] = paramValueStr;
                        m_demoDocument->setAnimationParams(capturedAnimId, params);
                        AnimationManageWidget* animWidget = m_demoWindow->animationManageWidget();
                        if (animWidget)
                            animWidget->selectAnimationById(capturedAnimId);
                    } });
            }
        }
    }

    m_steps.push_back({ tr("Replay Steps Complete!"), [this]() {
                           closePropertyWidget();
                       } });
}

void StepsReplayWindow::updateStepDisplay()
{
    if (m_currentStep < 0) {
        m_stepTitleLabel->setText(tr("Ready to start replay"));
        m_stepProgressLabel->setText(tr("%1 steps total").arg(m_steps.size()));
    } else if (m_currentStep < (int)m_steps.size()) {
        m_stepTitleLabel->setText(m_steps[m_currentStep].title);
        m_stepProgressLabel->setText(tr("Step %1 / %2").arg(m_currentStep + 1).arg(m_steps.size()));
    } else {
        m_stepTitleLabel->setText(tr("Replay Steps Complete!"));
        m_stepProgressLabel->clear();
        m_nextButton->setEnabled(false);
        m_playPauseButton->setEnabled(false);
        m_autoPlayTimer->stop();
        m_playing = false;
    }
}

void StepsReplayWindow::executeNextStep()
{
    if (m_waitingForRig)
        return;
    if (!m_demoDocument) {
        m_autoPlayTimer->stop();
        return;
    }

    m_currentStep++;
    if (m_currentStep >= (int)m_steps.size()) {
        updateStepDisplay();
        return;
    }

    m_steps[m_currentStep].execute();
    updateStepDisplay();
}

void StepsReplayWindow::togglePlayPause()
{
    m_playing = !m_playing;
    if (m_playing) {
        m_playPauseButton->setText(tr("Pause"));
        if (!m_waitingForRig)
            m_autoPlayTimer->start();
    } else {
        m_playPauseButton->setText(tr("Play"));
        m_autoPlayTimer->stop();
    }
}

void StepsReplayWindow::speedChanged(int value)
{
    // value 1-20: maps to 3000ms down to 50ms
    int interval = std::max(50, 3150 - value * 155);
    m_autoPlayTimer->setInterval(interval);
    double speed = value * 0.5;
    m_speedLabel->setText(QString("%1x").arg(speed, 0, 'f', 1));
}
