#pragma once

#include <QImage>

#include <opencv2/core/mat.hpp>

#include "common/types/Result.h"

class ImageConverter final {
public:
    static Result<cv::Mat> toMat(const QImage& image);
    static Result<QImage> toQImage(const cv::Mat& image);
};
