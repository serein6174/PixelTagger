#include "view/canvas/ImageCanvas.h"

#include <QMouseEvent>
#include <QPainter>
#include <QPen>
#include <QWheelEvent>

#include <algorithm>
#include <cmath>

namespace {
constexpr double kMinimumAnnotationSize = 3.0;
constexpr double kMinimumZoom = 1.0;
constexpr double kMaximumZoom = 20.0;
constexpr double kZoomStep = 1.25;
constexpr double kHandleSize = 10.0;
}

ImageCanvas::ImageCanvas(QWidget* parent)
    : QWidget(parent)
{
    setMinimumSize(640, 480);
    setAutoFillBackground(true);
    setMouseTracking(true);
    setFocusPolicy(Qt::StrongFocus);
}

void ImageCanvas::setImage(const QImage& image)
{
    image_ = image;
    drawingAnnotation_ = false;
    annotationEditMode_ = AnnotationEditMode::None;
    editingAnnotationId_ = -1;
    zoomFactor_ = 1.0;
    panOffset_ = QPointF{};
    updateMapper();
    update();
}

void ImageCanvas::setAnnotations(const QVector<AnnotationRenderData>& annotations)
{
    annotations_ = annotations;
    update();
}

double ImageCanvas::zoomFactor() const noexcept
{
    return zoomFactor_;
}

QPointF ImageCanvas::panOffset() const noexcept
{
    return panOffset_;
}

void ImageCanvas::zoomIn()
{
    applyZoom(zoomFactor_ * kZoomStep, rect().center());
}

void ImageCanvas::zoomOut()
{
    applyZoom(zoomFactor_ / kZoomStep, rect().center());
}

void ImageCanvas::resetView()
{
    zoomFactor_ = 1.0;
    panOffset_ = QPointF{};
    updateMapper();
    update();
}

void ImageCanvas::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);

    updateMapper();

    QPainter painter(this);
    painter.fillRect(rect(), QColor(28, 30, 34));

    if (image_.isNull()) {
        painter.setPen(QColor(190, 190, 190));
        painter.drawText(rect(), Qt::AlignCenter, QStringLiteral("打开一张图片开始"));
        return;
    }

    painter.setRenderHint(QPainter::SmoothPixmapTransform, true);
    painter.drawImage(mapper_.viewportRect(), image_);

    drawAnnotations(painter);
    drawPreview(painter);
}

void ImageCanvas::mousePressEvent(QMouseEvent* event)
{
    if (image_.isNull()) {
        QWidget::mousePressEvent(event);
        return;
    }

    if (event->button() == Qt::MiddleButton) {
        panning_ = true;
        lastPanWidgetPoint_ = event->position();
        setCursor(Qt::ClosedHandCursor);
        event->accept();
        return;
    }

    if (event->button() != Qt::LeftButton) {
        QWidget::mousePressEvent(event);
        return;
    }

    updateMapper();
    if (!mapper_.viewportRect().contains(event->position())) {
        return;
    }

    const AnnotationRenderData* selected = selectedAnnotation();
    if (selected) {
        const AnnotationEditMode handle = resizeHandleAt(event->position(), *selected);
        if (handle != AnnotationEditMode::None) {
            beginAnnotationEdit(
                *selected,
                handle,
                mapper_.widgetToImage(event->position())
            );
            return;
        }
    }

    const AnnotationId hitAnnotationId = annotationAt(event->position());
    if (hitAnnotationId >= 0) {
        drawingAnnotation_ = false;
        emit annotationSelectRequested(hitAnnotationId);
        const AnnotationRenderData* annotation = annotationById(hitAnnotationId);
        if (annotation) {
            beginAnnotationEdit(
                *annotation,
                AnnotationEditMode::Move,
                mapper_.widgetToImage(event->position())
            );
        }
        update();
        return;
    }

    emit annotationSelectionClearRequested();
    drawingAnnotation_ = true;
    dragStartImagePoint_ = mapper_.widgetToImage(event->position());
    dragCurrentImagePoint_ = dragStartImagePoint_;
    update();
}

void ImageCanvas::mouseMoveEvent(QMouseEvent* event)
{
    if (panning_) {
        panOffset_ += event->position() - lastPanWidgetPoint_;
        lastPanWidgetPoint_ = event->position();
        clampPanOffset();
        updateMapper();
        update();
        return;
    }

    if (annotationEditMode_ != AnnotationEditMode::None) {
        previewAnnotationRect_ = annotationEditRect(
            mapper_.widgetToImage(event->position())
        );
        update();
        return;
    }

    if (!drawingAnnotation_) {
        QWidget::mouseMoveEvent(event);
        return;
    }

    dragCurrentImagePoint_ = mapper_.widgetToImage(event->position());
    update();
}

void ImageCanvas::mouseReleaseEvent(QMouseEvent* event)
{
    if (panning_ && event->button() == Qt::MiddleButton) {
        panning_ = false;
        unsetCursor();
        event->accept();
        return;
    }

    if (annotationEditMode_ != AnnotationEditMode::None &&
        event->button() == Qt::LeftButton) {
        previewAnnotationRect_ = annotationEditRect(
            mapper_.widgetToImage(event->position())
        );

        const QRectF changedRect = previewAnnotationRect_;
        annotationEditMode_ = AnnotationEditMode::None;
        editingAnnotationId_ = -1;

        if (changedRect != originalAnnotationRect_ &&
            changedRect.width() >= kMinimumAnnotationSize &&
            changedRect.height() >= kMinimumAnnotationSize) {
            emit selectedAnnotationRectChangeRequested(changedRect);
        }
        update();
        return;
    }

    if (!drawingAnnotation_ || event->button() != Qt::LeftButton) {
        QWidget::mouseReleaseEvent(event);
        return;
    }

    drawingAnnotation_ = false;
    dragCurrentImagePoint_ = mapper_.widgetToImage(event->position());

    QRectF imageRect = QRectF(dragStartImagePoint_, dragCurrentImagePoint_)
                           .normalized()
                           .intersected(mapper_.imageBounds());

    if (imageRect.width() >= kMinimumAnnotationSize &&
        imageRect.height() >= kMinimumAnnotationSize) {
        emit annotationCreateRequested(imageRect);
    }

    update();
}

void ImageCanvas::wheelEvent(QWheelEvent* event)
{
    if (image_.isNull() || event->angleDelta().y() == 0) {
        QWidget::wheelEvent(event);
        return;
    }

    const double steps = event->angleDelta().y() / 120.0;
    applyZoom(
        zoomFactor_ * std::pow(kZoomStep, steps),
        event->position()
    );
    event->accept();
}

void ImageCanvas::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
    clampPanOffset();
    updateMapper();
}

QRectF ImageCanvas::imageViewportRect() const
{
    if (image_.isNull()) {
        return QRectF{};
    }

    QSizeF targetSize = image_.size();
    targetSize.scale(size(), Qt::KeepAspectRatio);
    targetSize *= zoomFactor_;

    return QRectF(
        (width() - targetSize.width()) / 2.0 + panOffset_.x(),
        (height() - targetSize.height()) / 2.0 + panOffset_.y(),
        targetSize.width(),
        targetSize.height()
    );
}

void ImageCanvas::updateMapper()
{
    mapper_.setImageSize(image_.isNull() ? QSizeF{} : QSizeF(image_.size()));
    mapper_.setViewportRect(imageViewportRect());
}

void ImageCanvas::applyZoom(double factor, const QPointF& anchor)
{
    if (image_.isNull()) {
        return;
    }

    updateMapper();
    const QPointF anchorImagePoint = mapper_.widgetToImage(anchor);
    const double boundedFactor = std::clamp(
        factor,
        kMinimumZoom,
        kMaximumZoom
    );
    if (qFuzzyCompare(zoomFactor_, boundedFactor)) {
        return;
    }

    zoomFactor_ = boundedFactor;
    if (qFuzzyCompare(zoomFactor_, kMinimumZoom)) {
        panOffset_ = QPointF{};
    } else {
        updateMapper();
        panOffset_ += anchor - mapper_.imageToWidget(anchorImagePoint);
        clampPanOffset();
    }
    updateMapper();
    update();
}

void ImageCanvas::clampPanOffset()
{
    if (image_.isNull() || qFuzzyCompare(zoomFactor_, kMinimumZoom)) {
        panOffset_ = QPointF{};
        return;
    }

    QSizeF viewportSize = image_.size();
    viewportSize.scale(size(), Qt::KeepAspectRatio);
    viewportSize *= zoomFactor_;

    const double maximumX = std::max(0.0, (viewportSize.width() - width()) / 2.0);
    const double maximumY = std::max(0.0, (viewportSize.height() - height()) / 2.0);
    panOffset_.setX(std::clamp(panOffset_.x(), -maximumX, maximumX));
    panOffset_.setY(std::clamp(panOffset_.y(), -maximumY, maximumY));
}

AnnotationId ImageCanvas::annotationAt(const QPointF& widgetPoint) const
{
    for (auto it = annotations_.crbegin(); it != annotations_.crend(); ++it) {
        if (mapper_.imageToWidget(it->imageRect).contains(widgetPoint)) {
            return it->id;
        }
    }
    return -1;
}

const AnnotationRenderData* ImageCanvas::annotationById(
    AnnotationId annotationId
) const
{
    for (const AnnotationRenderData& annotation : annotations_) {
        if (annotation.id == annotationId) {
            return &annotation;
        }
    }
    return nullptr;
}

const AnnotationRenderData* ImageCanvas::selectedAnnotation() const
{
    for (const AnnotationRenderData& annotation : annotations_) {
        if (annotation.selected) {
            return &annotation;
        }
    }
    return nullptr;
}

ImageCanvas::AnnotationEditMode ImageCanvas::resizeHandleAt(
    const QPointF& widgetPoint,
    const AnnotationRenderData& annotation
) const
{
    const QRectF widgetRect = mapper_.imageToWidget(annotation.imageRect);
    const double radius = kHandleSize / 2.0;
    const auto contains = [widgetPoint, radius](const QPointF& center) {
        return QRectF(
            center.x() - radius,
            center.y() - radius,
            kHandleSize,
            kHandleSize
        ).contains(widgetPoint);
    };

    if (contains(widgetRect.topLeft())) {
        return AnnotationEditMode::ResizeTopLeft;
    }
    if (contains(widgetRect.topRight())) {
        return AnnotationEditMode::ResizeTopRight;
    }
    if (contains(widgetRect.bottomLeft())) {
        return AnnotationEditMode::ResizeBottomLeft;
    }
    if (contains(widgetRect.bottomRight())) {
        return AnnotationEditMode::ResizeBottomRight;
    }
    return AnnotationEditMode::None;
}

void ImageCanvas::beginAnnotationEdit(
    const AnnotationRenderData& annotation,
    AnnotationEditMode mode,
    const QPointF& imagePoint
)
{
    drawingAnnotation_ = false;
    annotationEditMode_ = mode;
    editingAnnotationId_ = annotation.id;
    originalAnnotationRect_ = annotation.imageRect;
    previewAnnotationRect_ = annotation.imageRect;
    dragStartImagePoint_ = imagePoint;
}

QRectF ImageCanvas::annotationEditRect(const QPointF& imagePoint) const
{
    const QRectF bounds = mapper_.imageBounds();
    const QPointF boundedPoint(
        std::clamp(imagePoint.x(), bounds.left(), bounds.right()),
        std::clamp(imagePoint.y(), bounds.top(), bounds.bottom())
    );

    if (annotationEditMode_ == AnnotationEditMode::Move) {
        QPointF delta = imagePoint - dragStartImagePoint_;
        delta.setX(std::clamp(
            delta.x(),
            bounds.left() - originalAnnotationRect_.left(),
            bounds.right() - originalAnnotationRect_.right()
        ));
        delta.setY(std::clamp(
            delta.y(),
            bounds.top() - originalAnnotationRect_.top(),
            bounds.bottom() - originalAnnotationRect_.bottom()
        ));
        return originalAnnotationRect_.translated(delta);
    }

    QRectF rect = originalAnnotationRect_;
    switch (annotationEditMode_) {
    case AnnotationEditMode::ResizeTopLeft:
        rect.setTopLeft(boundedPoint);
        break;
    case AnnotationEditMode::ResizeTopRight:
        rect.setTopRight(boundedPoint);
        break;
    case AnnotationEditMode::ResizeBottomLeft:
        rect.setBottomLeft(boundedPoint);
        break;
    case AnnotationEditMode::ResizeBottomRight:
        rect.setBottomRight(boundedPoint);
        break;
    case AnnotationEditMode::None:
    case AnnotationEditMode::Move:
        break;
    }
    return rect.normalized().intersected(bounds);
}

QRectF ImageCanvas::visibleAnnotationRect(
    const AnnotationRenderData& annotation
) const
{
    if (annotation.id == editingAnnotationId_ &&
        annotationEditMode_ != AnnotationEditMode::None) {
        return previewAnnotationRect_;
    }
    return annotation.imageRect;
}

void ImageCanvas::drawAnnotations(QPainter& painter)
{
    painter.setRenderHint(QPainter::Antialiasing, true);

    for (const AnnotationRenderData& annotation : annotations_) {
        const QRectF widgetRect = mapper_.imageToWidget(
            visibleAnnotationRect(annotation)
        );

        QPen pen(annotation.color, annotation.selected ? 3.0 : 2.0);
        painter.setPen(pen);
        painter.setBrush(Qt::NoBrush);
        painter.drawRect(widgetRect);

        if (!annotation.labelName.isEmpty()) {
            const QRectF labelRect(widgetRect.topLeft(), QSizeF(72.0, 22.0));
            painter.fillRect(labelRect, annotation.color);
            painter.setPen(Qt::white);
            painter.drawText(labelRect.adjusted(6.0, 0.0, -4.0, 0.0),
                             Qt::AlignVCenter | Qt::AlignLeft,
                             annotation.labelName);
        }

        if (annotation.selected) {
            drawSelectionHandles(painter, widgetRect);
        }
    }
}

void ImageCanvas::drawSelectionHandles(QPainter& painter, const QRectF& widgetRect)
{
    const double radius = kHandleSize / 2.0;
    const QPointF handles[] = {
        widgetRect.topLeft(),
        widgetRect.topRight(),
        widgetRect.bottomLeft(),
        widgetRect.bottomRight()
    };

    painter.setPen(QPen(Qt::black, 1.0));
    painter.setBrush(Qt::white);
    for (const QPointF& center : handles) {
        painter.drawRect(QRectF(
            center.x() - radius,
            center.y() - radius,
            kHandleSize,
            kHandleSize
        ));
    }
}

void ImageCanvas::drawPreview(QPainter& painter)
{
    if (!drawingAnnotation_) {
        return;
    }

    const QRectF imageRect = QRectF(dragStartImagePoint_, dragCurrentImagePoint_)
                                 .normalized()
                                 .intersected(mapper_.imageBounds());
    const QRectF widgetRect = mapper_.imageToWidget(imageRect);

    QPen pen(QColor(255, 220, 90), 2.0, Qt::DashLine);
    painter.setPen(pen);
    painter.setBrush(QColor(255, 220, 90, 35));
    painter.drawRect(widgetRect);
}
