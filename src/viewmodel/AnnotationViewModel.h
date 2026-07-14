#pragma once

#include <optional>

#include <QObject>
#include <QVector>

#include "model/ProjectModel.h"
#include "common/presentation/AnnotationRenderData.h"
#include "common/types/EntityIds.h"
#include "common/types/ViewModelChange.h"

class AnnotationViewModel : public QObject {
    Q_OBJECT

public:
    explicit AnnotationViewModel(ProjectModel& project);
    QVector<AnnotationRenderData> annotationItems() const;

public slots:
    void createAnnotation(const QRectF& imageRect);
    void onCurrentImageChanged();
    void onLabelsChanged();

signals:
    void changed(ViewModelChange change);
    void errorOccurred(const QString& message);

private:
    void publishAnnotations();
    LabelModel defaultLabel() const;

    ProjectModel& project_;
    std::optional<AnnotationId> selectedAnnotationId_;
};

