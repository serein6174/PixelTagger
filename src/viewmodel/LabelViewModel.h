#pragma once

#include <QObject>
#include <QString>

#include "common/types/ViewModelChange.h"
#include "model/ProjectModel.h"

class LabelViewModel : public QObject {
    Q_OBJECT

public:
    explicit LabelViewModel(ProjectModel& project);

    QString currentLabelName() const;

public slots:
    void setCurrentLabelName(const QString& name);

signals:
    void changed(ViewModelChange change);
    void errorOccurred(const QString& message);

private:
    ProjectModel& project_;
};
