#pragma once

#include <QColor>
#include <QString>

#include "common/types/EntityIds.h"

struct LabelPresentationData final {
    LabelId id = -1;
    QString name;
    QColor color;
    bool current = false;
    bool inUse = false;
};
