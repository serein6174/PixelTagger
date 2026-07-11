#include "viewmodel/LabelViewModel.h"

LabelViewModel::LabelViewModel(ProjectModel& project)
    : project_(project)
{
}

QString LabelViewModel::currentLabelName() const
{
    return project_.labels.isEmpty() ? QStringLiteral("object") : project_.labels.front().name;
}

void LabelViewModel::setCurrentLabelName(const QString& name)
{
    const QString normalizedName = name.trimmed();
    if (normalizedName.isEmpty()) {
        emit errorOccurred(QStringLiteral("类别名称不能为空"));
        emit currentLabelNameChanged(currentLabelName());
        return;
    }

    if (project_.labels.isEmpty()) {
        project_.labels.push_back(LabelModel{});
    }

    if (project_.labels.front().name == normalizedName) {
        return;
    }

    project_.labels.front().name = normalizedName;
    project_.modified = true;

    emit currentLabelNameChanged(normalizedName);
    emit labelsChanged();
}
