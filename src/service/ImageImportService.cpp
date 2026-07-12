#include "service/ImageImportService.h"

#include <QDir>
#include <QFileInfo>
#include <QImageReader>
#include <QStringList>

Result<QVector<ImageModel>> ImageImportService::importImage(const QString& path) const
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

Result<QVector<ImageModel>> ImageImportService::importFolder(
    const QString& folderPath
) const
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

Result<ImageModel> ImageImportService::readImage(
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
