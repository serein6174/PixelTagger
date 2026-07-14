#include "viewmodel/ImageViewModel.h"

#include <QDir>
#include <QFileInfo>
#include <QImageReader>
#include <QStringList>
#include <utility>

ImageViewModel::ImageViewModel(ProjectModel& project)
    : project_(project)
{
}

void ImageViewModel::loadImage(const QString& path)
{
    Result<QVector<ImageModel>> result = importImage(path);
    if (!result.isSuccess()) {
        emit errorOccurred(result.error());
        return;
    }

    project_.replaceImages(result.takeValue());
    loadCurrentImage();
}

void ImageViewModel::loadFolder(const QString& folderPath)
{
    Result<QVector<ImageModel>> result = importFolder(folderPath);
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

const QImage& ImageViewModel::currentQImage() const noexcept
{
    return currentImage_;
}

Result<QVector<ImageModel>> ImageViewModel::importImage(const QString& path) const
{
    const QFileInfo fileInfo(path);
    Result<ImageModel> result = readImage(path, fileInfo.absolutePath());
    if (!result.isSuccess()) {
        return Result<QVector<ImageModel>>::failure(result.error());
    }

    QVector<ImageModel> images;
    images.push_back(result.takeValue());
    return Result<QVector<ImageModel>>::success(std::move(images));
}

Result<QVector<ImageModel>> ImageViewModel::importFolder(const QString& folderPath) const
{
    QDir directory(folderPath);
    if (!directory.exists()) {
        return Result<QVector<ImageModel>>::failure(
            QStringLiteral("图片文件夹不存在：%1").arg(folderPath)
        );
    }

    const QStringList filters = {
        QStringLiteral("*.png"),
        QStringLiteral("*.jpg"),
        QStringLiteral("*.jpeg"),
        QStringLiteral("*.bmp"),
        QStringLiteral("*.gif")
    };
    const QFileInfoList files = directory.entryInfoList(
        filters,
        QDir::Files | QDir::Readable,
        QDir::Name | QDir::IgnoreCase
    );

    QVector<ImageModel> images;
    images.reserve(files.size());
    for (const QFileInfo& file : files) {
        Result<ImageModel> result = readImage(file.absoluteFilePath(), folderPath);
        if (!result.isSuccess()) {
            return Result<QVector<ImageModel>>::failure(result.error());
        }
        images.push_back(result.takeValue());
    }

    if (images.isEmpty()) {
        return Result<QVector<ImageModel>>::failure(
            QStringLiteral("文件夹中没有可打开的图片：%1").arg(folderPath)
        );
    }

    return Result<QVector<ImageModel>>::success(std::move(images));
}

Result<ImageModel> ImageViewModel::readImage(
    const QString& path,
    const QString& rootPath
) const
{
    QImageReader reader(path);
    const QSize size = reader.size();
    if (!size.isValid() || !reader.canRead()) {
        return Result<ImageModel>::failure(
            QStringLiteral("无法读取图片：%1").arg(path)
        );
    }

    const QFileInfo fileInfo(path);
    ImageModel image;
    image.filePath = fileInfo.absoluteFilePath();
    image.fileName = fileInfo.fileName();
    image.relativePath = QDir(rootPath).relativeFilePath(image.filePath);
    image.width = size.width();
    image.height = size.height();
    image.modified = false;
    return Result<ImageModel>::success(std::move(image));
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
    emit changed(ViewModelChange::CurrentImage);
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
