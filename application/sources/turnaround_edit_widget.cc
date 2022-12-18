#include "turnaround_edit_widget.h"
#include "image_crop_widget.h"
#include <QHBoxLayout>
#include <QVBoxLayout>

TurnaroundEditWidget::TurnaroundEditWidget(QWidget* parent)
    : QDialog(parent)
{
    m_frontWidget = new ImageCropWidget(this);
    m_sideWidget = new ImageCropWidget(this);

    //m_saveButton = new QPushButton(tr("Save"), this);
    //connect(m_saveButton, &QPushButton::clicked, this, &MyWidget::onSaveButtonClicked);

    QHBoxLayout* rowLayout = new QHBoxLayout;
    rowLayout->addWidget(m_frontWidget);
    rowLayout->addWidget(m_sideWidget);

    QVBoxLayout* mainLayout = new QVBoxLayout;
    mainLayout->addLayout(rowLayout);
    //mainLayout->addWidget(m_saveButton, 0, Qt::AlignCenter);

    setLayout(mainLayout);
}
