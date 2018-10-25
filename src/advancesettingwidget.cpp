#include <QFormLayout>
#include <QCheckBox>
#include "advancesettingwidget.h"
#include "util.h"

AdvanceSettingWidget::AdvanceSettingWidget(const Document *document, QWidget *parent) :
    QDialog(parent),
    m_document(document)
{
    QCheckBox *enableWeldBox = new QCheckBox();
    enableWeldBox->setChecked(document->weldEnabled);
    connect(enableWeldBox, &QCheckBox::stateChanged, this, [=]() {
        emit enableWeld(enableWeldBox->isChecked());
    });
    
    QFormLayout *formLayout = new QFormLayout;
    formLayout->addRow(tr("Weld"), enableWeldBox);
    
    setLayout(formLayout);
    
    connect(this, &AdvanceSettingWidget::enableWeld, document, &Document::enableWeld);
}
