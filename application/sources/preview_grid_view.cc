#include "preview_grid_view.h"
#include "theme.h"

PreviewGridView::PreviewGridView(QWidget *parent):
    QListView(parent)
{
    setViewMode(QListView::IconMode);
    setGridSize(QSize(Theme::partPreviewImageSize, Theme::partPreviewImageSize));
    setMovement(QListView::Snap);
    setFlow(QListView::LeftToRight);
}
