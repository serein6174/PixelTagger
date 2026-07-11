#pragma once

#include <QRectF>

#include "model/Types.h"

class AnnotationModel {
public:
    AnnotationId id = -1;
    LabelId labelId = 0;
    QRectF imageRect;
};
