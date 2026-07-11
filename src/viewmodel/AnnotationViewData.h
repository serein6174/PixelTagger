#pragma once

#include <QColor>
#include <QRectF>
#include <QString>

#include "model/Types.h"

struct AnnotationViewData {
    AnnotationId id = -1;
    QRectF imageRect;
    QString labelName;
    QColor color;
    bool selected = false;
};
