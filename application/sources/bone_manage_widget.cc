#include "bone_manage_widget.h"
#include "document.h"
#include "theme.h"
#include <QComboBox>
#include <QLabel>
#include <QMenu>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFile>
#include <QIcon>
#include <QMap>
#include <QDebug>
#include <QTreeView>
#include <QStandardItemModel>
#include <QStandardItem>
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

    QLabel* rigTypeLabel = new QLabel(tr("Rig type:"));
    m_rigTypeComboBox = new QComboBox();

    // Add loaded rig types to combo box
    m_rigTypeComboBox->addItem(tr("None"));
    for (const auto& entry : m_rigStructures) {
        m_rigTypeComboBox->addItem(entry.second.name);
    }

    rigTypeLayout->addWidget(rigTypeLabel);
    rigTypeLayout->addWidget(m_rigTypeComboBox);
    rigTypeLayout->addStretch();

    // Add to main layout
    mainLayout->addLayout(rigTypeLayout);

    // Bone Tree View
    QLabel* boneTreeLabel = new QLabel(tr("Bone Hierarchy:"));
    m_boneTreeView = new QTreeView();
    m_boneTreeModel = new QStandardItemModel();
    m_boneTreeView->setModel(m_boneTreeModel);
    m_boneTreeView->setHeaderHidden(true);
    m_boneTreeView->setMinimumHeight(200);
    m_boneTreeView->setEditTriggers(QAbstractItemView::NoEditTriggers);

    mainLayout->addWidget(boneTreeLabel);
    mainLayout->addWidget(m_boneTreeView);

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

    // Initialize tree view with current rig type
    updateBoneTreeView(currentRigType);
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

                if (xAttr) bone.posX = QString::fromStdString(std::string(xAttr->value(), xAttr->value_size())).toFloat();
                if (yAttr) bone.posY = QString::fromStdString(std::string(yAttr->value(), yAttr->value_size())).toFloat();
                if (zAttr) bone.posZ = QString::fromStdString(std::string(zAttr->value(), zAttr->value_size())).toFloat();
            }

            // Get endPosition
            rapidxml::xml_node<>* endPosElement = boneElement->first_node("endPosition");
            if (endPosElement) {
                rapidxml::xml_attribute<>* xAttr = endPosElement->first_attribute("x");
                rapidxml::xml_attribute<>* yAttr = endPosElement->first_attribute("y");
                rapidxml::xml_attribute<>* zAttr = endPosElement->first_attribute("z");

                if (xAttr) bone.endX = QString::fromStdString(std::string(xAttr->value(), xAttr->value_size())).toFloat();
                if (yAttr) bone.endY = QString::fromStdString(std::string(yAttr->value(), yAttr->value_size())).toFloat();
                if (zAttr) bone.endZ = QString::fromStdString(std::string(zAttr->value(), zAttr->value_size())).toFloat();
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

void BoneManageWidget::updateBoneTreeView(const QString& rigType)
{
    m_boneTreeModel->clear();

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
}
