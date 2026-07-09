#pragma once

#include <QString>

class ImageModel {
public:
    QString filePath;
    QString fileName;
    int width = 0;
    int height = 0;
    bool modified = false;
};
