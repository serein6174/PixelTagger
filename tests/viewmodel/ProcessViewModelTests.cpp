#include <QtTest>

#include <QDir>
#include <QFileInfo>
#include <QTemporaryDir>

#include "processor/ImageProcessor.h"
#include "viewmodel/ProcessViewModel.h"

namespace {

QImage makeProcessImage()
{
    QImage image(4, 2, QImage::Format_RGB888);
    image.fill(Qt::black);
    for (int y = 0; y < image.height(); ++y) {
        image.setPixelColor(2, y, QColor(120, 160, 200));
        image.setPixelColor(3, y, Qt::white);
    }
    return image;
}

} // namespace

class ProcessViewModelTests final : public QObject {
    Q_OBJECT

private slots:
    void previewsEachOperationFromOriginalSource();
    void resetsPreviewWhenSourceChanges();
    void rejectsInvalidParametersWithoutReplacingPreview();
    void savesPreviewToExplicitPath();
};

void ProcessViewModelTests::previewsEachOperationFromOriginalSource()
{
    ImageProcessor processor;
    ProcessViewModel viewModel(processor);
    viewModel.setSourceImage(makeProcessImage());

    QSignalSpy previewSpy(&viewModel, &ProcessViewModel::previewChanged);
    QSignalSpy availabilitySpy(
        &viewModel,
        &ProcessViewModel::previewAvailabilityChanged
    );

    viewModel.previewBrightness(50);
    QVERIFY(viewModel.hasPreview());
    QCOMPARE(availabilitySpy.count(), 1);
    const QColor brightPixel = viewModel.previewImage().pixelColor(0, 0);
    QCOMPARE(brightPixel.red(), 50);

    viewModel.previewGrayscale();
    QCOMPARE(viewModel.previewImage().format(), QImage::Format_Grayscale8);
    QCOMPARE(viewModel.previewImage().pixelColor(0, 0).red(), 0);

    viewModel.previewBinary(128);
    const int binaryValue = viewModel.previewImage().pixelColor(2, 0).red();
    QVERIFY(binaryValue == 0 || binaryValue == 255);

    viewModel.previewMeanBlur(3);
    QCOMPARE(viewModel.previewImage().size(), viewModel.sourceImage().size());

    viewModel.previewGaussianBlur(3);
    QCOMPARE(viewModel.previewImage().size(), viewModel.sourceImage().size());

    viewModel.previewMedianBlur(3);
    QCOMPARE(viewModel.previewImage().size(), viewModel.sourceImage().size());

    viewModel.previewCanny(50, 150);
    QCOMPARE(viewModel.previewImage().format(), QImage::Format_Grayscale8);

    viewModel.previewContrast(1.5);
    QCOMPARE(viewModel.previewImage().pixelColor(0, 0), QColor(Qt::black));
    QCOMPARE(previewSpy.count(), 8);
    QCOMPARE(availabilitySpy.count(), 1);
}

void ProcessViewModelTests::resetsPreviewWhenSourceChanges()
{
    ImageProcessor processor;
    ProcessViewModel viewModel(processor);
    viewModel.setSourceImage(makeProcessImage());
    viewModel.previewGrayscale();
    QVERIFY(viewModel.hasPreview());

    QSignalSpy availabilitySpy(
        &viewModel,
        &ProcessViewModel::previewAvailabilityChanged
    );

    QImage nextImage(3, 3, QImage::Format_RGB888);
    nextImage.fill(Qt::red);
    viewModel.setSourceImage(nextImage);

    QVERIFY(!viewModel.hasPreview());
    QCOMPARE(viewModel.displayImage(), nextImage);
    QCOMPARE(availabilitySpy.count(), 1);
    QCOMPARE(availabilitySpy.at(0).at(0).toBool(), false);

    viewModel.previewBrightness(20);
    QVERIFY(viewModel.hasPreview());
    viewModel.resetPreview();
    QVERIFY(!viewModel.hasPreview());
    QCOMPARE(viewModel.displayImage(), nextImage);
}

void ProcessViewModelTests::rejectsInvalidParametersWithoutReplacingPreview()
{
    ImageProcessor processor;
    ProcessViewModel viewModel(processor);
    viewModel.setSourceImage(makeProcessImage());
    viewModel.previewGrayscale();
    const QImage validPreview = viewModel.previewImage();

    QSignalSpy errorSpy(&viewModel, &ProcessViewModel::errorOccurred);
    viewModel.previewBinary(300);
    viewModel.previewMeanBlur(2);
    viewModel.previewGaussianBlur(16);
    viewModel.previewMedianBlur(1);
    viewModel.previewCanny(180, 20);
    viewModel.previewBrightness(-101);
    viewModel.previewContrast(3.1);

    QCOMPARE(errorSpy.count(), 7);
    QCOMPARE(viewModel.previewImage(), validPreview);

    ProcessViewModel emptyViewModel(processor);
    QSignalSpy emptyErrorSpy(&emptyViewModel, &ProcessViewModel::errorOccurred);
    emptyViewModel.previewGrayscale();
    QCOMPARE(emptyErrorSpy.count(), 1);
    QVERIFY(!emptyViewModel.hasPreview());
}

void ProcessViewModelTests::savesPreviewToExplicitPath()
{
    ImageProcessor processor;
    ProcessViewModel viewModel(processor);
    viewModel.setSourceImage(makeProcessImage());
    viewModel.previewGrayscale();

    QTemporaryDir directory(
        QDir::currentPath() + QStringLiteral("/process-preview-test-XXXXXX")
    );
    QVERIFY(directory.isValid());
    const QString path = directory.filePath(QStringLiteral("preview.png"));

    QSignalSpy statusSpy(&viewModel, &ProcessViewModel::statusChanged);
    QSignalSpy errorSpy(&viewModel, &ProcessViewModel::errorOccurred);
    viewModel.savePreview(path);

    QVERIFY(QFileInfo::exists(path));
    QCOMPARE(errorSpy.count(), 0);
    QVERIFY(statusSpy.count() >= 1);
}

QTEST_APPLESS_MAIN(ProcessViewModelTests)

#include "ProcessViewModelTests.moc"
