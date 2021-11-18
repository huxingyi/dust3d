#ifndef DUST3D_APPLICATION_MATERIAL_EDIT_WIDGET_H_
#define DUST3D_APPLICATION_MATERIAL_EDIT_WIDGET_H_

#include <QDialog>
#include <map>
#include <QCloseEvent>
#include <QLineEdit>
#include "model_widget.h"
#include "material_previews_generator.h"
#include "image_preview_widget.h"
#include "float_number_widget.h"
#include "material_layer.h"

class Document;

enum class PopupWidgetType
{
    PitchYawRoll,
    Intersection
};

class MaterialEditWidget : public QDialog
{
    Q_OBJECT
signals:
    void addMaterial(dust3d::Uuid materialId, QString name, std::vector<MaterialLayer> layers);
    void removeMaterial(dust3d::Uuid materialId);
    void setMaterialLayers(dust3d::Uuid materialId, std::vector<MaterialLayer> layers);
    void renameMaterial(dust3d::Uuid materialId, QString name);
    void layersAdjusted();
public:
    MaterialEditWidget(const Document *document, QWidget *parent=nullptr);
    ~MaterialEditWidget();
public slots:
    void updatePreview();
    void setEditMaterialId(dust3d::Uuid materialId);
    void setEditMaterialName(QString name);
    void setEditMaterialLayers(std::vector<MaterialLayer> layers);
    void updateTitle();
    void save();
    void clearUnsaveState();
    void previewReady();
protected:
    QSize sizeHint() const override;
    void closeEvent(QCloseEvent *event) override;
    void reject() override;
private:
    void updateMapButtonBackground(ImagePreviewWidget *button, const QImage *image);
    ImagePreviewWidget *createMapButton();
    QImage *pickImage();
    const Document *m_document = nullptr;
    MaterialPreviewsGenerator *m_materialPreviewsGenerator = nullptr;
    ModelWidget *m_previewWidget = nullptr;
    FloatNumberWidget *m_tileScaleSlider = nullptr;
    bool m_isPreviewDirty = false;
    bool m_closed = false;
    dust3d::Uuid m_materialId;
    bool m_unsaved = false;
    QLineEdit *m_nameEdit = nullptr;
    std::vector<MaterialLayer> m_layers;
    ImagePreviewWidget *m_textureMapButtons[(int)dust3d::TextureType::Count - 1] = {nullptr};
};

#endif
