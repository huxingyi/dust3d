#ifndef DUST3D_APPLICATION_TURNAROUND_EDIT_H_
#define DUST3D_APPLICATION_TURNAROUND_EDIT_H_

#include <QDialog>

class ImageCropWidget;

class TurnaroundEditWidget : public QDialog {
    Q_OBJECT

public:
    TurnaroundEditWidget(QWidget* parent = nullptr);

private:
    ImageCropWidget* m_frontWidget = nullptr;
    ImageCropWidget* m_sideWidget = nullptr;
};

#endif
