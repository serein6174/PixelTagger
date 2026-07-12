#pragma once

#include <QVector>

#include "common/types/Result.h"
#include "model/ImageModel.h"

class ImageImportService final {
public:
    Result<QVector<ImageModel>> importImage(const QString& path) const;
    Result<QVector<ImageModel>> importFolder(const QString& folderPath) const;

private:
    Result<ImageModel> readImage(const QString& path, const QString& rootPath) const;
};
