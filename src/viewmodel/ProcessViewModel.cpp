#include "viewmodel/ProcessViewModel.h"

#include "processor/ImageConverter.h"

ProcessViewModel::ProcessViewModel(ImageProcessor& processor)
    : processor_(processor)
{
}

const QImage& ProcessViewModel::sourceImage() const noexcept
{
    return sourceImage_;
}

const QImage& ProcessViewModel::previewImage() const noexcept
{
    return previewImage_;
}

const QImage& ProcessViewModel::displayImage() const noexcept
{
    return previewImage_.isNull() ? sourceImage_ : previewImage_;
}

bool ProcessViewModel::hasSourceImage() const noexcept
{
    return !sourceImage_.isNull();
}

bool ProcessViewModel::hasPreview() const noexcept
{
    return !previewImage_.isNull();
}

void ProcessViewModel::setSourceImage(const QImage& image)
{
    const bool hadPreview = hasPreview();
    sourceImage_ = image.copy();
    previewImage_ = QImage{};
    if (hadPreview) {
        emit previewAvailabilityChanged(false);
    }
    emit previewChanged();
}

void ProcessViewModel::previewGrayscale()
{
    const Result<cv::Mat> sourceResult = ImageConverter::toMat(sourceImage_);
    if (!sourceResult.isSuccess()) {
        emit errorOccurred(sourceResult.error());
        return;
    }
    publishResult(
        processor_.grayscale(sourceResult.value()),
        QStringLiteral("灰度化")
    );
}

void ProcessViewModel::previewBinary(int threshold)
{
    const Result<cv::Mat> sourceResult = ImageConverter::toMat(sourceImage_);
    if (!sourceResult.isSuccess()) {
        emit errorOccurred(sourceResult.error());
        return;
    }
    publishResult(
        processor_.binary(sourceResult.value(), threshold),
        QStringLiteral("二值化")
    );
}

void ProcessViewModel::previewMeanBlur(int kernelSize)
{
    const Result<cv::Mat> sourceResult = ImageConverter::toMat(sourceImage_);
    if (!sourceResult.isSuccess()) {
        emit errorOccurred(sourceResult.error());
        return;
    }
    publishResult(
        processor_.meanBlur(sourceResult.value(), kernelSize),
        QStringLiteral("均值滤波")
    );
}

void ProcessViewModel::previewGaussianBlur(int kernelSize)
{
    const Result<cv::Mat> sourceResult = ImageConverter::toMat(sourceImage_);
    if (!sourceResult.isSuccess()) {
        emit errorOccurred(sourceResult.error());
        return;
    }
    publishResult(
        processor_.gaussianBlur(sourceResult.value(), kernelSize),
        QStringLiteral("高斯滤波")
    );
}

void ProcessViewModel::previewMedianBlur(int kernelSize)
{
    const Result<cv::Mat> sourceResult = ImageConverter::toMat(sourceImage_);
    if (!sourceResult.isSuccess()) {
        emit errorOccurred(sourceResult.error());
        return;
    }
    publishResult(
        processor_.medianBlur(sourceResult.value(), kernelSize),
        QStringLiteral("中值滤波")
    );
}

void ProcessViewModel::previewCanny(int lowThreshold, int highThreshold)
{
    const Result<cv::Mat> sourceResult = ImageConverter::toMat(sourceImage_);
    if (!sourceResult.isSuccess()) {
        emit errorOccurred(sourceResult.error());
        return;
    }
    publishResult(
        processor_.canny(
            sourceResult.value(),
            lowThreshold,
            highThreshold
        ),
        QStringLiteral("Canny 边缘检测")
    );
}

void ProcessViewModel::previewBrightness(int brightness)
{
    const Result<cv::Mat> sourceResult = ImageConverter::toMat(sourceImage_);
    if (!sourceResult.isSuccess()) {
        emit errorOccurred(sourceResult.error());
        return;
    }
    publishResult(
        processor_.adjustBrightness(sourceResult.value(), brightness),
        QStringLiteral("亮度调整")
    );
}

void ProcessViewModel::previewContrast(double contrast)
{
    const Result<cv::Mat> sourceResult = ImageConverter::toMat(sourceImage_);
    if (!sourceResult.isSuccess()) {
        emit errorOccurred(sourceResult.error());
        return;
    }
    publishResult(
        processor_.adjustContrast(sourceResult.value(), contrast),
        QStringLiteral("对比度调整")
    );
}

void ProcessViewModel::resetPreview()
{
    if (!hasPreview()) {
        return;
    }

    previewImage_ = QImage{};
    emit previewAvailabilityChanged(false);
    emit previewChanged();
    emit statusChanged(QStringLiteral("已恢复原图预览"));
}

void ProcessViewModel::savePreview(const QString& path)
{
    if (!hasPreview()) {
        emit errorOccurred(QStringLiteral("当前没有可保存的处理预览"));
        return;
    }
    if (path.trimmed().isEmpty() || !previewImage_.save(path)) {
        emit errorOccurred(QStringLiteral("无法保存处理结果：%1").arg(path));
        return;
    }
    emit statusChanged(QStringLiteral("处理结果已保存：%1").arg(path));
}

void ProcessViewModel::publishResult(
    Result<cv::Mat> result,
    const QString& operationName
)
{
    if (!result.isSuccess()) {
        emit errorOccurred(result.error());
        return;
    }

    Result<QImage> imageResult = ImageConverter::toQImage(result.value());
    if (!imageResult.isSuccess()) {
        emit errorOccurred(imageResult.error());
        return;
    }

    const bool hadPreview = hasPreview();
    previewImage_ = imageResult.takeValue();
    if (!hadPreview) {
        emit previewAvailabilityChanged(true);
    }
    emit previewChanged();
    emit statusChanged(QStringLiteral("%1预览已更新").arg(operationName));
}
