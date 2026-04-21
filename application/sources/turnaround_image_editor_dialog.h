#ifndef DUST3D_APPLICATION_TURNAROUND_IMAGE_EDITOR_DIALOG_H_
#define DUST3D_APPLICATION_TURNAROUND_IMAGE_EDITOR_DIALOG_H_

#include <QDialog>
#include <QImage>
#include <QRect>
#include <QWidget>

class QLabel;
class QPushButton;
class QMouseEvent;
class QPaintEvent;
class QEvent;
class QDragEnterEvent;
class QDragMoveEvent;
class QDropEvent;

class ImageCropWidget : public QWidget {
    Q_OBJECT
public:
    explicit ImageCropWidget(QWidget* parent = nullptr);

    void setImage(const QImage& image);
    void setImageAndCropRect(const QImage& image, const QRect& cropRect);
    const QImage& image() const { return m_image; }
    QRect cropRect() const { return m_cropRect; }
    void setCropRect(const QRect& rect);

signals:
    void cropChanged(const QRect& cropRect);
    void imageDropped(const QString& fileName);

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dragMoveEvent(QDragMoveEvent* event) override;
    void dropEvent(QDropEvent* event) override;
    void leaveEvent(QEvent* event) override;

private:
    enum class HandlePosition {
        None,
        TopLeft,
        Top,
        TopRight,
        Right,
        BottomRight,
        Bottom,
        BottomLeft,
        Left
    };

    QRect imageRect() const;
    QRect displayedCropRect() const;
    QRect handleRect(HandlePosition handle) const;
    HandlePosition handleAtPoint(const QPoint& point) const;
    void updateCursorShape(const QPoint& point);
    QPoint imagePointFromWidgetPoint(const QPoint& point) const;
    QPoint clampPointToImage(const QPoint& imagePoint) const;
    QRect clampCropRect(const QRect& rect) const;
    QRect clampResizeCropRect(const QRect& rect, HandlePosition handle) const;

    QImage m_image;
    QRect m_cropRect;
    bool m_draggingCrop = false;
    bool m_movingCrop = false;
    bool m_resizingCrop = false;
    QPoint m_lastMousePos;
    QRect m_cropBeginRect;
    HandlePosition m_activeHandle = HandlePosition::None;
    const int m_handleSize = 8;
};

class TurnaroundPreviewWorker : public QObject {
    Q_OBJECT
public:
    void setParameters(const QImage& frontImage, const QRect& frontCrop,
        const QImage& sideImage, const QRect& sideCrop);
    QImage* takeResultImage();

signals:
    void finished();

public slots:
    void process();

private:
    QImage m_frontImage;
    QRect m_frontCrop;
    QImage m_sideImage;
    QRect m_sideCrop;
    QImage* m_resultImage = nullptr;
};

class TurnaroundImageEditorDialog : public QDialog {
    Q_OBJECT
public:
    explicit TurnaroundImageEditorDialog(QWidget* parent = nullptr, const QStringList& fileNames = QStringList());
    ~TurnaroundImageEditorDialog();

    QImage resultImage() const;
    float resultOriginX() const;
    float resultOriginY() const;
    float resultOriginZ() const;

protected:
    void reject() override;

private slots:
    void onReloadFrontImage();
    void onReloadSideImage();
    void onFlipSideImage();
    void onSwapFrontAndSideImages();
    void onFrontImageDropped(const QString& fileName);
    void onSideImageDropped(const QString& fileName);
    void onFrontCropChanged(const QRect& cropRect);
    void onSideCropChanged(const QRect& cropRect);
    void onPreviewReady();
    void onAccepted();

private:
    void initializeUi();
    void loadInitialImages(const QStringList& fileNames);
    void schedulePreviewUpdate();
    void startPreviewWorker();
    void updatePreviewDisplay();
    void generatePreviewImageSynchronously();
    bool loadImageFromFile(const QString& fileName, QImage& output, QRect& cropRect);
    void computeResultOrigins(float* originX, float* originY, float* originZ) const;

    ImageCropWidget* m_frontCropWidget = nullptr;
    ImageCropWidget* m_sideCropWidget = nullptr;
    QLabel* m_previewLabel = nullptr;
    QPushButton* m_reloadFrontButton = nullptr;
    QPushButton* m_reloadSideButton = nullptr;
    QPushButton* m_flipSideButton = nullptr;
    QPushButton* m_swapButton = nullptr;
    QPushButton* m_acceptButton = nullptr;
    QPushButton* m_cancelButton = nullptr;

    QImage m_frontImage;
    QImage m_sideImage;
    QImage m_previewImage;
    QRect m_frontCropRect;
    QRect m_sideCropRect;
    bool m_isEdited = false;
    TurnaroundPreviewWorker* m_previewWorker = nullptr;
    QThread* m_previewThread = nullptr;
    bool m_previewObsolete = false;
};

#endif
