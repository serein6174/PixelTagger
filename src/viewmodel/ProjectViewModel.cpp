#include "viewmodel/ProjectViewModel.h"

ProjectViewModel::ProjectViewModel(
    ProjectModel& project,
    JsonProjectRepository& repository
)
    : project_(project), repository_(repository)
{
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
    emit changed(ViewModelChange::Project);
    emit statusChanged(QStringLiteral("项目已打开：%1").arg(path));
}
