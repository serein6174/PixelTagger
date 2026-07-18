#pragma once

#include <QColor>
#include <QRectF>
#include <QString>

#include "common/types/EntityIds.h"

struct AnnotationRenderItem final {
    AnnotationId id = -1;
    QRectF imageRect;
    QString labelName;
    QColor color;
    bool selected = false;
};
