#include <QGuiApplication>
#include <QElapsedTimer>
#include <QDebug>
#include <QFile>
#include <dust3d/base/ds3_file.h>
#include <dust3d/base/snapshot_xml.h>
#include "material_previews_generator.h"
#include "mesh_generator.h"
#include "texture_generator.h"
#include "image_forever.h"
#include "mesh_result_post_processor.h"

MaterialPreviewsGenerator::MaterialPreviewsGenerator()
{
}

MaterialPreviewsGenerator::~MaterialPreviewsGenerator()
{
    for (auto &item: m_previews) {
        delete item.second;
    }
}

void MaterialPreviewsGenerator::addMaterial(dust3d::Uuid materialId, const std::vector<MaterialLayer> &layers)
{
    m_materials.push_back({materialId, layers});
}

const std::set<dust3d::Uuid> &MaterialPreviewsGenerator::generatedPreviewMaterialIds()
{
    return m_generatedMaterialIds;
}

Model *MaterialPreviewsGenerator::takePreview(dust3d::Uuid materialId)
{
    Model *resultMesh = m_previews[materialId];
    m_previews[materialId] = nullptr;
    return resultMesh;
}

void MaterialPreviewsGenerator::generate()
{
    dust3d::Snapshot *snapshot = new dust3d::Snapshot;
    
    QFile file(":/resources/material-demo-model.ds3");
    file.open(QFile::ReadOnly);
    QByteArray fileData = file.readAll();
    
    std::vector<dust3d::Uuid> partIds;
    dust3d::Ds3FileReader ds3Reader((const std::uint8_t *)fileData.data(), fileData.size());
    for (int i = 0; i < ds3Reader.items().size(); ++i) {
        dust3d::Ds3ReaderItem item = ds3Reader.items().at(i);
        if (item.type == "model") {
            std::vector<std::uint8_t> data;
            ds3Reader.loadItem(item.name, &data);
            std::string xmlString((char *)data.data(), data.size());
            dust3d::loadSnapshotFromXmlString(snapshot, (char *)xmlString.c_str());
            for (const auto &item: snapshot->parts) {
                partIds.push_back(dust3d::Uuid(item.first));
            }
        }
    }
    
    dust3d::MeshGenerator::GeneratedCacheContext *cacheContext = new dust3d::MeshGenerator::GeneratedCacheContext();
    MeshGenerator *meshGenerator = new MeshGenerator(snapshot);
    meshGenerator->setGeneratedCacheContext(cacheContext);
    
    meshGenerator->generate();
    for (const auto &mirror: cacheContext->partMirrorIdMap) {
        partIds.push_back(dust3d::Uuid(mirror.first));
    }
    
    dust3d::Object *object = meshGenerator->takeObject();
    if (nullptr != object) {
        MeshResultPostProcessor *poseProcessor = new MeshResultPostProcessor(*object);
        poseProcessor->poseProcess();
        delete object;
        object = poseProcessor->takePostProcessedObject();
        delete poseProcessor;
    }
    
    if (nullptr != object) {
        for (const auto &material: m_materials) {
            TextureGenerator *textureGenerator = new TextureGenerator(*object);
            for (const auto &layer: material.second) {
                for (const auto &mapItem: layer.maps) {
                    const QImage *image = ImageForever::get(mapItem.imageId);
                    if (nullptr == image)
                        continue;
                    for (const auto &partId: partIds) {
                        if (dust3d::TextureType::BaseColor == mapItem.forWhat)
                            textureGenerator->addPartColorMap(partId, image, layer.tileScale);
                        else if (dust3d::TextureType::Normal == mapItem.forWhat)
                            textureGenerator->addPartNormalMap(partId, image, layer.tileScale);
                        else if (dust3d::TextureType::Metallic == mapItem.forWhat)
                            textureGenerator->addPartMetalnessMap(partId, image, layer.tileScale);
                        else if (dust3d::TextureType::Roughness == mapItem.forWhat)
                            textureGenerator->addPartRoughnessMap(partId, image, layer.tileScale);
                        else if (dust3d::TextureType::AmbientOcclusion == mapItem.forWhat)
                            textureGenerator->addPartAmbientOcclusionMap(partId, image, layer.tileScale);
                    }
                }
            }
            textureGenerator->generate();
            Model *texturedResultMesh = textureGenerator->takeResultMesh();
            if (nullptr != texturedResultMesh) {
                m_previews[material.first] = new Model(*texturedResultMesh);
                m_generatedMaterialIds.insert(material.first);
                delete texturedResultMesh;
            }
            delete textureGenerator;
        }
    }
    
    delete object;
    
    delete meshGenerator;
    delete cacheContext;
}

void MaterialPreviewsGenerator::process()
{
    QElapsedTimer countTimeConsumed;
    countTimeConsumed.start();

    generate();

    qDebug() << "The material previews generation took" << countTimeConsumed.elapsed() << "milliseconds";

    emit finished();
}
