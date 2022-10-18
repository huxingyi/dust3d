#include "preview_grid_view.h"
#include "theme.h"

PreviewGridView::PreviewGridView(QWidget* parent)
    : QListView(parent)
{
    QPalette viewPalette = palette();
    viewPalette.setColor(QPalette::Window, Qt::transparent);
    viewPalette.setColor(QPalette::Base, Qt::transparent);
    setPalette(viewPalette);

    auto borderSize = Theme::previewIconBorderSize;
    auto margin = Theme::previewIconMargin;
    auto borderRadius = Theme::previewIconBorderRadius;
    setStyleSheet(
        "QListView::item {border:" + QString::number(borderSize) + "px solid transparent; margin:" + QString::number(margin) + "px; background-color: " + Theme::gray.name() + "; border-radius: " + QString::number(borderRadius) + ";}" + "QListView::item:selected {border-color: " + Theme::red.name() + ";}");

    setViewMode(QListView::IconMode);
    setGridSize(QSize(Theme::partPreviewImageSize, Theme::partPreviewImageSize));
    setMovement(QListView::Snap);
    setFlow(QListView::LeftToRight);
    setSelectionMode(QAbstractItemView::ExtendedSelection);
    setResizeMode(QListView::Adjust);
}