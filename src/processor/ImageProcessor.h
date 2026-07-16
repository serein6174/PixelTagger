#pragma once

#include <opencv2/core/mat.hpp>

#include "common/types/Result.h"

class ImageProcessor final {
public:
    Result<cv::Mat> grayscale(const cv::Mat& source) const;
    Result<cv::Mat> binary(const cv::Mat& source, int threshold) const;
    Result<cv::Mat> meanBlur(const cv::Mat& source, int kernelSize) const;
    Result<cv::Mat> gaussianBlur(const cv::Mat& source, int kernelSize) const;
    Result<cv::Mat> medianBlur(const cv::Mat& source, int kernelSize) const;
    Result<cv::Mat> canny(
        const cv::Mat& source,
        int lowThreshold,
        int highThreshold
    ) const;
    Result<cv::Mat> adjustBrightness(
        const cv::Mat& source,
        int brightness
    ) const;
    Result<cv::Mat> adjustContrast(
        const cv::Mat& source,
        double contrast
    ) const;

private:
    Result<void> validateSource(const cv::Mat& source) const;
    Result<void> validateKernelSize(int kernelSize) const;
    Result<cv::Mat> toGrayscale(const cv::Mat& source) const;
};
