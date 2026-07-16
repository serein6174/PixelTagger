#pragma once

#include <QColor>
#include <QString>

#include "common/types/EntityIds.h"

struct LabelViewData final {
    LabelId id = -1;
    QString name;
    QColor color;
    bool current = false;
    bool inUse = false;
};
