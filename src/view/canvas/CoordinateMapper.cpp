#include "view/canvas/CoordinateMapper.h"

void CoordinateMapper::setImageSize(const QSizeF& imageSize)
{
    imageSize_ = imageSize;
}

void CoordinateMapper::setViewportRect(const QRectF& viewportRect)
{
    viewportRect_ = viewportRect;
}

QPointF CoordinateMapper::widgetToImage(const QPointF& point) const
{
    if (imageSize_.isEmpty() || viewportRect_.isEmpty()) {
        return QPointF{};
    }

    const double x = (point.x() - viewportRect_.left()) * imageSize_.width() / viewportRect_.width();
    const double y = (point.y() - viewportRect_.top()) * imageSize_.height() / viewportRect_.height();
    return QPointF(x, y);
}

QPointF CoordinateMapper::imageToWidget(const QPointF& point) const
{
    if (imageSize_.isEmpty() || viewportRect_.isEmpty()) {
        return QPointF{};
    }

    const double x = viewportRect_.left() + point.x() * viewportRect_.width() / imageSize_.width();
    const double y = viewportRect_.top() + point.y() * viewportRect_.height() / imageSize_.height();
    return QPointF(x, y);
}

QRectF CoordinateMapper::widgetToImage(const QRectF& rect) const
{
    return QRectF(widgetToImage(rect.topLeft()), widgetToImage(rect.bottomRight())).normalized();
}

QRectF CoordinateMapper::imageToWidget(const QRectF& rect) const
{
    return QRectF(imageToWidget(rect.topLeft()), imageToWidget(rect.bottomRight())).normalized();
}

QRectF CoordinateMapper::imageBounds() const
{
    return QRectF(QPointF(0.0, 0.0), imageSize_);
}

QRectF CoordinateMapper::viewportRect() const
{
    return viewportRect_;
}
