#include "processor/ImageProcessor.h"

#include <utility>

#include <opencv2/imgproc.hpp>

Result<cv::Mat> ImageProcessor::grayscale(const cv::Mat& source) const
{
    return toGrayscale(source);
}

Result<cv::Mat> ImageProcessor::binary(
    const cv::Mat& source,
    int threshold
) const
{
    if (threshold < 0 || threshold > 255) {
        return Result<cv::Mat>::failure(
            QStringLiteral("二值化阈值必须在 0 到 255 之间")
        );
    }

    Result<cv::Mat> grayResult = toGrayscale(source);
    if (!grayResult.isSuccess()) {
        return Result<cv::Mat>::failure(grayResult.error());
    }

    cv::Mat result;
    cv::threshold(
        grayResult.value(),
        result,
        threshold,
        255.0,
        cv::THRESH_BINARY
    );
    return Result<cv::Mat>::success(std::move(result));
}

Result<cv::Mat> ImageProcessor::meanBlur(
    const cv::Mat& source,
    int kernelSize
) const
{
    const Result<void> sourceValidation = validateSource(source);
    if (!sourceValidation.isSuccess()) {
        return Result<cv::Mat>::failure(sourceValidation.error());
    }
    const Result<void> kernelValidation = validateKernelSize(kernelSize);
    if (!kernelValidation.isSuccess()) {
        return Result<cv::Mat>::failure(kernelValidation.error());
    }

    cv::Mat result;
    cv::blur(source, result, cv::Size(kernelSize, kernelSize));
    return Result<cv::Mat>::success(std::move(result));
}

Result<cv::Mat> ImageProcessor::gaussianBlur(
    const cv::Mat& source,
    int kernelSize
) const
{
    const Result<void> sourceValidation = validateSource(source);
    if (!sourceValidation.isSuccess()) {
        return Result<cv::Mat>::failure(sourceValidation.error());
    }
    const Result<void> kernelValidation = validateKernelSize(kernelSize);
    if (!kernelValidation.isSuccess()) {
        return Result<cv::Mat>::failure(kernelValidation.error());
    }

    cv::Mat result;
    cv::GaussianBlur(
        source,
        result,
        cv::Size(kernelSize, kernelSize),
        0.0
    );
    return Result<cv::Mat>::success(std::move(result));
}

Result<cv::Mat> ImageProcessor::medianBlur(
    const cv::Mat& source,
    int kernelSize
) const
{
    const Result<void> sourceValidation = validateSource(source);
    if (!sourceValidation.isSuccess()) {
        return Result<cv::Mat>::failure(sourceValidation.error());
    }
    const Result<void> kernelValidation = validateKernelSize(kernelSize);
    if (!kernelValidation.isSuccess()) {
        return Result<cv::Mat>::failure(kernelValidation.error());
    }

    cv::Mat result;
    cv::medianBlur(source, result, kernelSize);
    return Result<cv::Mat>::success(std::move(result));
}

Result<cv::Mat> ImageProcessor::canny(
    const cv::Mat& source,
    int lowThreshold,
    int highThreshold
) const
{
    if (lowThreshold < 0 || highThreshold > 255 ||
        lowThreshold >= highThreshold) {
        return Result<cv::Mat>::failure(
            QStringLiteral("Canny 阈值必须满足 0 <= 低阈值 < 高阈值 <= 255")
        );
    }

    Result<cv::Mat> grayResult = toGrayscale(source);
    if (!grayResult.isSuccess()) {
        return Result<cv::Mat>::failure(grayResult.error());
    }

    cv::Mat result;
    cv::Canny(
        grayResult.value(),
        result,
        lowThreshold,
        highThreshold
    );
    return Result<cv::Mat>::success(std::move(result));
}

Result<cv::Mat> ImageProcessor::adjustBrightness(
    const cv::Mat& source,
    int brightness
) const
{
    const Result<void> validation = validateSource(source);
    if (!validation.isSuccess()) {
        return Result<cv::Mat>::failure(validation.error());
    }
    if (brightness < -100 || brightness > 100) {
        return Result<cv::Mat>::failure(
            QStringLiteral("亮度必须在 -100 到 100 之间")
        );
    }

    cv::Mat result;
    source.convertTo(result, source.type(), 1.0, brightness);
    return Result<cv::Mat>::success(std::move(result));
}

Result<cv::Mat> ImageProcessor::adjustContrast(
    const cv::Mat& source,
    double contrast
) const
{
    const Result<void> validation = validateSource(source);
    if (!validation.isSuccess()) {
        return Result<cv::Mat>::failure(validation.error());
    }
    if (contrast < 0.1 || contrast > 3.0) {
        return Result<cv::Mat>::failure(
            QStringLiteral("对比度必须在 0.1 到 3.0 之间")
        );
    }

    cv::Mat result;
    source.convertTo(result, source.type(), contrast, 0.0);
    return Result<cv::Mat>::success(std::move(result));
}

Result<void> ImageProcessor::validateSource(const cv::Mat& source) const
{
    if (source.empty()) {
        return Result<void>::failure(QStringLiteral("没有可处理的图像"));
    }
    if (source.depth() != CV_8U ||
        (source.channels() != 1 && source.channels() != 3)) {
        return Result<void>::failure(
            QStringLiteral("只支持 8 位灰度图或三通道彩色图")
        );
    }
    return Result<void>::success();
}

Result<void> ImageProcessor::validateKernelSize(int kernelSize) const
{
    if (kernelSize < 3 || kernelSize > 15 || kernelSize % 2 == 0) {
        return Result<void>::failure(
            QStringLiteral("滤波核大小必须是 3 到 15 之间的奇数")
        );
    }
    return Result<void>::success();
}

Result<cv::Mat> ImageProcessor::toGrayscale(const cv::Mat& source) const
{
    const Result<void> validation = validateSource(source);
    if (!validation.isSuccess()) {
        return Result<cv::Mat>::failure(validation.error());
    }

    if (source.channels() == 1) {
        return Result<cv::Mat>::success(source.clone());
    }

    cv::Mat result;
    cv::cvtColor(source, result, cv::COLOR_BGR2GRAY);
    return Result<cv::Mat>::success(std::move(result));
}
