#include "preview_grid_view.h"
#include "theme.h"

PreviewGridView::PreviewGridView(QWidget *parent):
    QListView(parent)
{
    QPalette viewPalette = palette();
    viewPalette.setColor(QPalette::Window, Qt::transparent);
    viewPalette.setColor(QPalette::Base, Qt::transparent);
    setPalette(viewPalette);

    auto borderSize = std::max(1, Theme::partPreviewImageSize / 20);
    auto margin = std::max(1, borderSize / 2);
    auto borderRadius = std::max(3, Theme::partPreviewImageSize / 10);
    setStyleSheet(
        "QListView::item {border:" + QString::number(borderSize) + "px solid transparent; margin:" + QString::number(margin) + "px; background-color: " + Theme::gray.name() +"; border-radius: " + QString::number(borderRadius) + ";}" +
        "QListView::item:selected {border-color: " + Theme::red.name() + ";}"
    );

    setViewMode(QListView::IconMode);
    setGridSize(QSize(Theme::partPreviewImageSize, Theme::partPreviewImageSize));
    setMovement(QListView::Snap);
    setFlow(QListView::LeftToRight);
    setSelectionMode(QAbstractItemView::ExtendedSelection);
}
