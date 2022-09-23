#ifndef DUST3D_APPLICATION_MATERIAL_PREVIEWS_GENERATOR_H_
#define DUST3D_APPLICATION_MATERIAL_PREVIEWS_GENERATOR_H_

#include <QObject>
#include <map>
#include <vector>
#include "model_mesh.h"
#include "material_layer.h"

class MaterialPreviewsGenerator : public QObject
{
    Q_OBJECT
public:
    MaterialPreviewsGenerator();
    ~MaterialPreviewsGenerator();
    void addMaterial(dust3d::Uuid materialId, const std::vector<MaterialLayer> &layers);
    const std::set<dust3d::Uuid> &generatedPreviewMaterialIds();
    ModelMesh *takePreview(dust3d::Uuid materialId);
    void generate();
signals:
    void finished();
public slots:
    void process();
private:
    std::vector<std::pair<dust3d::Uuid, std::vector<MaterialLayer>>> m_materials;
    std::map<dust3d::Uuid, ModelMesh *> m_previews;
    std::set<dust3d::Uuid> m_generatedMaterialIds;
};

#endif
