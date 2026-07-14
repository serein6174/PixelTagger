#include "viewmodel/LabelViewModel.h"

LabelViewModel::LabelViewModel(ProjectModel& project)
    : project_(project)
{
}

QString LabelViewModel::currentLabelName() const
{
    const LabelModel* label = project_.defaultLabel();
    return label ? label->name : QStringLiteral("object");
}

void LabelViewModel::setCurrentLabelName(const QString& name)
{
    const QString normalizedName = name.trimmed();
    if (normalizedName.isEmpty()) {
        emit errorOccurred(QStringLiteral("类别名称不能为空"));
        emit changed(ViewModelChange::CurrentLabel);
        return;
    }

    if (!project_.renameDefaultLabel(normalizedName)) {
        return;
    }

    emit changed(ViewModelChange::CurrentLabel);
}
