#pragma once

#include <QString>

#include "common/types/Result.h"
#include "model/ProjectModel.h"

class YoloExporter final {
public:
    Result<void> exportDataset(
        const ProjectModel& project,
        const QString& outputDirectory
    ) const;
};
