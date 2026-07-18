#include <QtTest>

#include <QSignalSpy>

#include "view/canvas/ImageCanvas.h"

namespace {

void prepareCanvas(ImageCanvas& canvas)
{
    canvas.setMinimumSize(0, 0);
    canvas.resize(200, 200);

    QImage image(100, 100, QImage::Format_RGB32);
    image.fill(Qt::white);
    canvas.setImage(image);
}

AnnotationRenderItem selectedAnnotation()
{
    AnnotationRenderItem annotation;
    annotation.id = 1;
    annotation.imageRect = QRectF(20, 20, 20, 20);
    annotation.labelName = QStringLiteral("object");
    annotation.color = Qt::red;
    annotation.selected = true;
    return annotation;
}

} // namespace

class ImageCanvasInteractionTests final : public QObject {
    Q_OBJECT

private slots:
    void zoomsAndPansWithoutChangingModelData();
    void movesSelectedAnnotationInImageCoordinates();
    void resizesSelectedAnnotationFromCornerHandle();
};

void ImageCanvasInteractionTests::zoomsAndPansWithoutChangingModelData()
{
    ImageCanvas canvas;
    prepareCanvas(canvas);

    QCOMPARE(canvas.zoomFactor(), 1.0);
    QCOMPARE(canvas.panOffset(), QPointF{});

    canvas.zoomIn();
    QCOMPARE(canvas.zoomFactor(), 1.25);

    QTest::mousePress(
        &canvas,
        Qt::MiddleButton,
        Qt::NoModifier,
        QPoint(100, 100)
    );
    QTest::mouseMove(&canvas, QPoint(120, 110));
    QTest::mouseRelease(
        &canvas,
        Qt::MiddleButton,
        Qt::NoModifier,
        QPoint(120, 110)
    );
    QVERIFY(qAbs(canvas.panOffset().x() - 20.0) < 0.5);
    QVERIFY(qAbs(canvas.panOffset().y() - 10.0) < 0.5);

    canvas.resetView();
    QCOMPARE(canvas.zoomFactor(), 1.0);
    QCOMPARE(canvas.panOffset(), QPointF{});
}

void ImageCanvasInteractionTests::movesSelectedAnnotationInImageCoordinates()
{
    ImageCanvas canvas;
    prepareCanvas(canvas);
    canvas.setAnnotations({selectedAnnotation()});
    QSignalSpy rectSpy(
        &canvas,
        &ImageCanvas::selectedAnnotationRectChangeRequested
    );

    QTest::mousePress(
        &canvas,
        Qt::LeftButton,
        Qt::NoModifier,
        QPoint(60, 60)
    );
    QTest::mouseMove(&canvas, QPoint(80, 90));
    QTest::mouseRelease(
        &canvas,
        Qt::LeftButton,
        Qt::NoModifier,
        QPoint(80, 90)
    );

    QCOMPARE(rectSpy.count(), 1);
    QCOMPARE(
        rectSpy.takeFirst().at(0).toRectF(),
        QRectF(30, 35, 20, 20)
    );
}

void ImageCanvasInteractionTests::resizesSelectedAnnotationFromCornerHandle()
{
    ImageCanvas canvas;
    prepareCanvas(canvas);
    canvas.setAnnotations({selectedAnnotation()});
    QSignalSpy rectSpy(
        &canvas,
        &ImageCanvas::selectedAnnotationRectChangeRequested
    );

    QTest::mousePress(
        &canvas,
        Qt::LeftButton,
        Qt::NoModifier,
        QPoint(40, 40)
    );
    QTest::mouseMove(&canvas, QPoint(30, 20));
    QTest::mouseRelease(
        &canvas,
        Qt::LeftButton,
        Qt::NoModifier,
        QPoint(30, 20)
    );

    QCOMPARE(rectSpy.count(), 1);
    QCOMPARE(
        rectSpy.takeFirst().at(0).toRectF(),
        QRectF(15, 10, 25, 30)
    );
}

QTEST_MAIN(ImageCanvasInteractionTests)

#include "ImageCanvasInteractionTests.moc"
