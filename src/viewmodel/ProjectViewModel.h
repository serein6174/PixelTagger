#pragma once

#include <QObject>
#include <QString>

#include "common/types/ViewModelChange.h"
#include "exporter/YoloExporter.h"
#include "model/ProjectModel.h"
#include "repository/JsonProjectRepository.h"

class ProjectViewModel final : public QObject {
    Q_OBJECT

public:
    ProjectViewModel(
        ProjectModel& project,
        JsonProjectRepository& repository,
        YoloExporter& yoloExporter
    );

public slots:
    void saveProject(const QString& path);
    void openProject(const QString& path);
    void exportYolo(const QString& outputDirectory);

signals:
    void changed(ViewModelChange change);
    void statusChanged(const QString& message);
    void errorOccurred(const QString& message);

private:
    ProjectModel& project_;
    JsonProjectRepository& repository_;
    YoloExporter& yoloExporter_;
};
