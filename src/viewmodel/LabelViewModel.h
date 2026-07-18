#pragma once

#include <QObject>
#include <QColor>
#include <QString>
#include <QVector>

#include "common/presentation/LabelPresentationData.h"
#include "common/types/EntityIds.h"
#include "model/ProjectModel.h"

class LabelViewModel : public QObject {
    Q_OBJECT

public:
    explicit LabelViewModel(ProjectModel& project);

    QVector<LabelPresentationData> labelItems() const;
    LabelId currentLabelId() const noexcept;
    QString currentLabelName() const;

public slots:
    void addLabel(const QString& name, const QColor& color);
    void removeLabel(LabelId labelId);
    void renameLabel(LabelId labelId, const QString& name);
    void setLabelColor(LabelId labelId, const QColor& color);
    void setCurrentLabel(LabelId labelId);
    void setCurrentLabelName(const QString& name);
    void onProjectChanged();

signals:
    void labelsChanged();
    void currentLabelChanged(LabelId labelId);
    void errorOccurred(const QString& message);

private:
    void resetCurrentLabel();
    void publishCurrentLabel();

    ProjectModel& project_;
    LabelId currentLabelId_ = -1;
};
