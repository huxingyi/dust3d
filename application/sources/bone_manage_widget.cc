#include "bone_manage_widget.h"
#include "document.h"
#include "model_mesh.h"
#include "skeleton_graphics_widget.h"
#include "theme.h"
#include <QComboBox>
#include <QDebug>
#include <QFile>
#include <QHBoxLayout>
#include <QIcon>
#include <QItemSelectionModel>
#include <QLabel>
#include <QMap>
#include <QMenu>
#include <QPushButton>
#include <QStandardItem>
#include <QStandardItemModel>
#include <QThread>
#include <QTreeView>
#include <QVBoxLayout>
#include <rapidxml.hpp>
#include <string>

BoneManageWidget::BoneManageWidget(Document* document, QWidget* parent)
    : QWidget(parent)
    , m_document(document)
{
    setContextMenuPolicy(Qt::CustomContextMenu);

    // Load rig structures from XML files
    loadRigStructures();

    QVBoxLayout* mainLayout = new QVBoxLayout;
    mainLayout->setSpacing(5);
    mainLayout->setContentsMargins(5, 5, 5, 5);

    // Rig Type Selection
    QHBoxLayout* rigTypeLayout = new QHBoxLayout;
    rigTypeLayout->setSpacing(5);
    rigTypeLayout->setContentsMargins(0, 0, 0, 0);

    m_rigTypeComboBox = new QComboBox();

    // Add loaded rig types to combo box
    m_rigTypeComboBox->addItem(tr("None"));
    for (const auto& entry : m_rigStructures) {
        m_rigTypeComboBox->addItem(entry.second.name);
    }

    rigTypeLayout->addWidget(m_rigTypeComboBox);
    rigTypeLayout->addStretch();

    // Add to main layout
    mainLayout->addLayout(rigTypeLayout);

    // Bone Tree View
    m_boneTreeView = new QTreeView();
    m_boneTreeModel = new QStandardItemModel();
    m_boneTreeView->setModel(m_boneTreeModel);
    m_boneTreeView->setHeaderHidden(true);
    m_boneTreeView->setMinimumHeight(200);
    m_boneTreeView->setEditTriggers(QAbstractItemView::NoEditTriggers);

    mainLayout->addWidget(m_boneTreeView);

    // Assign button to assign selected edges to the selected bone
    m_assignButton = new QPushButton(tr("Assign Selected Edges to Bone"));
    m_assignButton->setToolTip(tr("Assign the selected edges from the canvas to the selected bone"));
    m_assignButton->setEnabled(false);
    mainLayout->addWidget(m_assignButton);

    // Model Widget for rendering the rig skeleton mesh
    m_rigTemplateModelWidget = new ModelWidget();
    m_rigTemplateModelWidget->setMinimumHeight(250);
    m_rigTemplateModelWidget->enableZoom(false); // Only allow rotation, disable zoom

    m_rigTemplateGroupBox = new QGroupBox(tr("Template"));
    QVBoxLayout* rigTemplateLayout = new QVBoxLayout;
    rigTemplateLayout->setContentsMargins(3, 3, 3, 3);
    rigTemplateLayout->addWidget(m_rigTemplateModelWidget);
    m_rigTemplateGroupBox->setLayout(rigTemplateLayout);
    mainLayout->addWidget(m_rigTemplateGroupBox);

    // Model Widget for rendering the actual rig skeleton mesh (computed from edge assignments)
    m_rigSkinningModelWidget = new ModelWidget();
    m_rigSkinningModelWidget->setMinimumHeight(250);
    m_rigSkinningModelWidget->enableZoom(false);
    m_rigSkinningModelWidget->disableCullFace();

    m_rigSkinningGroupBox = new QGroupBox(tr("Skinned"));
    QVBoxLayout* rigSkinningLayout = new QVBoxLayout;
    rigSkinningLayout->setContentsMargins(3, 3, 3, 3);
    rigSkinningLayout->addWidget(m_rigSkinningModelWidget);
    m_rigSkinningGroupBox->setLayout(rigSkinningLayout);
    mainLayout->addWidget(m_rigSkinningGroupBox);

    mainLayout->addStretch();

    setLayout(mainLayout);

    // Initialize combo box with current document rigType
    QString currentRigType = m_document->getRigType();
    int index = 0;
    for (int i = 0; i < m_rigTypeComboBox->count(); ++i) {
        if (m_rigTypeComboBox->itemText(i).compare(currentRigType, Qt::CaseInsensitive) == 0) {
            index = i;
            break;
        }
    }
    m_rigTypeComboBox->setCurrentIndex(index);

    // Connect signals
    // Combo box changes trigger document's setRigType slot
    connect(m_rigTypeComboBox, QOverload<const QString&>::of(&QComboBox::currentTextChanged),
        m_document, &Document::setRigType);

    // Document's rigTypeChanged signal updates the bone tree view
    connect(m_document, &Document::rigTypeChanged,
        this, &BoneManageWidget::onRigTypeChanged);

    connect(this, &QWidget::customContextMenuRequested, this, &BoneManageWidget::showContextMenu);

    // Connect tree view selection changes to update the highlighted bone
    connect(m_boneTreeView->selectionModel(), &QItemSelectionModel::selectionChanged,
        this, &BoneManageWidget::onBoneSelectionChanged);

    // Connect button click to assign selected edges to bone
    connect(m_assignButton, &QPushButton::clicked,
        this, &BoneManageWidget::assignSelectedEdgesToBone);

    // Connect rig generation ready signal to update actual rig model widget
    connect(m_document, &Document::resultRigChanged,
        this, &BoneManageWidget::onRigGenerationReady);

    // Initialize tree view with current rig type
    updateBoneTreeView(currentRigType);
}

BoneManageWidget::~BoneManageWidget()
{
}

void BoneManageWidget::showContextMenu(const QPoint& pos)
{
    if (!m_contextMenu) {
        m_contextMenu = std::make_unique<QMenu>();
        // TODO: Add context menu items
    }

    if (!m_contextMenu->isEmpty())
        m_contextMenu->popup(mapToGlobal(pos));
}

void BoneManageWidget::loadRigStructures()
{
    // Load all rig structure XML files from resources
    const QStringList rigFiles = {
        ":/resources/rig_human.xml",
        ":/resources/rig_quad.xml",
        ":/resources/rig_bird.xml",
        ":/resources/rig_fish.xml",
        ":/resources/rig_fly.xml",
        ":/resources/rig_generic.xml"
    };

    for (const auto& filePath : rigFiles) {
        loadRigFromXml(filePath);
    }

    qDebug() << "Loaded" << m_rigStructures.size() << "rig structures";
}

bool BoneManageWidget::loadRigFromXml(const QString& filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "Failed to open rig file:" << filePath;
        return false;
    }

    // Read entire file into memory
    QByteArray xmlData = file.readAll();
    file.close();

    if (xmlData.isEmpty()) {
        qWarning() << "Rig file is empty:" << filePath;
        return false;
    }

    try {
        // Parse XML with rapidxml
        rapidxml::xml_document<> doc;
        doc.parse<0>(xmlData.data());

        // Get root element
        rapidxml::xml_node<>* rigElement = doc.first_node("rig");
        if (!rigElement) {
            qWarning() << "Invalid rig file format (missing <rig> root):" << filePath;
            return false;
        }

        RigStructure rigStruct;

        // Get attributes
        rapidxml::xml_attribute<>* typeAttr = rigElement->first_attribute("type");
        rapidxml::xml_attribute<>* nameAttr = rigElement->first_attribute("name");

        if (typeAttr) {
            rigStruct.type = QString::fromStdString(std::string(typeAttr->value(), typeAttr->value_size()));
        }
        if (nameAttr) {
            rigStruct.name = QString::fromStdString(std::string(nameAttr->value(), nameAttr->value_size()));
        }

        // Get description
        rapidxml::xml_node<>* descElement = rigElement->first_node("description");
        if (descElement && descElement->value_size() > 0) {
            rigStruct.description = QString::fromStdString(std::string(descElement->value(), descElement->value_size()));
        }

        // Parse bones
        for (rapidxml::xml_node<>* boneElement = rigElement->first_node("bone");
            boneElement;
            boneElement = boneElement->next_sibling("bone")) {

            BoneNode bone;

            // Get bone attributes
            rapidxml::xml_attribute<>* boneName = boneElement->first_attribute("name");
            rapidxml::xml_attribute<>* boneParent = boneElement->first_attribute("parent");

            if (boneName) {
                bone.name = QString::fromStdString(std::string(boneName->value(), boneName->value_size()));
            }
            if (boneParent) {
                bone.parent = QString::fromStdString(std::string(boneParent->value(), boneParent->value_size()));
            }

            // Get position
            rapidxml::xml_node<>* posElement = boneElement->first_node("position");
            if (posElement) {
                rapidxml::xml_attribute<>* xAttr = posElement->first_attribute("x");
                rapidxml::xml_attribute<>* yAttr = posElement->first_attribute("y");
                rapidxml::xml_attribute<>* zAttr = posElement->first_attribute("z");

                if (xAttr)
                    bone.posX = QString::fromStdString(std::string(xAttr->value(), xAttr->value_size())).toFloat();
                if (yAttr)
                    bone.posY = QString::fromStdString(std::string(yAttr->value(), yAttr->value_size())).toFloat();
                if (zAttr)
                    bone.posZ = QString::fromStdString(std::string(zAttr->value(), zAttr->value_size())).toFloat();
            }

            // Get endPosition
            rapidxml::xml_node<>* endPosElement = boneElement->first_node("endPosition");
            if (endPosElement) {
                rapidxml::xml_attribute<>* xAttr = endPosElement->first_attribute("x");
                rapidxml::xml_attribute<>* yAttr = endPosElement->first_attribute("y");
                rapidxml::xml_attribute<>* zAttr = endPosElement->first_attribute("z");

                if (xAttr)
                    bone.endX = QString::fromStdString(std::string(xAttr->value(), xAttr->value_size())).toFloat();
                if (yAttr)
                    bone.endY = QString::fromStdString(std::string(yAttr->value(), yAttr->value_size())).toFloat();
                if (zAttr)
                    bone.endZ = QString::fromStdString(std::string(zAttr->value(), zAttr->value_size())).toFloat();
            }

            rigStruct.bones.push_back(bone);
        }

        if (!rigStruct.type.isEmpty()) {
            m_rigStructures[rigStruct.type] = rigStruct;
            qDebug() << "Loaded rig:" << rigStruct.type << "-" << rigStruct.name
                     << "with" << rigStruct.bones.size() << "bones";
            return true;
        }

    } catch (const std::exception& e) {
        qWarning() << "Failed to parse XML in" << filePath << "Error:" << e.what();
        return false;
    }

    return false;
}

void BoneManageWidget::onRigTypeChanged(const QString& rigType)
{
    // Update the combo box selection to match the document's current rig type
    int index = 0;
    for (int i = 0; i < m_rigTypeComboBox->count(); ++i) {
        if (m_rigTypeComboBox->itemText(i).compare(rigType, Qt::CaseInsensitive) == 0) {
            index = i;
            break;
        }
    }
    m_rigTypeComboBox->setCurrentIndex(index);

    updateBoneTreeView(rigType);
}

void BoneManageWidget::onBoneSelectionChanged()
{
    QModelIndexList selectedIndexes = m_boneTreeView->selectionModel()->selectedIndexes();
    QString selectedBoneName;

    if (!selectedIndexes.isEmpty()) {
        QModelIndex selectedIndex = selectedIndexes.first();
        QStandardItem* selectedItem = m_boneTreeModel->itemFromIndex(selectedIndex);
        if (selectedItem) {
            selectedBoneName = selectedItem->text();
        }
    }

    m_selectedBoneName = selectedBoneName;

    // Regenerate mesh with the selected bone highlighted
    QString currentRigType = m_rigTypeComboBox->currentText();
    generateRigTemplateMesh(currentRigType, selectedBoneName);

    generateRigSkinningMesh();

    updateAssignButtonState();
}

void BoneManageWidget::updateBoneTreeView(const QString& rigType)
{
    m_boneTreeModel->clear();

    if (rigType == tr("None")) {
        m_boneTreeView->hide();
        m_rigTemplateModelWidget->hide();
        m_rigSkinningModelWidget->hide();
        m_assignButton->hide();
        m_rigSkinningGroupBox->hide();
        m_rigTemplateGroupBox->hide();
        return;
    }

    m_boneTreeView->show();
    m_rigTemplateModelWidget->show();
    m_rigSkinningModelWidget->show();
    m_assignButton->show();
    m_rigSkinningGroupBox->show();
    m_rigTemplateGroupBox->show();

    // Find the rig structure by name
    RigStructure* selectedRig = nullptr;
    for (auto& entry : m_rigStructures) {
        if (entry.second.name == rigType) {
            selectedRig = &entry.second;
            break;
        }
    }

    if (!selectedRig) {
        return;
    }

    // Create a map of bone name to item for hierarchy building
    std::map<QString, QStandardItem*> boneItems;

    // First pass: create all items
    for (const auto& bone : selectedRig->bones) {
        QStandardItem* item = new QStandardItem(bone.name);
        boneItems[bone.name] = item;
    }

    // Second pass: build hierarchy
    QStandardItem* rootItem = m_boneTreeModel->invisibleRootItem();
    for (const auto& bone : selectedRig->bones) {
        QStandardItem* item = boneItems[bone.name];
        if (bone.parent.isEmpty()) {
            // Root bone
            rootItem->appendRow(item);
        } else {
            // Child bone
            auto parentIt = boneItems.find(bone.parent);
            if (parentIt != boneItems.end()) {
                parentIt->second->appendRow(item);
            } else {
                // Parent not found, add to root
                rootItem->appendRow(item);
            }
        }
    }

    m_boneTreeView->expandAll();

    // Generate the rig skeleton mesh for visualization
    generateRigTemplateMesh(rigType, m_selectedBoneName);
}

void BoneManageWidget::generateRigTemplateMesh(const QString& rigType, const QString& selectedBoneName)
{
    if (rigType == tr("None")) {
        return;
    }

    // Find the rig structure by name
    RigStructure* selectedRig = nullptr;
    for (auto& entry : m_rigStructures) {
        if (entry.second.name == rigType) {
            selectedRig = &entry.second;
            break;
        }
    }

    if (!selectedRig || selectedRig->bones.empty()) {
        return;
    }

    // If a mesh generation thread is already running, mark as obsolete instead of waiting
    if (nullptr != m_rigTemplateMeshWorker) {
        m_rigTemplateMeshObsolete = true;
        m_pendingRigType = rigType;
        m_pendingSelectedBoneName = selectedBoneName;
        qDebug() << "Mesh generation already in progress, marking as obsolete";
        return;
    }

    m_rigTemplateMeshObsolete = false;

    // Create a new worker and thread
    m_rigTemplateMeshWorker = std::make_unique<RigSkeletonMeshWorker>();
    m_rigTemplateMeshWorker->setParameters(*selectedRig, selectedBoneName, 0.02);

    auto thread = new QThread;
    m_rigTemplateMeshWorker->moveToThread(thread);

    // Connect signals
    connect(thread, &QThread::started, m_rigTemplateMeshWorker.get(), &RigSkeletonMeshWorker::process);
    connect(m_rigTemplateMeshWorker.get(), &RigSkeletonMeshWorker::finished, this, &BoneManageWidget::rigSkeletonTemplateMeshReady);
    connect(m_rigTemplateMeshWorker.get(), &RigSkeletonMeshWorker::finished, thread, &QThread::quit);
    connect(thread, &QThread::finished, thread, &QThread::deleteLater);

    qDebug() << "Starting threaded mesh generation for" << rigType;
    thread->start();
}

void BoneManageWidget::rigSkeletonTemplateMeshReady()
{
    const auto& vertices = m_rigTemplateMeshWorker->getVertices();
    const auto& faces = m_rigTemplateMeshWorker->getFaces();

    if (vertices.empty() || faces.empty()) {
        qWarning() << "Failed to generate rig skeleton mesh";
    } else {
        // Generate normals for each triangle face
        std::vector<std::vector<dust3d::Vector3>> triangleVertexNormals;
        for (const auto& face : faces) {
            if (face.size() >= 3) {
                dust3d::Vector3 faceNormal = dust3d::Vector3::normal(
                    vertices[face[0]], vertices[face[1]], vertices[face[2]]);
                triangleVertexNormals.push_back({ faceNormal, faceNormal, faceNormal });
            }
        }

        // Get per-vertex properties if available (for bone highlighting)
        const auto* vertexProperties = m_rigTemplateMeshWorker->getVertexProperties();

        ModelMesh* rigSkeletonMesh = new ModelMesh(vertices, faces, triangleVertexNormals,
            dust3d::Color(Theme::green.redF(), Theme::green.greenF(), Theme::green.blueF()), // Default light gray color for skeleton
            0.0, // No metalness
            1.0, // Full roughness
            vertexProperties); // Use per-vertex properties if available

        // Update the model widget to display the generated mesh
        if (m_rigTemplateModelWidget) {
            m_rigTemplateModelWidget->updateMesh(rigSkeletonMesh);
        }

        qDebug() << "Rig skeleton mesh generated successfully"
            << "with" << vertices.size() << "vertices and" << faces.size() << "faces";
    }

    m_rigTemplateMeshWorker.reset();

    if (m_rigTemplateMeshObsolete) {
        generateRigTemplateMesh(m_pendingRigType, m_pendingSelectedBoneName);
    }
}

void BoneManageWidget::setSkeletonGraphicsWidget(SkeletonGraphicsWidget* graphicsWidget)
{
    m_skeletonGraphicsWidget = graphicsWidget;

    if (m_skeletonGraphicsWidget) {
        connect(m_skeletonGraphicsWidget, &SkeletonGraphicsWidget::skeletonSelectionChanged,
            this, &BoneManageWidget::updateAssignButtonState);
    }

    updateAssignButtonState();
}

void BoneManageWidget::assignSelectedEdgesToBone()
{
    if (!m_skeletonGraphicsWidget || m_selectedBoneName.isEmpty()) {
        qWarning() << "Cannot assign edges: graphics widget or selected bone name is empty";
        return;
    }

    // Get the selected edge IDs from the graphics widget
    std::set<dust3d::Uuid> selectedEdgeIds = m_skeletonGraphicsWidget->getSelectedEdgeIds();

    if (selectedEdgeIds.empty()) {
        qWarning() << "No edges selected on the canvas";
        return;
    }

    // Assign each selected edge to the selected bone
    for (const auto& edgeId : selectedEdgeIds) {
        m_document->setEdgeBoneName(edgeId, m_selectedBoneName);
    }

    m_document->saveSnapshot();

    qDebug() << "Assigned" << selectedEdgeIds.size() << "edges to bone:" << m_selectedBoneName;
}

void BoneManageWidget::onRigGenerationReady()
{
    generateRigSkinningMesh();
}

void BoneManageWidget::updateAssignButtonState()
{
    bool hasBone = !m_selectedBoneName.isEmpty();
    bool hasEdgeSelection = m_skeletonGraphicsWidget ? m_skeletonGraphicsWidget->hasEdgeSelection() : false;
    if (m_assignButton)
        m_assignButton->setEnabled(isVisible() && hasBone && hasEdgeSelection);
}

void BoneManageWidget::rigSkinningMeshReady()
{
    // Use combined mesh built in the worker thread
    ModelOpenGLVertex* combinedVertices = m_rigSkinningMeshWorker->takeCombinedVertices();
    int combinedVertexCount = m_rigSkinningMeshWorker->getCombinedVertexCount();

    if (combinedVertices && combinedVertexCount > 0) {
        ModelMesh* combinedMesh = new ModelMesh(combinedVertices, combinedVertexCount);
        if (m_rigSkinningModelWidget) {
            m_rigSkinningModelWidget->updateMesh(combinedMesh);
        }
    }

    m_rigSkinningMeshWorker.reset();

    qDebug() << "Actual rig mesh updated with" << combinedVertexCount << "combined vertices";

    if (m_rigSkinningMeshObsolete) {
        generateRigSkinningMesh();
    }
}

void BoneManageWidget::generateRigSkinningMesh()
{
    if (nullptr != m_rigSkinningMeshWorker) {
        m_rigSkinningMeshObsolete = true;
        return;
    }

    m_rigSkinningMeshObsolete = false;

    const RigStructure& actualRig = m_document->getActualRigStructure();
    if (actualRig.bones.empty())
        return;

    dust3d::Object* rigObject = m_document->takeRigObject();

    m_rigSkinningMeshWorker = std::make_unique<RigSkeletonMeshWorker>();
    m_rigSkinningMeshWorker->setParameters(actualRig, m_selectedBoneName, 0.02);
    m_rigSkinningMeshWorker->setRigObject(rigObject, m_selectedBoneName);

    auto thread = new QThread;
    m_rigSkinningMeshWorker->moveToThread(thread);

    connect(thread, &QThread::started, m_rigSkinningMeshWorker.get(), &RigSkeletonMeshWorker::process);
    connect(m_rigSkinningMeshWorker.get(), &RigSkeletonMeshWorker::finished, this, &BoneManageWidget::rigSkinningMeshReady);
    connect(m_rigSkinningMeshWorker.get(), &RigSkeletonMeshWorker::finished, thread, &QThread::quit);
    connect(thread, &QThread::finished, thread, &QThread::deleteLater);

    thread->start();
}

