#ifndef MATERIAL_PREVIEWS_GENERATOR_H
#define MATERIAL_PREVIEWS_GENERATOR_H
#include <QObject>
#include <map>
#include <QUuid>
#include <vector>
#include "meshloader.h"
#include "skeletondocument.h"

class MaterialPreviewsGenerator : public QObject
{
    Q_OBJECT
public:
    MaterialPreviewsGenerator();
    ~MaterialPreviewsGenerator();
    void addMaterial(QUuid materialId, const std::vector<SkeletonMaterialLayer> &layers);
    const std::set<QUuid> &generatedPreviewMaterialIds();
    MeshLoader *takePreview(QUuid materialId);
    void generate();
signals:
    void finished();
public slots:
    void process();
private:
    std::vector<std::pair<QUuid, std::vector<SkeletonMaterialLayer>>> m_materials;
    std::map<QUuid, MeshLoader *> m_previews;
    std::set<QUuid> m_generatedMaterialIds;
};

#endif
