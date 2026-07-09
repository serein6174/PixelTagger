#include "view/ImageCanvas.h"

#include <QPainter>

ImageCanvas::ImageCanvas(QWidget* parent)
    : QWidget(parent)
{
    setMinimumSize(640, 480);
    setAutoFillBackground(true);
}

void ImageCanvas::setImage(const QImage& image)
{
    image_ = image;
    update();
}

void ImageCanvas::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);

    QPainter painter(this);
    painter.fillRect(rect(), QColor(28, 30, 34));

    if (image_.isNull()) {
        painter.setPen(QColor(190, 190, 190));
        painter.drawText(rect(), Qt::AlignCenter, QStringLiteral("打开一张图片开始"));
        return;
    }

    QSize targetSize = image_.size();
    targetSize.scale(size(), Qt::KeepAspectRatio);

    const QRect targetRect(
        (width() - targetSize.width()) / 2,
        (height() - targetSize.height()) / 2,
        targetSize.width(),
        targetSize.height()
    );

    painter.setRenderHint(QPainter::SmoothPixmapTransform, true);
    painter.drawImage(targetRect, image_);
}
