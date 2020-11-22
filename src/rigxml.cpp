#include <stack>
#include <QUuid>
#include <QDebug>
#include <QStringList>
#include <QRegExp>
#include "rigxml.h"
#include "util.h"
#include "jointnodetree.h"

void saveRigToXmlStream(const Object *object,
    const std::vector<RigBone> *rigBones, 
    const std::map<int, RigVertexWeights> *rigWeights,
    QXmlStreamWriter *writer)
{
    JointNodeTree jointNodeTree(rigBones);
    const auto &boneNodes = jointNodeTree.nodes();
    
    writer->setAutoFormatting(true);
    writer->writeStartDocument();
    
    writer->writeStartElement("rig");
        writer->writeStartElement("bones");
        for (size_t boneIndex = 0; boneIndex < rigBones->size(); ++boneIndex) {
            const auto &bone = (*rigBones)[boneIndex];
            const auto &node = boneNodes[boneIndex];
            writer->writeStartElement("bone");
            writer->writeAttribute("name", bone.name);
            writer->writeAttribute("parentIndex", QString::number(bone.parent));
            writer->writeAttribute("headX", QString::number(bone.headPosition.x()));
            writer->writeAttribute("headY", QString::number(bone.headPosition.y()));
            writer->writeAttribute("headZ", QString::number(bone.headPosition.z()));
            writer->writeAttribute("tailX", QString::number(bone.tailPosition.x()));
            writer->writeAttribute("tailY", QString::number(bone.tailPosition.y()));
            writer->writeAttribute("tailZ", QString::number(bone.tailPosition.z()));
            writer->writeAttribute("headRadius", QString::number(bone.headRadius));
            writer->writeAttribute("tailRadius", QString::number(bone.tailRadius));
            writer->writeAttribute("color", bone.color.name(QColor::HexArgb));
            for (const auto &attribute: bone.attributes)
                writer->writeAttribute(attribute.first, attribute.second);
            const float *floatArray = node.inverseBindMatrix.constData();
            QStringList matrixItemList;
            for (auto j = 0u; j < 16; j++) {
                matrixItemList += QString::number(floatArray[j]);
            }
            writer->writeAttribute("inverseBindMatrix", matrixItemList.join(","));
            writer->writeEndElement();
        }
        writer->writeEndElement();
        
        writer->writeStartElement("weights");
        QStringList weightsList;
        for (size_t vertexIndex = 0; vertexIndex < object->vertices.size(); ++vertexIndex) {
            auto findWeights = rigWeights->find(vertexIndex);
            if (findWeights == rigWeights->end()) {
                QStringList vertexWeightsList;
                for (size_t i = 0; i < MAX_WEIGHT_NUM; ++i) {
                    vertexWeightsList += "0,0";
                }
                weightsList += vertexWeightsList.join(",");
            } else {
                QStringList vertexWeightsList;
                for (size_t i = 0; i < MAX_WEIGHT_NUM; ++i) {
                    vertexWeightsList += QString::number(findWeights->second.boneIndices[i]) + "," + QString::number(findWeights->second.boneWeights[i]);
                }
                weightsList += vertexWeightsList.join(",");
            }
        }
        writer->writeCharacters(weightsList.join(" "));
        writer->writeEndElement();
    
    writer->writeEndElement();
    writer->writeEndDocument();
}
