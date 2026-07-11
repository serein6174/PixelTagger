#pragma once

#include <optional>

#include <QObject>
#include <QVector>

#include "model/ProjectModel.h"
#include "model/Types.h"
#include "viewmodel/AnnotationViewData.h"

class AnnotationViewModel : public QObject {
    Q_OBJECT

public:
    explicit AnnotationViewModel(ProjectModel& project);

public slots:
    void createAnnotation(const QRectF& imageRect);
    void onCurrentImageChanged();
    void onLabelsChanged();

signals:
    void annotationsChanged(const QVector<AnnotationViewData>& annotations);
    void errorOccurred(const QString& message);

private:
    void publishAnnotations();
    LabelModel defaultLabel() const;

    ProjectModel& project_;
    std::optional<AnnotationId> selectedAnnotationId_;
};

