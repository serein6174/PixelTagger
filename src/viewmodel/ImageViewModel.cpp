#include "viewmodel/ImageViewModel.h"

ImageViewModel::ImageViewModel(
    ProjectModel& project,
    const ImageImportService& importService
)
    : project_(project), importService_(importService)
{
}

void ImageViewModel::loadImage(const QString& path)
{
    Result<QVector<ImageModel>> result = importService_.importImage(path);
    if (!result.isSuccess()) {
        emit errorOccurred(result.error());
        return;
    }

    project_.replaceImages(result.takeValue());
    loadCurrentImage();
}

void ImageViewModel::loadFolder(const QString& folderPath)
{
    Result<QVector<ImageModel>> result = importService_.importFolder(folderPath);
    if (!result.isSuccess()) {
        emit errorOccurred(result.error());
        return;
    }

    project_.replaceImages(result.takeValue());
    loadCurrentImage();
}

void ImageViewModel::nextImage()
{
    if (!project_.moveNextImage()) {
        emit statusChanged(QStringLiteral("已经是最后一张图片  %1").arg(imagePositionText()));
        return;
    }
    loadCurrentImage();
}

void ImageViewModel::previousImage()
{
    if (!project_.movePreviousImage()) {
        emit statusChanged(QStringLiteral("已经是第一张图片  %1").arg(imagePositionText()));
        return;
    }
    loadCurrentImage();
}

ImageModel ImageViewModel::currentImage() const
{
    return project_.currentImageValue();
}

QImage ImageViewModel::currentQImage() const
{
    return currentImage_;
}

bool ImageViewModel::loadCurrentImage()
{
    if (!project_.hasCurrentImage()) {
        emit errorOccurred(QStringLiteral("当前没有可显示的图片"));
        return false;
    }
    ImageModel model = project_.currentImageValue();
    QImage image;
    if (!image.load(model.filePath)) {
        emit errorOccurred(QStringLiteral("无法打开图片：%1").arg(model.filePath));
        return false;
    }
    currentImage_ = image;
    emit imageChanged(currentImage_);
    emit currentImageChanged();
    emit statusChanged(QStringLiteral("%1  %2x%3  %4")
                           .arg(model.fileName).arg(model.width).arg(model.height)
                           .arg(imagePositionText()));
    return true;
}

QString ImageViewModel::imagePositionText() const
{
    if (!project_.hasCurrentImage()) {
        return QStringLiteral("0/0");
    }

    return QStringLiteral("%1/%2")
        .arg(project_.currentImagePosition())
        .arg(project_.imageCount());
}
