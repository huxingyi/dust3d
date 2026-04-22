#include "turnaround_image_editor_dialog.h"
#include "theme.h"
#include <QBoxLayout>
#include <QCursor>
#include <QDialogButtonBox>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QFileDialog>
#include <QLabel>
#include <QMessageBox>
#include <QMimeData>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QPushButton>
#include <QThread>
#include <QUrl>

ImageCropWidget::ImageCropWidget(QWidget* parent)
    : QWidget(parent)
{
    setMinimumSize(240, 240);
    setMouseTracking(true);
    setAcceptDrops(true);
}

static bool isSupportedImageFile(const QString& fileName)
{
    return fileName.endsWith(".png", Qt::CaseInsensitive)
        || fileName.endsWith(".jpg", Qt::CaseInsensitive)
        || fileName.endsWith(".jpeg", Qt::CaseInsensitive)
        || fileName.endsWith(".bmp", Qt::CaseInsensitive)
        || fileName.endsWith(".gif", Qt::CaseInsensitive)
        || fileName.endsWith(".tiff", Qt::CaseInsensitive);
}

static bool isSupportedDrop(const QMimeData* mimeData)
{
    if (!mimeData)
        return false;

    if (mimeData->hasUrls()) {
        for (const QUrl& url : mimeData->urls()) {
            if (url.isLocalFile() && isSupportedImageFile(url.toLocalFile()))
                return true;
        }
    }
    return false;
}

void ImageCropWidget::dragEnterEvent(QDragEnterEvent* event)
{
    if (isSupportedDrop(event->mimeData())) {
        event->acceptProposedAction();
        event->accept();
    } else {
        event->ignore();
    }
}

void ImageCropWidget::dragMoveEvent(QDragMoveEvent* event)
{
    if (isSupportedDrop(event->mimeData())) {
        event->acceptProposedAction();
        event->accept();
    } else {
        event->ignore();
    }
}

void ImageCropWidget::dropEvent(QDropEvent* event)
{
    if (!isSupportedDrop(event->mimeData())) {
        event->ignore();
        return;
    }

    for (const QUrl& url : event->mimeData()->urls()) {
        if (url.isLocalFile() && isSupportedImageFile(url.toLocalFile())) {
            emit imageDropped(url.toLocalFile());
            event->acceptProposedAction();
            return;
        }
    }
    event->ignore();
}

void ImageCropWidget::setImage(const QImage& image)
{
    m_image = image;
    if (!m_image.isNull()) {
        m_cropRect = QRect(QPoint(0, 0), m_image.size());
    } else {
        m_cropRect = QRect();
    }
    update();
    emit cropChanged(m_cropRect);
}

void ImageCropWidget::setImageAndCropRect(const QImage& image, const QRect& cropRect)
{
    m_image = image;
    if (!m_image.isNull()) {
        m_cropRect = clampCropRect(cropRect);
    } else {
        m_cropRect = QRect();
    }
    update();
    emit cropChanged(m_cropRect);
}

void ImageCropWidget::setCropRect(const QRect& rect)
{
    QRect normalizedRect = clampCropRect(rect);
    if (normalizedRect == m_cropRect)
        return;
    m_cropRect = normalizedRect;
    update();
    emit cropChanged(m_cropRect);
}

void ImageCropWidget::paintEvent(QPaintEvent* /*event*/)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.fillRect(rect(), QColor(45, 45, 45));

    QRect imageArea = imageRect();
    if (m_image.isNull() || imageArea.isEmpty()) {
        painter.setPen(Qt::lightGray);
        painter.drawText(rect(), Qt::AlignCenter, tr("No image loaded"));
        return;
    }

    painter.drawImage(imageArea, m_image);

    QRect cropDisplay = displayedCropRect();
    if (!cropDisplay.isEmpty()) {
        QColor overlayColor(0, 0, 0, 160);
        QPainterPath maskPath;
        maskPath.addRect(imageArea);
        maskPath.addRect(cropDisplay);
        maskPath.setFillRule(Qt::OddEvenFill);
        painter.fillPath(maskPath, overlayColor);

        painter.setPen(QPen(Qt::white, 2));
        painter.drawRect(cropDisplay.adjusted(0, 0, -1, -1));

        painter.setBrush(Qt::white);
        painter.setPen(QPen(Qt::black, 1));
        const ImageCropWidget::HandlePosition handles[] = {
            ImageCropWidget::HandlePosition::TopLeft,
            ImageCropWidget::HandlePosition::Top,
            ImageCropWidget::HandlePosition::TopRight,
            ImageCropWidget::HandlePosition::Right,
            ImageCropWidget::HandlePosition::BottomRight,
            ImageCropWidget::HandlePosition::Bottom,
            ImageCropWidget::HandlePosition::BottomLeft,
            ImageCropWidget::HandlePosition::Left
        };
        for (auto handle : handles) {
            QRect handleRect = this->handleRect(handle);
            painter.drawRect(handleRect.adjusted(0, 0, -1, -1));
            painter.fillRect(handleRect.adjusted(1, 1, -1, -1), Qt::white);
        }
    }
}

void ImageCropWidget::mousePressEvent(QMouseEvent* event)
{
    if (m_image.isNull() || event->button() != Qt::LeftButton)
        return;

    QRect imageArea = imageRect();
    if (!imageArea.contains(event->pos()))
        return;

    QRect cropDisplay = displayedCropRect();
    if (!cropDisplay.contains(event->pos()))
        return;

    m_activeHandle = handleAtPoint(event->pos());
    if (m_activeHandle != HandlePosition::None) {
        m_resizingCrop = true;
    } else {
        m_movingCrop = true;
    }
    m_cropBeginRect = m_cropRect;
    m_lastMousePos = event->pos();
}

void ImageCropWidget::mouseMoveEvent(QMouseEvent* event)
{
    if (m_image.isNull())
        return;

    updateCursorShape(event->pos());

    QPoint imagePoint = imagePointFromWidgetPoint(event->pos());
    if (m_resizingCrop) {
        QRect newRect = m_cropBeginRect;
        switch (m_activeHandle) {
        case HandlePosition::TopLeft:
            newRect.setTopLeft(clampPointToImage(imagePoint));
            break;
        case HandlePosition::Top:
            newRect.setTop(clampPointToImage(imagePoint).y());
            break;
        case HandlePosition::TopRight:
            newRect.setTopRight(clampPointToImage(imagePoint));
            break;
        case HandlePosition::Right:
            newRect.setRight(clampPointToImage(imagePoint).x());
            break;
        case HandlePosition::BottomRight:
            newRect.setBottomRight(clampPointToImage(imagePoint));
            break;
        case HandlePosition::Bottom:
            newRect.setBottom(clampPointToImage(imagePoint).y());
            break;
        case HandlePosition::BottomLeft:
            newRect.setBottomLeft(clampPointToImage(imagePoint));
            break;
        case HandlePosition::Left:
            newRect.setLeft(clampPointToImage(imagePoint).x());
            break;
        default:
            break;
        }
        newRect = clampResizeCropRect(newRect, m_activeHandle);
        if (newRect != m_cropRect) {
            m_cropRect = newRect;
            emit cropChanged(m_cropRect);
            update();
        }
    } else if (m_movingCrop) {
        QPoint lastImagePoint = imagePointFromWidgetPoint(m_lastMousePos);
        QPoint delta = imagePoint - lastImagePoint;
        QRect newRect = m_cropBeginRect.translated(delta);
        newRect = clampCropRect(newRect);
        if (newRect != m_cropRect) {
            m_cropRect = newRect;
            emit cropChanged(m_cropRect);
            update();
        }
    }
}

void ImageCropWidget::mouseReleaseEvent(QMouseEvent* event)
{
    Q_UNUSED(event);
    m_draggingCrop = false;
    m_movingCrop = false;
    m_resizingCrop = false;
    m_activeHandle = HandlePosition::None;
    updateCursorShape(event->pos());
}

void ImageCropWidget::leaveEvent(QEvent* event)
{
    Q_UNUSED(event);
    unsetCursor();
}

QRect ImageCropWidget::imageRect() const
{
    if (m_image.isNull())
        return QRect();

    QSize available = rect().size() - QSize(4, 4);
    QSize scaledSize = m_image.size().scaled(available, Qt::KeepAspectRatio);
    QPoint topLeft((width() - scaledSize.width()) / 2, (height() - scaledSize.height()) / 2);
    return QRect(topLeft, scaledSize);
}

QRect ImageCropWidget::displayedCropRect() const
{
    QRect imageArea = imageRect();
    if (m_image.isNull() || m_cropRect.isEmpty() || imageArea.isEmpty())
        return QRect();

    qreal ratioX = imageArea.width() / static_cast<qreal>(m_image.width());
    qreal ratioY = imageArea.height() / static_cast<qreal>(m_image.height());
    QRect displayRect;
    displayRect.setX(imageArea.x() + static_cast<int>(m_cropRect.x() * ratioX));
    displayRect.setY(imageArea.y() + static_cast<int>(m_cropRect.y() * ratioY));
    displayRect.setWidth(static_cast<int>(m_cropRect.width() * ratioX));
    displayRect.setHeight(static_cast<int>(m_cropRect.height() * ratioY));
    return displayRect;
}

QPoint ImageCropWidget::imagePointFromWidgetPoint(const QPoint& point) const
{
    QRect imageArea = imageRect();
    if (m_image.isNull() || imageArea.isEmpty())
        return QPoint();

    qreal x = (point.x() - imageArea.x()) / static_cast<qreal>(imageArea.width());
    qreal y = (point.y() - imageArea.y()) / static_cast<qreal>(imageArea.height());
    x = qBound<qreal>(0.0, x, 1.0);
    y = qBound<qreal>(0.0, y, 1.0);
    return QPoint(static_cast<int>(x * (m_image.width() - 1)), static_cast<int>(y * (m_image.height() - 1)));
}

QPoint ImageCropWidget::clampPointToImage(const QPoint& imagePoint) const
{
    if (m_image.isNull())
        return QPoint();

    int x = qBound(0, imagePoint.x(), m_image.width() - 1);
    int y = qBound(0, imagePoint.y(), m_image.height() - 1);
    return QPoint(x, y);
}

void ImageCropWidget::updateCursorShape(const QPoint& point)
{
    if (m_image.isNull()) {
        unsetCursor();
        return;
    }

    QRect cropDisplay = displayedCropRect();
    if (cropDisplay.isEmpty()) {
        unsetCursor();
        return;
    }

    HandlePosition handle = handleAtPoint(point);
    if (handle != HandlePosition::None) {
        switch (handle) {
        case HandlePosition::TopLeft:
        case HandlePosition::BottomRight:
            setCursor(Qt::SizeFDiagCursor);
            break;
        case HandlePosition::TopRight:
        case HandlePosition::BottomLeft:
            setCursor(Qt::SizeBDiagCursor);
            break;
        case HandlePosition::Top:
        case HandlePosition::Bottom:
            setCursor(Qt::SizeVerCursor);
            break;
        case HandlePosition::Left:
        case HandlePosition::Right:
            setCursor(Qt::SizeHorCursor);
            break;
        default:
            setCursor(Qt::ArrowCursor);
            break;
        }
        return;
    }

    if (cropDisplay.contains(point)) {
        setCursor(Qt::OpenHandCursor);
        return;
    }

    unsetCursor();
}

QRect ImageCropWidget::handleRect(HandlePosition handle) const
{
    QRect cropDisplay = displayedCropRect();
    if (cropDisplay.isEmpty())
        return QRect();

    int halfHandle = m_handleSize / 2;
    QPoint center;
    switch (handle) {
    case HandlePosition::TopLeft:
        center = cropDisplay.topLeft();
        break;
    case HandlePosition::Top:
        center = QPoint(cropDisplay.left() + cropDisplay.width() / 2, cropDisplay.top());
        break;
    case HandlePosition::TopRight:
        center = cropDisplay.topRight();
        break;
    case HandlePosition::Right:
        center = QPoint(cropDisplay.right(), cropDisplay.top() + cropDisplay.height() / 2);
        break;
    case HandlePosition::BottomRight:
        center = cropDisplay.bottomRight();
        break;
    case HandlePosition::Bottom:
        center = QPoint(cropDisplay.left() + cropDisplay.width() / 2, cropDisplay.bottom());
        break;
    case HandlePosition::BottomLeft:
        center = cropDisplay.bottomLeft();
        break;
    case HandlePosition::Left:
        center = QPoint(cropDisplay.left(), cropDisplay.top() + cropDisplay.height() / 2);
        break;
    default:
        return QRect();
    }
    return QRect(center - QPoint(halfHandle, halfHandle), QSize(m_handleSize, m_handleSize));
}

ImageCropWidget::HandlePosition ImageCropWidget::handleAtPoint(const QPoint& point) const
{
    QRect cropDisplay = displayedCropRect();
    if (cropDisplay.isEmpty())
        return HandlePosition::None;

    int tolerance = qMax(m_handleSize, 12);
    QRect topEdge(cropDisplay.left() + tolerance, cropDisplay.top() - tolerance,
        cropDisplay.width() - tolerance * 2, tolerance * 2 + 1);
    QRect bottomEdge(cropDisplay.left() + tolerance, cropDisplay.bottom() - tolerance,
        cropDisplay.width() - tolerance * 2, tolerance * 2 + 1);
    QRect leftEdge(cropDisplay.left() - tolerance, cropDisplay.top() + tolerance,
        tolerance * 2 + 1, cropDisplay.height() - tolerance * 2);
    QRect rightEdge(cropDisplay.right() - tolerance, cropDisplay.top() + tolerance,
        tolerance * 2 + 1, cropDisplay.height() - tolerance * 2);

    if (topEdge.contains(point))
        return HandlePosition::Top;
    if (bottomEdge.contains(point))
        return HandlePosition::Bottom;
    if (leftEdge.contains(point))
        return HandlePosition::Left;
    if (rightEdge.contains(point))
        return HandlePosition::Right;

    const HandlePosition corners[] = {
        HandlePosition::TopLeft,
        HandlePosition::TopRight,
        HandlePosition::BottomRight,
        HandlePosition::BottomLeft
    };
    for (auto handle : corners) {
        QRect rect = handleRect(handle).adjusted(-tolerance, -tolerance, tolerance, tolerance);
        if (rect.contains(point))
            return handle;
    }

    return HandlePosition::None;
}

QRect ImageCropWidget::clampCropRect(const QRect& rect) const
{
    if (m_image.isNull())
        return QRect();

    QRect normalized = rect.normalized();
    QRect bounds(QPoint(0, 0), m_image.size());
    normalized &= bounds;
    if (normalized.width() <= 0 || normalized.height() <= 0)
        normalized = QRect(QPoint(0, 0), m_image.size());
    return normalized;
}

QRect ImageCropWidget::clampResizeCropRect(const QRect& rect, HandlePosition handle) const
{
    QRect result = rect;
    if (m_image.isNull())
        return result;

    QRect bounds(QPoint(0, 0), m_image.size());
    const int minSize = qMax(16, m_handleSize * 2);

    if (handle == HandlePosition::TopLeft || handle == HandlePosition::Left || handle == HandlePosition::BottomLeft) {
        int maxLeft = result.right() - minSize + 1;
        result.setLeft(qBound(bounds.left(), maxLeft, result.left()));
    } else if (handle == HandlePosition::TopRight || handle == HandlePosition::Right || handle == HandlePosition::BottomRight) {
        int minRight = result.left() + minSize - 1;
        result.setRight(qBound(minRight, bounds.right(), result.right()));
    }

    if (handle == HandlePosition::TopLeft || handle == HandlePosition::Top || handle == HandlePosition::TopRight) {
        int maxTop = result.bottom() - minSize + 1;
        result.setTop(qBound(bounds.top(), maxTop, result.top()));
    } else if (handle == HandlePosition::BottomLeft || handle == HandlePosition::Bottom || handle == HandlePosition::BottomRight) {
        int minBottom = result.top() + minSize - 1;
        result.setBottom(qBound(minBottom, bounds.bottom(), result.bottom()));
    }

    if (!bounds.contains(result)) {
        result = result.intersected(bounds);
    }

    return result;
}

void TurnaroundPreviewWorker::setParameters(const QImage& frontImage, const QRect& frontCrop,
    const QImage& sideImage, const QRect& sideCrop)
{
    m_frontImage = frontImage;
    m_frontCrop = frontCrop;
    m_sideImage = sideImage;
    m_sideCrop = sideCrop;
}

QImage* TurnaroundPreviewWorker::takeResultImage()
{
    QImage* result = m_resultImage;
    m_resultImage = nullptr;
    return result;
}

void TurnaroundPreviewWorker::process()
{
    QImage result;
    bool hasFront = !m_frontImage.isNull();
    bool hasSide = !m_sideImage.isNull();
    if (!hasFront && !hasSide) {
        m_resultImage = new QImage(result);
        emit finished();
        return;
    }

    QImage frontScaled;
    if (hasFront) {
        QRect frontCrop = m_frontCrop.isValid() ? m_frontCrop.normalized() : QRect(QPoint(0, 0), m_frontImage.size());
        QImage frontCropped = m_frontImage.copy(frontCrop);
        frontScaled = frontCropped.scaledToHeight(512, Qt::SmoothTransformation);
    }

    QImage sideScaled;
    if (hasSide && m_sideCrop.isValid()) {
        QRect sideCrop = m_sideCrop.normalized();
        QImage sideCropped = m_sideImage.copy(sideCrop);
        sideScaled = sideCropped.scaledToHeight(512, Qt::SmoothTransformation);
    }

    if (hasFront && hasSide && !sideScaled.isNull()) {
        QRect sideCrop = m_sideCrop.normalized();
        QImage sideCropped = m_sideImage.copy(sideCrop);
        QImage sideScaled = sideCropped.scaledToHeight(512, Qt::SmoothTransformation);
        int combinedWidth = frontScaled.width() + sideScaled.width();
        int finalWidth = std::max({ combinedWidth, frontScaled.width() * 2, sideScaled.width() * 2, 512 * 2 });
        int leftHalfWidth = finalWidth / 2;
        int rightHalfWidth = finalWidth - leftHalfWidth;
        int frontX = (leftHalfWidth - frontScaled.width()) / 2;
        int sideX = leftHalfWidth + (rightHalfWidth - sideScaled.width()) / 2;
        result = QImage(finalWidth, 512, QImage::Format_RGBA8888);
        result.fill(Qt::white);
        QPainter painter(&result);
        painter.drawImage(frontX, 0, frontScaled);
        painter.drawImage(sideX, 0, sideScaled);
    } else if (hasSide && !sideScaled.isNull()) {
        int finalWidth = std::max(sideScaled.width() * 2, 512 * 2);
        int leftHalfWidth = finalWidth / 2;
        int sideX = leftHalfWidth + (leftHalfWidth - sideScaled.width()) / 2;
        result = QImage(finalWidth, 512, QImage::Format_RGBA8888);
        result.fill(Qt::white);
        QPainter painter(&result);
        painter.drawImage(sideX, 0, sideScaled);
    } else {
        int finalWidth = std::max(frontScaled.width() * 2, 512 * 2);
        int leftHalfWidth = finalWidth / 2;
        int frontX = (leftHalfWidth - frontScaled.width()) / 2;
        result = QImage(finalWidth, 512, QImage::Format_RGBA8888);
        result.fill(Qt::white);
        QPainter painter(&result);
        painter.drawImage(frontX, 0, frontScaled);
    }

    m_resultImage = new QImage(result);
    emit finished();
}

TurnaroundImageEditorDialog::TurnaroundImageEditorDialog(QWidget* parent, const QStringList& fileNames)
    : QDialog(parent)
{
    setWindowTitle(tr("Background Image Editor"));
    setModal(true);
    initializeUi();
    loadInitialImages(fileNames);
    schedulePreviewUpdate();
}

TurnaroundImageEditorDialog::~TurnaroundImageEditorDialog()
{
    if (m_previewThread) {
        m_previewThread->quit();
        m_previewThread->wait();
        m_previewThread = nullptr;
    }
    delete m_previewWorker;
}

QImage TurnaroundImageEditorDialog::resultImage() const
{
    return m_previewImage;
}

float TurnaroundImageEditorDialog::resultOriginX() const
{
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
    computeResultOrigins(&x, &y, &z);
    return x;
}

float TurnaroundImageEditorDialog::resultOriginY() const
{
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
    computeResultOrigins(&x, &y, &z);
    return y;
}

float TurnaroundImageEditorDialog::resultOriginZ() const
{
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
    computeResultOrigins(&x, &y, &z);
    return z;
}

void TurnaroundImageEditorDialog::computeResultOrigins(float* originX, float* originY, float* originZ) const
{
    const bool hasFront = !m_frontImage.isNull();
    const bool hasSide = !m_sideImage.isNull() && m_sideCropRect.isValid();
    const int targetHeight = 512;
    *originY = 0.5f;

    QImage frontScaled;
    if (hasFront) {
        QRect frontCrop = m_frontCropRect.isValid() ? m_frontCropRect.normalized() : QRect(QPoint(0, 0), m_frontImage.size());
        QImage frontCropped = m_frontImage.copy(frontCrop);
        frontScaled = frontCropped.scaledToHeight(targetHeight, Qt::SmoothTransformation);
    }

    QImage sideScaled;
    if (hasSide) {
        QRect sideCrop = m_sideCropRect.normalized();
        QImage sideCropped = m_sideImage.copy(sideCrop);
        sideScaled = sideCropped.scaledToHeight(targetHeight, Qt::SmoothTransformation);
    }

    if (hasFront && hasSide && !sideScaled.isNull()) {
        int combinedWidth = frontScaled.width() + sideScaled.width();
        int finalWidth = std::max({ combinedWidth, frontScaled.width() * 2, sideScaled.width() * 2, targetHeight * 2 });
        int leftHalfWidth = finalWidth / 2;
        int rightHalfWidth = finalWidth - leftHalfWidth;
        int frontX = (leftHalfWidth - frontScaled.width()) / 2;
        int sideX = leftHalfWidth + (rightHalfWidth - sideScaled.width()) / 2;
        *originX = (frontX + frontScaled.width() * 0.5f) / static_cast<float>(targetHeight);
        *originZ = (sideX + sideScaled.width() * 0.5f) / static_cast<float>(targetHeight);
    } else if (hasSide && !sideScaled.isNull()) {
        int finalWidth = std::max(sideScaled.width() * 2, targetHeight * 2);
        int leftHalfWidth = finalWidth / 2;
        int sideX = leftHalfWidth + (leftHalfWidth - sideScaled.width()) / 2;
        *originX = (leftHalfWidth * 0.5f) / static_cast<float>(targetHeight);
        *originZ = (sideX + sideScaled.width() * 0.5f) / static_cast<float>(targetHeight);
    } else if (hasFront) {
        int finalWidth = std::max(frontScaled.width() * 2, targetHeight * 2);
        int leftHalfWidth = finalWidth / 2;
        int frontX = (leftHalfWidth - frontScaled.width()) / 2;
        *originX = (frontX + frontScaled.width() * 0.5f) / static_cast<float>(targetHeight);
        *originZ = (leftHalfWidth + leftHalfWidth * 0.5f) / static_cast<float>(targetHeight);
    } else {
        *originX = 0.5f;
        *originZ = 0.5f;
    }
}

void TurnaroundImageEditorDialog::reject()
{
    if (m_isEdited) {
        QMessageBox::StandardButton answer = QMessageBox::question(this,
            tr("Discard changes?"),
            tr("You have unsaved crop changes. Discard them and close?"),
            QMessageBox::Yes | QMessageBox::No,
            QMessageBox::No);
        if (answer != QMessageBox::Yes)
            return;
    }
    QDialog::reject();
}

void TurnaroundImageEditorDialog::onReloadFrontImage()
{
    QString fileName = QFileDialog::getOpenFileName(this,
        tr("Reload Front Image"),
        QString(),
        tr("Image Files (*.png *.jpg *.jpeg *.bmp)"));
    if (fileName.isEmpty())
        return;

    QImage image;
    QRect cropRect;
    if (!loadImageFromFile(fileName, image, cropRect)) {
        QMessageBox::warning(this, tr("Invalid Image"), tr("Cannot load front view image."));
        return;
    }

    m_frontImage = image;
    m_frontCropRect = cropRect;
    m_frontCropWidget->setImage(m_frontImage);
    m_isEdited = true;
    schedulePreviewUpdate();
}

void TurnaroundImageEditorDialog::onReloadSideImage()
{
    QString fileName = QFileDialog::getOpenFileName(this,
        tr("Reload Side Image"),
        QString(),
        tr("Image Files (*.png *.jpg *.jpeg *.bmp)"));
    if (fileName.isEmpty())
        return;

    QImage image;
    QRect cropRect;
    if (!loadImageFromFile(fileName, image, cropRect)) {
        QMessageBox::warning(this, tr("Invalid Image"), tr("Cannot load side view image."));
        return;
    }

    m_sideImage = image;
    m_sideCropRect = cropRect;
    m_sideCropWidget->setImage(m_sideImage);
    m_isEdited = true;
    schedulePreviewUpdate();
}

void TurnaroundImageEditorDialog::onFrontImageDropped(const QString& fileName)
{
    QImage image;
    QRect cropRect;
    if (!loadImageFromFile(fileName, image, cropRect)) {
        QMessageBox::warning(this, tr("Invalid Image"), tr("Cannot load front view image."));
        return;
    }

    m_frontImage = image;
    m_frontCropRect = cropRect;
    m_frontCropWidget->setImage(m_frontImage);
    m_isEdited = true;
    schedulePreviewUpdate();
}

void TurnaroundImageEditorDialog::onSideImageDropped(const QString& fileName)
{
    QImage image;
    QRect cropRect;
    if (!loadImageFromFile(fileName, image, cropRect)) {
        QMessageBox::warning(this, tr("Invalid Image"), tr("Cannot load side view image."));
        return;
    }

    m_sideImage = image;
    m_sideCropRect = cropRect;
    m_sideCropWidget->setImage(m_sideImage);
    m_isEdited = true;
    schedulePreviewUpdate();
}

void TurnaroundImageEditorDialog::onFrontCropChanged(const QRect& cropRect)
{
    m_frontCropRect = cropRect;
    m_isEdited = true;
    schedulePreviewUpdate();
}

void TurnaroundImageEditorDialog::onSideCropChanged(const QRect& cropRect)
{
    m_sideCropRect = cropRect;
    m_isEdited = true;
    schedulePreviewUpdate();
}

void TurnaroundImageEditorDialog::onPreviewReady()
{
    if (nullptr == m_previewWorker)
        return;

    QImage* result = m_previewWorker->takeResultImage();
    if (result) {
        m_previewImage = std::move(*result);
        delete result;
    } else {
        m_previewImage = QImage();
    }

    m_previewWorker = nullptr;
    m_previewThread = nullptr;
    updatePreviewDisplay();

    if (m_previewObsolete) {
        m_previewObsolete = false;
        schedulePreviewUpdate();
    }
}

void TurnaroundImageEditorDialog::onAccepted()
{
    if (m_frontImage.isNull() && m_sideImage.isNull()) {
        QMessageBox::warning(this, tr("No image loaded"), tr("Please load a front or side image before using it."));
        return;
    }

    if (m_previewImage.isNull()) {
        generatePreviewImageSynchronously();
    }

    accept();
}

void TurnaroundImageEditorDialog::onFlipSideImage()
{
    if (m_sideImage.isNull()) {
        return;
    }

    m_sideImage = m_sideImage.mirrored(true, false);
    if (m_sideCropRect.isValid()) {
        QRect normalizedCrop = m_sideCropRect.normalized();
        int width = m_sideImage.width();
        int newX = width - (normalizedCrop.x() + normalizedCrop.width());
        normalizedCrop.moveLeft(newX);
        m_sideCropRect = normalizedCrop;
    }

    m_sideCropWidget->setImageAndCropRect(m_sideImage, m_sideCropRect);
    m_isEdited = true;
    schedulePreviewUpdate();
}

void TurnaroundImageEditorDialog::onSwapFrontAndSideImages()
{
    std::swap(m_frontImage, m_sideImage);
    std::swap(m_frontCropRect, m_sideCropRect);

    m_frontCropWidget->setImageAndCropRect(m_frontImage, m_frontCropRect);
    m_sideCropWidget->setImageAndCropRect(m_sideImage, m_sideCropRect);

    m_isEdited = true;
    schedulePreviewUpdate();
}

void TurnaroundImageEditorDialog::initializeUi()
{
    m_frontCropWidget = new ImageCropWidget(this);
    m_sideCropWidget = new ImageCropWidget(this);
    m_previewLabel = new QLabel(this);
    m_previewLabel->setFrameStyle(QFrame::StyledPanel | QFrame::Sunken);
    m_previewLabel->setMinimumSize(320, 320);
    m_previewLabel->setAlignment(Qt::AlignCenter);

    m_reloadFrontButton = new QPushButton(Theme::awesome()->icon(fa::image), QString(), this);
    Theme::initIconButton(m_reloadFrontButton);
    m_reloadFrontButton->setToolTip(tr("Change Front Image"));

    m_reloadSideButton = new QPushButton(Theme::awesome()->icon(fa::image), QString(), this);
    Theme::initIconButton(m_reloadSideButton);
    m_reloadSideButton->setToolTip(tr("Change Side Image"));

    m_flipSideButton = new QPushButton(Theme::awesome()->icon(fa::arrowsh), QString(), this);
    Theme::initIconButton(m_flipSideButton);
    m_flipSideButton->setToolTip(tr("Flip Side"));

    m_swapButton = new QPushButton(Theme::awesome()->icon(fa::exchange), QString(), this);
    Theme::initIconButton(m_swapButton);
    m_swapButton->setToolTip(tr("Swap Front/Side"));

    QDialogButtonBox* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal, this);
    m_acceptButton = buttonBox->button(QDialogButtonBox::Ok);
    m_cancelButton = buttonBox->button(QDialogButtonBox::Cancel);
    m_acceptButton->setText(tr("Use Image"));
    m_cancelButton->setText(tr("Cancel"));

    m_frontCropWidget->setAcceptDrops(true);
    m_sideCropWidget->setAcceptDrops(true);

    connect(m_reloadFrontButton, &QPushButton::clicked, this, &TurnaroundImageEditorDialog::onReloadFrontImage);
    connect(m_reloadSideButton, &QPushButton::clicked, this, &TurnaroundImageEditorDialog::onReloadSideImage);
    connect(m_flipSideButton, &QPushButton::clicked, this, &TurnaroundImageEditorDialog::onFlipSideImage);
    connect(m_swapButton, &QPushButton::clicked, this, &TurnaroundImageEditorDialog::onSwapFrontAndSideImages);
    connect(m_frontCropWidget, &ImageCropWidget::cropChanged, this, &TurnaroundImageEditorDialog::onFrontCropChanged);
    connect(m_sideCropWidget, &ImageCropWidget::cropChanged, this, &TurnaroundImageEditorDialog::onSideCropChanged);
    connect(m_frontCropWidget, &ImageCropWidget::imageDropped, this, &TurnaroundImageEditorDialog::onFrontImageDropped);
    connect(m_sideCropWidget, &ImageCropWidget::imageDropped, this, &TurnaroundImageEditorDialog::onSideImageDropped);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &TurnaroundImageEditorDialog::onAccepted);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &TurnaroundImageEditorDialog::reject);

    QVBoxLayout* frontLayout = new QVBoxLayout;
    frontLayout->addWidget(new QLabel(tr("Front View"), this));
    frontLayout->addWidget(m_frontCropWidget, 1);
    QHBoxLayout* frontButtonLayout = new QHBoxLayout;
    frontButtonLayout->addStretch();
    frontButtonLayout->addWidget(m_reloadFrontButton);
    frontButtonLayout->addStretch();
    frontLayout->addLayout(frontButtonLayout);

    QVBoxLayout* sideLayout = new QVBoxLayout;
    sideLayout->addWidget(new QLabel(tr("Side View"), this));
    sideLayout->addWidget(m_sideCropWidget, 1);
    QHBoxLayout* sideButtonLayout = new QHBoxLayout;
    sideButtonLayout->addStretch();
    sideButtonLayout->addWidget(m_reloadSideButton);
    sideButtonLayout->addWidget(m_flipSideButton);
    sideButtonLayout->addStretch();
    sideLayout->addLayout(sideButtonLayout);

    QVBoxLayout* swapLayout = new QVBoxLayout;
    swapLayout->addStretch();
    swapLayout->addWidget(m_swapButton, 0, Qt::AlignCenter);
    swapLayout->addStretch();

    QHBoxLayout* imageRowLayout = new QHBoxLayout;
    imageRowLayout->addLayout(frontLayout, 1);
    imageRowLayout->addLayout(swapLayout);
    imageRowLayout->addLayout(sideLayout, 1);

    QVBoxLayout* previewLayout = new QVBoxLayout;
    previewLayout->addWidget(new QLabel(tr("Combined Preview"), this));
    previewLayout->addWidget(m_previewLabel);

    QHBoxLayout* topLayout = new QHBoxLayout;
    topLayout->addLayout(imageRowLayout, 2);
    topLayout->addLayout(previewLayout, 1);

    QVBoxLayout* mainLayout = new QVBoxLayout;
    mainLayout->addLayout(topLayout);
    mainLayout->addWidget(buttonBox);
    setLayout(mainLayout);
}

void TurnaroundImageEditorDialog::loadInitialImages(const QStringList& fileNames)
{
    if (fileNames.isEmpty())
        return;

    QImage image;
    QRect cropRect;
    if (loadImageFromFile(fileNames[0], image, cropRect)) {
        m_frontImage = image;
        m_frontCropRect = cropRect;
        m_frontCropWidget->setImage(m_frontImage);
    }

    if (fileNames.size() > 1) {
        if (loadImageFromFile(fileNames[1], image, cropRect)) {
            m_sideImage = image;
            m_sideCropRect = cropRect;
            m_sideCropWidget->setImage(m_sideImage);
        }
    }
}

void TurnaroundImageEditorDialog::schedulePreviewUpdate()
{
    if (m_previewThread != nullptr || m_previewWorker != nullptr) {
        m_previewObsolete = true;
        return;
    }
    startPreviewWorker();
}

void TurnaroundImageEditorDialog::startPreviewWorker()
{
    if (m_frontImage.isNull() && m_sideImage.isNull()) {
        m_previewImage = QImage();
        updatePreviewDisplay();
        return;
    }
    m_previewWorker = new TurnaroundPreviewWorker;
    m_previewWorker->setParameters(m_frontImage, m_frontCropRect, m_sideImage, m_sideCropRect);

    m_previewThread = new QThread;
    m_previewWorker->moveToThread(m_previewThread);
    connect(m_previewThread, &QThread::started, m_previewWorker, &TurnaroundPreviewWorker::process);
    connect(m_previewWorker, &TurnaroundPreviewWorker::finished, this, &TurnaroundImageEditorDialog::onPreviewReady);
    connect(m_previewWorker, &TurnaroundPreviewWorker::finished, m_previewThread, &QThread::quit);
    connect(m_previewThread, &QThread::finished, m_previewWorker, &QObject::deleteLater);
    connect(m_previewThread, &QThread::finished, m_previewThread, &QObject::deleteLater);
    m_previewThread->start();
}

void TurnaroundImageEditorDialog::updatePreviewDisplay()
{
    if (m_previewImage.isNull()) {
        m_previewLabel->setText(tr("No preview available"));
        m_previewLabel->setPixmap(QPixmap());
        return;
    }

    QPixmap pixmap = QPixmap::fromImage(m_previewImage.scaled(m_previewLabel->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
    m_previewLabel->setPixmap(pixmap);
    m_previewLabel->setText(QString());
}

void TurnaroundImageEditorDialog::generatePreviewImageSynchronously()
{
    bool hasFront = !m_frontImage.isNull();
    bool hasSide = !m_sideImage.isNull();
    if (!hasFront && !hasSide) {
        m_previewImage = QImage();
        return;
    }

    QImage frontScaled;
    if (hasFront) {
        QRect frontCrop = m_frontCropRect.isValid() ? m_frontCropRect.normalized() : QRect(QPoint(0, 0), m_frontImage.size());
        QImage frontCropped = m_frontImage.copy(frontCrop);
        frontScaled = frontCropped.scaledToHeight(512, Qt::SmoothTransformation);
    }

    QImage sideScaled;
    if (hasSide && m_sideCropRect.isValid()) {
        QRect sideCrop = m_sideCropRect.normalized();
        QImage sideCropped = m_sideImage.copy(sideCrop);
        sideScaled = sideCropped.scaledToHeight(512, Qt::SmoothTransformation);
    }

    if (hasFront && hasSide && !sideScaled.isNull()) {
        QRect sideCrop = m_sideCropRect.normalized();
        QImage sideCropped = m_sideImage.copy(sideCrop);
        QImage sideScaled = sideCropped.scaledToHeight(512, Qt::SmoothTransformation);
        int combinedWidth = frontScaled.width() + sideScaled.width();
        int finalWidth = std::max({ combinedWidth, frontScaled.width() * 2, sideScaled.width() * 2, 512 * 2 });
        int leftHalfWidth = finalWidth / 2;
        int rightHalfWidth = finalWidth - leftHalfWidth;
        int frontX = (leftHalfWidth - frontScaled.width()) / 2;
        int sideX = leftHalfWidth + (rightHalfWidth - sideScaled.width()) / 2;
        QImage result(finalWidth, 512, QImage::Format_RGBA8888);
        result.fill(Qt::white);
        QPainter painter(&result);
        painter.drawImage(frontX, 0, frontScaled);
        painter.drawImage(sideX, 0, sideScaled);
        m_previewImage = result;
    } else if (hasSide && !sideScaled.isNull()) {
        int finalWidth = std::max(sideScaled.width() * 2, 512 * 2);
        int leftHalfWidth = finalWidth / 2;
        int sideX = leftHalfWidth + (leftHalfWidth - sideScaled.width()) / 2;
        QImage result(finalWidth, 512, QImage::Format_RGBA8888);
        result.fill(Qt::white);
        QPainter painter(&result);
        painter.drawImage(sideX, 0, sideScaled);
        m_previewImage = result;
    } else {
        int finalWidth = std::max(frontScaled.width() * 2, 512 * 2);
        int leftHalfWidth = finalWidth / 2;
        int frontX = (leftHalfWidth - frontScaled.width()) / 2;
        QImage result(finalWidth, 512, QImage::Format_RGBA8888);
        result.fill(Qt::white);
        QPainter painter(&result);
        painter.drawImage(frontX, 0, frontScaled);
        m_previewImage = result;
    }
}

bool TurnaroundImageEditorDialog::loadImageFromFile(const QString& fileName, QImage& output, QRect& cropRect)
{
    QImage image(fileName);
    if (image.isNull())
        return false;
    output = image;
    cropRect = QRect(QPoint(0, 0), image.size());
    return true;
}
