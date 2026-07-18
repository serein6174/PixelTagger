#pragma once

#include <QString>

#include "common/types/Result.h"
#include "model/ProjectModel.h"

class JsonProjectRepository final {
public:
    Result<void> save(const ProjectModel& project, const QString& path) const;
    Result<ProjectModel> load(const QString& path) const;
};
