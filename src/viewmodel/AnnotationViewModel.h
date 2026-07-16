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
    LabelId currentLabelId() const noexcept;
    std::optional<AnnotationId> selectedAnnotationId() const noexcept;

public slots:
    void createAnnotation(const QRectF& imageRect);
    void selectAnnotation(AnnotationId annotationId);
    void clearSelection();
    void deleteSelectedAnnotation();
    void updateSelectedAnnotationRect(const QRectF& imageRect);
    void setSelectedAnnotationLabel(LabelId labelId);
    void setCurrentLabelId(LabelId labelId);
    void onImageViewModelChanged(ViewModelChange change);
    void onLabelViewModelChanged(ViewModelChange change);

signals:
    void changed(ViewModelChange change);
    void selectionChanged(bool hasSelection);
    void errorOccurred(const QString& message);

private:
    void publishAnnotations();
    void resetCurrentLabel();

    ProjectModel& project_;
    LabelId currentLabelId_ = -1;
    std::optional<AnnotationId> selectedAnnotationId_;
};

