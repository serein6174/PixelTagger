#include "viewmodel/ImageViewModel.h"

#include <QDir>
#include <QFileInfo>
#include <QStringList>

ImageViewModel::ImageViewModel(ProjectModel& project)
    : project_(project)
{
}

void ImageViewModel::loadImage(const QString& path)
{
    QImage image;
    if (!image.load(path)) {
        emit errorOccurred(QStringLiteral("无法打开图片：%1").arg(path));
        return;
    }
    QFileInfo info(path);
    ImageModel model;
    model.id = project_.nextImageId++;
    model.filePath = info.absoluteFilePath();
    model.fileName = info.fileName();
    model.relativePath = info.fileName();
    model.width = image.width();
    model.height = image.height();
    model.modified = false;
    project_.setSingleImage(model);
    currentImage_ = image;
    emit imageChanged(currentImage_);
    emit currentImageChanged();
    emit statusChanged(QStringLiteral("%1  %2x%3  %4")
                           .arg(model.fileName).arg(model.width).arg(model.height)
                           .arg(imagePositionText()));
}

void ImageViewModel::loadFolder(const QString& folderPath)
{
    QDir directory(folderPath);
    const QStringList filters = {
        QStringLiteral("*.png"), QStringLiteral("*.jpg"),
        QStringLiteral("*.jpeg"), QStringLiteral("*.bmp"),
        QStringLiteral("*.gif")
    };
    const QFileInfoList files = directory.entryInfoList(
        filters, QDir::Files | QDir::Readable, QDir::Name | QDir::IgnoreCase);
    if (files.isEmpty()) {
        emit errorOccurred(QStringLiteral("文件夹中没有可打开的图片：%1").arg(folderPath));
        return;
    }
    QVector<ImageModel> imageList;
    imageList.reserve(files.size());
    for (const QFileInfo& file : files) {
        ImageModel model;
        model.id = project_.nextImageId++;
        model.filePath = file.absoluteFilePath();
        model.fileName = file.fileName();
        model.relativePath = file.fileName();
        model.modified = false;
        imageList.push_back(model);
    }
    project_.setImages(imageList);
    loadCurrentImage();
}

void ImageViewModel::nextImage()
{
    if (!project_.moveNext()) {
        emit statusChanged(QStringLiteral("已经是最后一张图片  %1").arg(imagePositionText()));
        return;
    }
    loadCurrentImage();
}

void ImageViewModel::previousImage()
{
    if (!project_.movePrevious()) {
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
    model.width = image.width();
    model.height = image.height();
    project_.images[project_.currentIndex] = model;
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
        .arg(project_.currentIndex + 1).arg(project_.images.size());
}
