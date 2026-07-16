#pragma once

#include "model/ImageModel.h"

inline ImageModel makeTestImage(
    const QString& fileName = QStringLiteral("sample.png")
)
{
    ImageModel image;
    image.filePath = QStringLiteral("C:/test/images/") + fileName;
    image.fileName = fileName;
    image.relativePath = QStringLiteral("images/") + fileName;
    image.width = 640;
    image.height = 480;
    return image;
}
