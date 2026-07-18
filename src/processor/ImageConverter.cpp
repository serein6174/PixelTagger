#include "processor/ImageConverter.h"

#include <utility>

#include <opencv2/imgproc.hpp>

Result<cv::Mat> ImageConverter::toMat(const QImage& image)
{
    if (image.isNull()) {
        return Result<cv::Mat>::failure(QStringLiteral("没有可处理的原图"));
    }

    if (image.format() == QImage::Format_Grayscale8) {
        const cv::Mat view(
            image.height(),
            image.width(),
            CV_8UC1,
            const_cast<uchar*>(image.constBits()),
            image.bytesPerLine()
        );
        return Result<cv::Mat>::success(view.clone());
    }

    const QImage rgbImage = image.convertToFormat(QImage::Format_RGB888);
    const cv::Mat rgbView(
        rgbImage.height(),
        rgbImage.width(),
        CV_8UC3,
        const_cast<uchar*>(rgbImage.constBits()),
        rgbImage.bytesPerLine()
    );

    cv::Mat bgrImage;
    cv::cvtColor(rgbView, bgrImage, cv::COLOR_RGB2BGR);
    return Result<cv::Mat>::success(std::move(bgrImage));
}

Result<QImage> ImageConverter::toQImage(const cv::Mat& image)
{
    if (image.empty() || image.depth() != CV_8U) {
        return Result<QImage>::failure(QStringLiteral("OpenCV 处理结果格式无效"));
    }

    if (image.channels() == 1) {
        return Result<QImage>::success(
            QImage(
                image.data,
                image.cols,
                image.rows,
                static_cast<int>(image.step),
                QImage::Format_Grayscale8
            ).copy()
        );
    }

    if (image.channels() == 3) {
        cv::Mat rgbImage;
        cv::cvtColor(image, rgbImage, cv::COLOR_BGR2RGB);
        return Result<QImage>::success(
            QImage(
                rgbImage.data,
                rgbImage.cols,
                rgbImage.rows,
                static_cast<int>(rgbImage.step),
                QImage::Format_RGB888
            ).copy()
        );
    }

    return Result<QImage>::failure(QStringLiteral("不支持的 OpenCV 图像通道数"));
}
