#pragma once

#include <QPointF>
#include <QRectF>
#include <QSizeF>

class CoordinateMapper {
public:
    void setImageSize(const QSizeF& imageSize);
    void setViewportRect(const QRectF& viewportRect);

    QPointF widgetToImage(const QPointF& point) const;
    QPointF imageToWidget(const QPointF& point) const;

    QRectF widgetToImage(const QRectF& rect) const;
    QRectF imageToWidget(const QRectF& rect) const;

    QRectF imageBounds() const;
    QRectF viewportRect() const;

private:
    QSizeF imageSize_;
    QRectF viewportRect_;
};
