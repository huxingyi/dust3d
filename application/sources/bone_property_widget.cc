#include "bone_property_widget.h"
#include "document.h"
#include "theme.h"
#include <QCheckBox>
#include <QGroupBox>
#include <QVBoxLayout>

BonePropertyWidget::BonePropertyWidget(Document* document, QWidget* parent)
    : QWidget(parent)
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 9, 0)
    const auto checkboxStateChangedSignal = &QCheckBox::checkStateChanged;
#else
    const auto checkboxStateChangedSignal = &QCheckBox::stateChanged;
#endif

    QCheckBox* eyelidsCheckBox = new QCheckBox();
    Theme::initCheckbox(eyelidsCheckBox);
    eyelidsCheckBox->setText(tr("Head bone has eyelids"));
    eyelidsCheckBox->setChecked(document->getHeadHasEyelids());

    connect(eyelidsCheckBox, checkboxStateChangedSignal, this, [=]() {
        emit setHeadHasEyelids(eyelidsCheckBox->isChecked());
        emit groupOperationAdded();
    });

    QVBoxLayout* eyelidsLayout = new QVBoxLayout;
    eyelidsLayout->addWidget(eyelidsCheckBox);

    QGroupBox* eyelidsGroupBox = new QGroupBox(tr("Facial Features"));
    eyelidsGroupBox->setLayout(eyelidsLayout);

    QVBoxLayout* mainLayout = new QVBoxLayout;
    mainLayout->addWidget(eyelidsGroupBox);
    mainLayout->setSizeConstraint(QLayout::SetFixedSize);

    connect(this, &BonePropertyWidget::setHeadHasEyelids, document, &Document::setHeadHasEyelids);
    connect(this, &BonePropertyWidget::groupOperationAdded, document, &Document::saveSnapshot);

    setLayout(mainLayout);
    setFixedSize(minimumSizeHint());
}
