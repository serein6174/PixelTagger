#include <QtTest>

#include <opencv2/core.hpp>

#include "processor/ImageConverter.h"
#include "processor/ImageProcessor.h"

class ImageProcessorTests final : public QObject {
    Q_OBJECT

private slots:
    void convertsBetweenQImageAndMat();
    void appliesGrayscaleAndBinary();
    void appliesMeanGaussianAndMedianBlur();
    void appliesCanny();
    void adjustsBrightnessAndContrast();
    void rejectsInvalidInputs();
};

void ImageProcessorTests::convertsBetweenQImageAndMat()
{
    QImage source(2, 1, QImage::Format_RGB888);
    source.setPixelColor(0, 0, QColor(255, 0, 0));
    source.setPixelColor(1, 0, QColor(0, 255, 0));

    const Result<cv::Mat> matResult = ImageConverter::toMat(source);
    QVERIFY2(matResult.isSuccess(), qPrintable(matResult.error()));
    QCOMPARE(matResult.value().type(), CV_8UC3);
    QVERIFY(matResult.value().at<cv::Vec3b>(0, 0) == cv::Vec3b(0, 0, 255));

    const Result<QImage> imageResult = ImageConverter::toQImage(matResult.value());
    QVERIFY2(imageResult.isSuccess(), qPrintable(imageResult.error()));
    QCOMPARE(imageResult.value().pixelColor(0, 0), QColor(255, 0, 0));
    QCOMPARE(imageResult.value().pixelColor(1, 0), QColor(0, 255, 0));
}

void ImageProcessorTests::appliesGrayscaleAndBinary()
{
    ImageProcessor processor;
    cv::Mat source(1, 2, CV_8UC3);
    source.at<cv::Vec3b>(0, 0) = cv::Vec3b(0, 0, 0);
    source.at<cv::Vec3b>(0, 1) = cv::Vec3b(255, 255, 255);

    const Result<cv::Mat> grayResult = processor.grayscale(source);
    QVERIFY2(grayResult.isSuccess(), qPrintable(grayResult.error()));
    QCOMPARE(grayResult.value().type(), CV_8UC1);
    QCOMPARE(grayResult.value().cols, 2);
    QCOMPARE(grayResult.value().rows, 1);

    const Result<cv::Mat> binaryResult = processor.binary(source, 128);
    QVERIFY2(binaryResult.isSuccess(), qPrintable(binaryResult.error()));
    QCOMPARE(binaryResult.value().at<uchar>(0, 0), static_cast<uchar>(0));
    QCOMPARE(binaryResult.value().at<uchar>(0, 1), static_cast<uchar>(255));
}

void ImageProcessorTests::appliesMeanGaussianAndMedianBlur()
{
    ImageProcessor processor;
    cv::Mat source(7, 7, CV_8UC1, cv::Scalar(0));
    source.at<uchar>(3, 3) = 255;

    const Result<cv::Mat> meanResult = processor.meanBlur(source, 3);
    QVERIFY2(meanResult.isSuccess(), qPrintable(meanResult.error()));
    QVERIFY(meanResult.value().at<uchar>(3, 3) > 0);
    QVERIFY(meanResult.value().at<uchar>(3, 3) < 255);

    const Result<cv::Mat> gaussianResult = processor.gaussianBlur(source, 3);
    QVERIFY2(gaussianResult.isSuccess(), qPrintable(gaussianResult.error()));
    QVERIFY(gaussianResult.value().at<uchar>(3, 3) > 0);
    QVERIFY(gaussianResult.value().at<uchar>(3, 3) < 255);

    const Result<cv::Mat> medianResult = processor.medianBlur(source, 3);
    QVERIFY2(medianResult.isSuccess(), qPrintable(medianResult.error()));
    QCOMPARE(medianResult.value().at<uchar>(3, 3), static_cast<uchar>(0));
}

void ImageProcessorTests::appliesCanny()
{
    ImageProcessor processor;
    cv::Mat source(40, 40, CV_8UC1, cv::Scalar(0));
    source.colRange(20, 40).setTo(cv::Scalar(255));

    const Result<cv::Mat> result = processor.canny(source, 50, 150);
    QVERIFY2(result.isSuccess(), qPrintable(result.error()));
    QCOMPARE(result.value().type(), CV_8UC1);
    QVERIFY(cv::countNonZero(result.value()) > 0);
}

void ImageProcessorTests::adjustsBrightnessAndContrast()
{
    ImageProcessor processor;
    cv::Mat source(1, 2, CV_8UC1);
    source.at<uchar>(0, 0) = 50;
    source.at<uchar>(0, 1) = 200;

    const Result<cv::Mat> brightResult = processor.adjustBrightness(source, 40);
    QVERIFY2(brightResult.isSuccess(), qPrintable(brightResult.error()));
    QCOMPARE(brightResult.value().at<uchar>(0, 0), static_cast<uchar>(90));
    QCOMPARE(brightResult.value().at<uchar>(0, 1), static_cast<uchar>(240));

    const Result<cv::Mat> contrastResult = processor.adjustContrast(source, 2.0);
    QVERIFY2(contrastResult.isSuccess(), qPrintable(contrastResult.error()));
    QCOMPARE(contrastResult.value().at<uchar>(0, 0), static_cast<uchar>(100));
    QCOMPARE(contrastResult.value().at<uchar>(0, 1), static_cast<uchar>(255));
}

void ImageProcessorTests::rejectsInvalidInputs()
{
    ImageProcessor processor;
    const cv::Mat source(2, 2, CV_8UC1, cv::Scalar(128));

    QVERIFY(!processor.grayscale(cv::Mat{}).isSuccess());
    QVERIFY(!processor.binary(source, -1).isSuccess());
    QVERIFY(!processor.binary(source, 256).isSuccess());
    QVERIFY(!processor.meanBlur(source, 2).isSuccess());
    QVERIFY(!processor.gaussianBlur(source, 17).isSuccess());
    QVERIFY(!processor.medianBlur(source, 0).isSuccess());
    QVERIFY(!processor.canny(source, 150, 50).isSuccess());
    QVERIFY(!processor.adjustBrightness(source, 101).isSuccess());
    QVERIFY(!processor.adjustContrast(source, 0.0).isSuccess());
}

QTEST_APPLESS_MAIN(ImageProcessorTests)

#include "ImageProcessorTests.moc"
