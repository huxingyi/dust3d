#ifndef DUST3D_MATERIAL_PREVIEWS_GENERATOR_H
#define DUST3D_MATERIAL_PREVIEWS_GENERATOR_H
#include <QObject>
#include <map>
#include <QUuid>
#include <vector>
#include "model.h"
#include "document.h"

class MaterialPreviewsGenerator : public QObject
{
    Q_OBJECT
public:
    MaterialPreviewsGenerator();
    ~MaterialPreviewsGenerator();
    void addMaterial(QUuid materialId, const std::vector<MaterialLayer> &layers);
    const std::set<QUuid> &generatedPreviewMaterialIds();
    Model *takePreview(QUuid materialId);
    void generate();
signals:
    void finished();
public slots:
    void process();
private:
    std::vector<std::pair<QUuid, std::vector<MaterialLayer>>> m_materials;
    std::map<QUuid, Model *> m_previews;
    std::set<QUuid> m_generatedMaterialIds;
};

#endif
