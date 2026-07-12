#pragma once

#include <QColor>
#include <QString>

#include "common/types/EntityIds.h"

class LabelModel {
public:
    LabelId id = 0;
    QString name = QStringLiteral("object");
    QColor color = QColor(220, 70, 70);
};
