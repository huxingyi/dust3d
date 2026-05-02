#ifndef DUST3D_APPLICATION_TURNAROUND_OVERLAY_WIDGET_H_
#define DUST3D_APPLICATION_TURNAROUND_OVERLAY_WIDGET_H_

#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QWidget>
#include <vector>

class QAction;
class QLabel;
class SceneWidget;
class PreviewOverlayController;
class QMenu;
class QToolButton;
class QString;
class DocumentWindow;
class QShowEvent;
class QHideEvent;

class TurnaroundOverlayWidget : public QWidget {
    Q_OBJECT
public:
    TurnaroundOverlayWidget(QWidget* parent, DocumentWindow* window);
    void setCardTitle(const QString& text);

protected:
    void showEvent(QShowEvent* event) override;
    void hideEvent(QHideEvent* event) override;
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dragMoveEvent(QDragMoveEvent* event) override;
    void dropEvent(QDropEvent* event) override;

private:
    static bool canAcceptDrop(const QMimeData* mimeData);
    static QString strippedName(const QString& fullFileName);

    DocumentWindow* m_window = nullptr;
    QLabel* m_cardTitleLabel = nullptr;
    SceneWidget* m_previewWidget = nullptr;
    std::vector<PreviewOverlayController*> m_previewOverlayControllers;
};

#endif
