#include "viewmodel/ProjectViewModel.h"

ProjectViewModel::ProjectViewModel(
    ProjectModel& project,
    JsonProjectRepository& repository,
    YoloExporter& yoloExporter
)
    : project_(project),
      repository_(repository),
      yoloExporter_(yoloExporter)
{
}

void ProjectViewModel::exportYolo(const QString& outputDirectory)
{
    const Result<void> result = yoloExporter_.exportDataset(
        project_,
        outputDirectory
    );
    if (!result.isSuccess()) {
        emit errorOccurred(result.error());
        return;
    }

    emit statusChanged(
        QStringLiteral("YOLO 数据集已导出：%1").arg(outputDirectory)
    );
}

void ProjectViewModel::saveProject(const QString& path)
{
    const Result<void> result = repository_.save(project_, path);
    if (!result.isSuccess()) {
        emit errorOccurred(result.error());
        return;
    }

    project_.markSaved();
    emit statusChanged(QStringLiteral("项目已保存：%1").arg(path));
}

void ProjectViewModel::openProject(const QString& path)
{
    Result<ProjectModel> result = repository_.load(path);
    if (!result.isSuccess()) {
        emit errorOccurred(result.error());
        return;
    }

    project_ = result.takeValue();
    emit projectChanged();
    emit statusChanged(QStringLiteral("项目已打开：%1").arg(path));
}
