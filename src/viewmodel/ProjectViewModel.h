#pragma once

#include <QObject>
#include <QString>

#include "common/types/ViewModelChange.h"
#include "model/ProjectModel.h"
#include "repository/JsonProjectRepository.h"

class ProjectViewModel final : public QObject {
    Q_OBJECT

public:
    ProjectViewModel(ProjectModel& project, JsonProjectRepository& repository);

public slots:
    void saveProject(const QString& path);
    void openProject(const QString& path);

signals:
    void changed(ViewModelChange change);
    void statusChanged(const QString& message);
    void errorOccurred(const QString& message);

private:
    ProjectModel& project_;
    JsonProjectRepository& repository_;
};
