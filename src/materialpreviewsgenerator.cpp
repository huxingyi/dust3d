#include <QGuiApplication>
#include <QElapsedTimer>
#include "materialpreviewsgenerator.h"
#include "meshgenerator.h"
#include "snapshotxml.h"
#include "ds3file.h"
#include "texturegenerator.h"
#include "imageforever.h"
#include "meshresultpostprocessor.h"

MaterialPreviewsGenerator::MaterialPreviewsGenerator()
{
}

MaterialPreviewsGenerator::~MaterialPreviewsGenerator()
{
    for (auto &item: m_previews) {
        delete item.second;
    }
}

void MaterialPreviewsGenerator::addMaterial(QUuid materialId, const std::vector<MaterialLayer> &layers)
{
    m_materials.push_back({materialId, layers});
}

const std::set<QUuid> &MaterialPreviewsGenerator::generatedPreviewMaterialIds()
{
    return m_generatedMaterialIds;
}

MeshLoader *MaterialPreviewsGenerator::takePreview(QUuid materialId)
{
    MeshLoader *resultMesh = m_previews[materialId];
    m_previews[materialId] = nullptr;
    return resultMesh;
}

void MaterialPreviewsGenerator::generate()
{
    Snapshot *snapshot = new Snapshot;
    
    std::vector<QUuid> partIds;
    Ds3FileReader ds3Reader(":/resources/material-demo-model.ds3");
    for (int i = 0; i < ds3Reader.items().size(); ++i) {
        Ds3ReaderItem item = ds3Reader.items().at(i);
        if (item.type == "model") {
            QByteArray data;
            ds3Reader.loadItem(item.name, &data);
            QXmlStreamReader stream(data);
            loadSkeletonFromXmlStream(snapshot, stream);
            for (const auto &item: snapshot->parts) {
                partIds.push_back(QUuid(item.first));
            }
        }
    }
    
    GeneratedCacheContext *cacheContext = new GeneratedCacheContext();
    MeshGenerator *meshGenerator = new MeshGenerator(snapshot);
    meshGenerator->setGeneratedCacheContext(cacheContext);
    
    meshGenerator->generate();
    for (const auto &mirror: cacheContext->partMirrorIdMap) {
        partIds.push_back(QUuid(mirror.first));
    }
    
    Outcome *outcome = meshGenerator->takeOutcome();
    if (nullptr != outcome) {
        MeshResultPostProcessor *poseProcessor = new MeshResultPostProcessor(*outcome);
        poseProcessor->poseProcess();
        delete outcome;
        outcome = poseProcessor->takePostProcessedOutcome();
        delete poseProcessor;
    }
    
    if (nullptr != outcome) {
        for (const auto &material: m_materials) {
            TextureGenerator *textureGenerator = new TextureGenerator(*outcome);
            for (const auto &layer: material.second) {
                for (const auto &mapItem: layer.maps) {
                    const QImage *image = ImageForever::get(mapItem.imageId);
                    if (nullptr == image)
                        continue;
                    for (const auto &partId: partIds) {
                        if (TextureType::BaseColor == mapItem.forWhat)
                            textureGenerator->addPartColorMap(partId, image, layer.tileScale);
                        else if (TextureType::Normal == mapItem.forWhat)
                            textureGenerator->addPartNormalMap(partId, image, layer.tileScale);
                        else if (TextureType::Metalness == mapItem.forWhat)
                            textureGenerator->addPartMetalnessMap(partId, image, layer.tileScale);
                        else if (TextureType::Roughness == mapItem.forWhat)
                            textureGenerator->addPartRoughnessMap(partId, image, layer.tileScale);
                        else if (TextureType::AmbientOcclusion == mapItem.forWhat)
                            textureGenerator->addPartAmbientOcclusionMap(partId, image, layer.tileScale);
                    }
                }
            }
            textureGenerator->generate();
            MeshLoader *texturedResultMesh = textureGenerator->takeResultMesh();
            if (nullptr != texturedResultMesh) {
                m_previews[material.first] = new MeshLoader(*texturedResultMesh);
                m_generatedMaterialIds.insert(material.first);
                delete texturedResultMesh;
            }
            delete textureGenerator;
        }
    }
    
    delete outcome;
    
    delete meshGenerator;
    delete cacheContext;
}

void MaterialPreviewsGenerator::process()
{
    QElapsedTimer countTimeConsumed;
    countTimeConsumed.start();

    generate();

    qDebug() << "The material previews generation took" << countTimeConsumed.elapsed() << "milliseconds";

    this->moveToThread(QGuiApplication::instance()->thread());
    emit finished();
}
