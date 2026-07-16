#include "viewmodel/LabelViewModel.h"

LabelViewModel::LabelViewModel(ProjectModel& project)
    : project_(project)
{
    resetCurrentLabel();
}

QVector<LabelViewData> LabelViewModel::labelItems() const
{
    QVector<LabelViewData> items;
    items.reserve(project_.labels().size());
    for (const LabelModel& label : project_.labels()) {
        LabelViewData item;
        item.id = label.id;
        item.name = label.name;
        item.color = label.color;
        item.current = label.id == currentLabelId_;
        item.inUse = project_.isLabelInUse(label.id);
        items.push_back(item);
    }
    return items;
}

LabelId LabelViewModel::currentLabelId() const noexcept
{
    return currentLabelId_;
}

QString LabelViewModel::currentLabelName() const
{
    const LabelModel* label = project_.findLabel(currentLabelId_);
    return label ? label->name : QStringLiteral("object");
}

void LabelViewModel::addLabel(const QString& name, const QColor& color)
{
    Result<LabelId> result = project_.addLabel(name, color);
    if (!result.isSuccess()) {
        emit errorOccurred(result.error());
        return;
    }

    currentLabelId_ = result.value();
    emit labelsChanged();
    publishCurrentLabel();
}

void LabelViewModel::removeLabel(LabelId labelId)
{
    const bool removesCurrentLabel = labelId == currentLabelId_;
    const Result<void> result = project_.removeLabel(labelId);
    if (!result.isSuccess()) {
        emit errorOccurred(result.error());
        return;
    }

    if (removesCurrentLabel) {
        resetCurrentLabel();
    }
    emit labelsChanged();
    if (removesCurrentLabel) {
        publishCurrentLabel();
    }
}

void LabelViewModel::renameLabel(LabelId labelId, const QString& name)
{
    const Result<void> result = project_.renameLabel(labelId, name);
    if (!result.isSuccess()) {
        emit errorOccurred(result.error());
        return;
    }

    emit labelsChanged();
    emit changed(ViewModelChange::CurrentLabel);
}

void LabelViewModel::setLabelColor(LabelId labelId, const QColor& color)
{
    const Result<void> result = project_.setLabelColor(labelId, color);
    if (!result.isSuccess()) {
        emit errorOccurred(result.error());
        return;
    }

    emit labelsChanged();
    emit changed(ViewModelChange::CurrentLabel);
}

void LabelViewModel::setCurrentLabel(LabelId labelId)
{
    if (!project_.findLabel(labelId)) {
        emit errorOccurred(QStringLiteral("选择的类别不存在"));
        return;
    }
    if (currentLabelId_ == labelId) {
        return;
    }

    currentLabelId_ = labelId;
    emit labelsChanged();
    publishCurrentLabel();
}

void LabelViewModel::setCurrentLabelName(const QString& name)
{
    renameLabel(currentLabelId_, name);
}

void LabelViewModel::onProjectChanged(ViewModelChange change)
{
    if (change == ViewModelChange::Project) {
        resetCurrentLabel();
        emit labelsChanged();
        publishCurrentLabel();
    }
}

void LabelViewModel::resetCurrentLabel()
{
    const LabelModel* label = project_.defaultLabel();
    currentLabelId_ = label ? label->id : -1;
}

void LabelViewModel::publishCurrentLabel()
{
    emit currentLabelChanged(currentLabelId_);
    emit changed(ViewModelChange::CurrentLabel);
}
