#include <QVBoxLayout>
#include "material_widget.h"
#include "document.h"

MaterialWidget::MaterialWidget(const Document *document, dust3d::Uuid materialId) :
    m_materialId(materialId),
    m_document(document)
{
    setObjectName("MaterialFrame");

    m_previewWidget = new ModelWidget(this);
    m_previewWidget->setAttribute(Qt::WA_TransparentForMouseEvents);
    m_previewWidget->setFixedSize(Theme::materialPreviewImageSize, Theme::materialPreviewImageSize);
    m_previewWidget->enableMove(false);
    m_previewWidget->enableZoom(false);
    m_previewWidget->enableEnvironmentLight();

    m_nameLabel = new QLabel;
    m_nameLabel->setAlignment(Qt::AlignCenter);
    m_nameLabel->setStyleSheet("background: qlineargradient(x1:0.5 y1:-15.5, x2:0.5 y2:1, stop:0 " + Theme::white.name() + ", stop:1 #252525);");

    QFont nameFont;
    nameFont.setWeight(QFont::Light);
    nameFont.setBold(false);
    m_nameLabel->setFont(nameFont);

    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->addStretch();
    mainLayout->addWidget(m_nameLabel);

    setLayout(mainLayout);

    setFixedSize(Theme::materialPreviewImageSize, MaterialWidget::preferredHeight());

    connect(document, &Document::materialNameChanged, this, &MaterialWidget::updateName);
    connect(document, &Document::materialPreviewChanged, this, &MaterialWidget::updatePreview);
}

void MaterialWidget::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    m_previewWidget->move((width() - Theme::materialPreviewImageSize) / 2, 0);
}

int MaterialWidget::preferredHeight()
{
    return Theme::materialPreviewImageSize;
}

void MaterialWidget::reload()
{
    updatePreview(m_materialId);
    updateName(m_materialId);
}

void MaterialWidget::updatePreview(dust3d::Uuid materialId)
{
    if (materialId != m_materialId)
        return;
    const Material *material = m_document->findMaterial(m_materialId);
    if (!material) {
        qDebug() << "Material not found:" << m_materialId;
        return;
    }
    ModelMesh *previewMesh = material->takePreviewMesh();
    m_previewWidget->updateMesh(previewMesh);
}

void MaterialWidget::updateName(dust3d::Uuid materialId)
{
    if (materialId != m_materialId)
        return;
    const Material *material = m_document->findMaterial(m_materialId);
    if (!material) {
        qDebug() << "Material not found:" << m_materialId;
        return;
    }
    m_nameLabel->setText(material->name);
}

void MaterialWidget::updateCheckedState(bool checked)
{
    if (checked)
        setStyleSheet("#MaterialFrame {border: 1px solid " + Theme::red.name() + ";}");
    else
        setStyleSheet("#MaterialFrame {border: 1px solid transparent;}");
}

ModelWidget *MaterialWidget::previewWidget()
{
    return m_previewWidget;
}

void MaterialWidget::mouseDoubleClickEvent(QMouseEvent *event)
{
    QFrame::mouseDoubleClickEvent(event);
    emit modifyMaterial(m_materialId);
}
