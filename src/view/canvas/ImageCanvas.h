#pragma once

#include <QImage>
#include <QPointF>
#include <QVector>
#include <QWidget>

#include "view/canvas/CoordinateMapper.h"
#include "common/presentation/AnnotationRenderItem.h"

class ImageCanvas : public QWidget {
    Q_OBJECT

public:
    explicit ImageCanvas(QWidget* parent = nullptr);

    double zoomFactor() const noexcept;
    QPointF panOffset() const noexcept;

public slots:
    void setImage(const QImage& image);
    void setAnnotations(const QVector<AnnotationRenderItem>& annotations);
    void zoomIn();
    void zoomOut();
    void resetView();

signals:
    void annotationCreateRequested(const QRectF& imageRect);
    void annotationSelectRequested(AnnotationId annotationId);
    void annotationSelectionClearRequested();
    void selectedAnnotationRectChangeRequested(const QRectF& imageRect);

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;

private:
    enum class AnnotationEditMode {
        None,
        Move,
        ResizeTopLeft,
        ResizeTopRight,
        ResizeBottomLeft,
        ResizeBottomRight
    };

    QRectF imageViewportRect() const;
    void updateMapper();
    void applyZoom(double factor, const QPointF& anchor);
    void clampPanOffset();
    AnnotationId annotationAt(const QPointF& widgetPoint) const;
    const AnnotationRenderItem* annotationById(AnnotationId annotationId) const;
    const AnnotationRenderItem* selectedAnnotation() const;
    AnnotationEditMode resizeHandleAt(
        const QPointF& widgetPoint,
        const AnnotationRenderItem& annotation
    ) const;
    void beginAnnotationEdit(
        const AnnotationRenderItem& annotation,
        AnnotationEditMode mode,
        const QPointF& imagePoint
    );
    QRectF annotationEditRect(const QPointF& imagePoint) const;
    QRectF visibleAnnotationRect(const AnnotationRenderItem& annotation) const;
    void drawAnnotations(QPainter& painter);
    void drawSelectionHandles(QPainter& painter, const QRectF& widgetRect);
    void drawPreview(QPainter& painter);

    QImage image_;
    QVector<AnnotationRenderItem> annotations_;
    CoordinateMapper mapper_;

    double zoomFactor_ = 1.0;
    QPointF panOffset_;
    bool panning_ = false;
    QPointF lastPanWidgetPoint_;

    bool drawingAnnotation_ = false;
    QPointF dragStartImagePoint_;
    QPointF dragCurrentImagePoint_;

    AnnotationEditMode annotationEditMode_ = AnnotationEditMode::None;
    AnnotationId editingAnnotationId_ = -1;
    QRectF originalAnnotationRect_;
    QRectF previewAnnotationRect_;
};
