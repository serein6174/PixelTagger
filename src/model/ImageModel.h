#pragma once

#include <QString>
#include <QVector>

#include "model/AnnotationModel.h"
#include "model/Types.h"

class ImageModel {
public:
    ImageId id = -1;
    QString filePath;
    QString fileName;
    QString relativePath;
    int width = 0;
    int height = 0;
    bool modified = false;
    QVector<AnnotationModel> annotations;
};
