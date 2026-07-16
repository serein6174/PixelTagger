#pragma once

#include <QObject>
#include <QImage>
#include <QString>

#include "processor/ImageProcessor.h"

class ProcessViewModel final : public QObject {
    Q_OBJECT

public:
    explicit ProcessViewModel(ImageProcessor& processor);

    const QImage& sourceImage() const noexcept;
    const QImage& previewImage() const noexcept;
    const QImage& displayImage() const noexcept;
    bool hasSourceImage() const noexcept;
    bool hasPreview() const noexcept;

public slots:
    void setSourceImage(const QImage& image);
    void previewGrayscale();
    void previewBinary(int threshold = 128);
    void previewMeanBlur(int kernelSize = 3);
    void previewGaussianBlur(int kernelSize = 3);
    void previewMedianBlur(int kernelSize = 3);
    void previewCanny(int lowThreshold = 50, int highThreshold = 150);
    void previewBrightness(int brightness);
    void previewContrast(double contrast);
    void resetPreview();
    void savePreview(const QString& path);

signals:
    void previewChanged();
    void previewAvailabilityChanged(bool available);
    void statusChanged(const QString& message);
    void errorOccurred(const QString& message);

private:
    void publishResult(
        Result<cv::Mat> result,
        const QString& operationName
    );

    ImageProcessor& processor_;
    QImage sourceImage_;
    QImage previewImage_;
};
