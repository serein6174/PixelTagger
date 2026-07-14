#include "view/canvas/ImageCanvas.h"

#include <QMouseEvent>
#include <QPainter>
#include <QPen>

namespace {
constexpr double kMinimumAnnotationSize = 3.0;
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
    updateMapper();
    update();
}

void ImageCanvas::setAnnotations(const QVector<AnnotationRenderData>& annotations)
{
    annotations_ = annotations;
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
    if (image_.isNull() || event->button() != Qt::LeftButton) {
        QWidget::mousePressEvent(event);
        return;
    }

    updateMapper();
    if (!mapper_.viewportRect().contains(event->position())) {
        return;
    }

    drawingAnnotation_ = true;
    dragStartImagePoint_ = mapper_.widgetToImage(event->position());
    dragCurrentImagePoint_ = dragStartImagePoint_;
    update();
}

void ImageCanvas::mouseMoveEvent(QMouseEvent* event)
{
    if (!drawingAnnotation_) {
        QWidget::mouseMoveEvent(event);
        return;
    }

    dragCurrentImagePoint_ = mapper_.widgetToImage(event->position());
    update();
}

void ImageCanvas::mouseReleaseEvent(QMouseEvent* event)
{
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

void ImageCanvas::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
    updateMapper();
}

QRectF ImageCanvas::imageViewportRect() const
{
    if (image_.isNull()) {
        return QRectF{};
    }

    QSizeF targetSize = image_.size();
    targetSize.scale(size(), Qt::KeepAspectRatio);

    return QRectF(
        (width() - targetSize.width()) / 2.0,
        (height() - targetSize.height()) / 2.0,
        targetSize.width(),
        targetSize.height()
    );
}

void ImageCanvas::updateMapper()
{
    mapper_.setImageSize(image_.isNull() ? QSizeF{} : QSizeF(image_.size()));
    mapper_.setViewportRect(imageViewportRect());
}

void ImageCanvas::drawAnnotations(QPainter& painter)
{
    painter.setRenderHint(QPainter::Antialiasing, true);

    for (const AnnotationRenderData& annotation : annotations_) {
        const QRectF widgetRect = mapper_.imageToWidget(annotation.imageRect);

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
