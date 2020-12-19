#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QPushButton>
#include "materialmanagewidget.h"
#include "theme.h"
#include "materialeditwidget.h"
#include "infolabel.h"
#include "document.h"

MaterialManageWidget::MaterialManageWidget(const Document *document, QWidget *parent) :
    QWidget(parent),
    m_document(document)
{
    QPushButton *addMaterialButton = new QPushButton(Theme::awesome()->icon(fa::plus), tr("Add Material..."));

    connect(addMaterialButton, &QPushButton::clicked, this, &MaterialManageWidget::showAddMaterialDialog);

    QHBoxLayout *toolsLayout = new QHBoxLayout;
    toolsLayout->addWidget(addMaterialButton);

    m_materialListWidget = new MaterialListWidget(document);
    connect(m_materialListWidget, &MaterialListWidget::modifyMaterial, this, &MaterialManageWidget::showMaterialDialog);

    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->addLayout(toolsLayout);
    mainLayout->addWidget(m_materialListWidget);

    setLayout(mainLayout);
}

MaterialListWidget *MaterialManageWidget::materialListWidget()
{
    return m_materialListWidget;
}

QSize MaterialManageWidget::sizeHint() const
{
    return QSize(Theme::sidebarPreferredWidth, 0);
}

void MaterialManageWidget::showAddMaterialDialog()
{
    showMaterialDialog(QUuid());
}

void MaterialManageWidget::showMaterialDialog(QUuid materialId)
{
    MaterialEditWidget *materialEditWidget = new MaterialEditWidget(m_document);
    materialEditWidget->setAttribute(Qt::WA_DeleteOnClose);
    if (!materialId.isNull()) {
        const Material *material = m_document->findMaterial(materialId);
        if (nullptr != material) {
            materialEditWidget->setEditMaterialId(materialId);
            materialEditWidget->setEditMaterialName(material->name);
            materialEditWidget->setEditMaterialLayers(material->layers);
            materialEditWidget->clearUnsaveState();
        }
    }
    materialEditWidget->show();
    connect(materialEditWidget, &QDialog::destroyed, [=]() {
        emit unregisterDialog((QWidget *)materialEditWidget);
    });
    emit registerDialog((QWidget *)materialEditWidget);
}
