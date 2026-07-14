#pragma once

#include <QImage>
#include <QPointF>
#include <QVector>
#include <QWidget>

#include "view/canvas/CoordinateMapper.h"
#include "common/presentation/AnnotationRenderData.h"

class ImageCanvas : public QWidget {
    Q_OBJECT

public:
    explicit ImageCanvas(QWidget* parent = nullptr);

public slots:
    void setImage(const QImage& image);
    void setAnnotations(const QVector<AnnotationRenderData>& annotations);

signals:
    void annotationCreateRequested(const QRectF& imageRect);

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;

private:
    QRectF imageViewportRect() const;
    void updateMapper();
    void drawAnnotations(QPainter& painter);
    void drawPreview(QPainter& painter);

    QImage image_;
    QVector<AnnotationRenderData> annotations_;
    CoordinateMapper mapper_;

    bool drawingAnnotation_ = false;
    QPointF dragStartImagePoint_;
    QPointF dragCurrentImagePoint_;
};
