#pragma once

#include <QRectF>

#include "common/types/EntityIds.h"

class AnnotationModel {
public:
    AnnotationId id = -1;
    LabelId labelId = 0;
    QRectF imageRect;
};
