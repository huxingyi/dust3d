#ifndef DUST3D_APPLICATION_PREVIEW_GRID_VIEW_H_
#define DUST3D_APPLICATION_PREVIEW_GRID_VIEW_H_

#include <QListView>

class PreviewGridView: public QListView
{
    Q_OBJECT
public:
    PreviewGridView(QWidget *parent=nullptr);
};

#endif
