#include "viewmodel/AnnotationViewModel.h"

namespace {
constexpr double kMinimumAnnotationSize = 3.0;
}

AnnotationViewModel::AnnotationViewModel(ProjectModel& project)
    : project_(project)
{
    resetCurrentLabel();
}

void AnnotationViewModel::createAnnotation(const QRectF& imageRect)
{
    const ImageModel* image = project_.currentImage();
    if (!image) {
        emit errorOccurred(QStringLiteral("当前没有可标注的图片"));
        return;
    }

    QRectF boundedRect = imageRect.normalized().intersected(
        QRectF(0.0, 0.0, image->width, image->height)
    );

    if (boundedRect.width() < kMinimumAnnotationSize ||
        boundedRect.height() < kMinimumAnnotationSize) {
        return;
    }

    const LabelModel* label = project_.findLabel(currentLabelId_);
    if (!label) {
        emit errorOccurred(QStringLiteral("当前没有有效类别"));
        return;
    }

    if (!project_.addAnnotationToCurrentImage(boundedRect, label->id)) {
        emit errorOccurred(QStringLiteral("创建标注失败"));
        return;
    }

    publishAnnotations();
}

void AnnotationViewModel::onImageViewModelChanged(ViewModelChange change)
{
    if (change != ViewModelChange::CurrentImage) {
        return;
    }
    selectedAnnotationId_.reset();
    publishAnnotations();
}

LabelId AnnotationViewModel::currentLabelId() const noexcept
{
    return currentLabelId_;
}

void AnnotationViewModel::setCurrentLabelId(LabelId labelId)
{
    if (!project_.findLabel(labelId)) {
        emit errorOccurred(QStringLiteral("选择的类别不存在"));
        return;
    }
    currentLabelId_ = labelId;
}

void AnnotationViewModel::onLabelViewModelChanged(ViewModelChange change)
{
    if (change != ViewModelChange::CurrentLabel) {
        return;
    }
    if (!project_.findLabel(currentLabelId_)) {
        resetCurrentLabel();
    }
    publishAnnotations();
}

void AnnotationViewModel::publishAnnotations()
{
    emit changed(ViewModelChange::Annotations);
}

QVector<AnnotationRenderData> AnnotationViewModel::annotationItems() const
{
    QVector<AnnotationRenderData> items;

    const ImageModel* image = project_.currentImage();
    if (!image) {
        return items;
    }

    items.reserve(image->annotations.size());

    for (const AnnotationModel& annotation : image->annotations) {
        const LabelModel* label = project_.findLabel(annotation.labelId);
        if (!label) {
            continue;
        }

        AnnotationRenderData item;
        item.id = annotation.id;
        item.imageRect = annotation.imageRect;
        item.labelName = label->name;
        item.color = label->color;
        item.selected = selectedAnnotationId_.has_value() &&
                        selectedAnnotationId_.value() == annotation.id;
        items.push_back(item);
    }

    return items;
}

void AnnotationViewModel::resetCurrentLabel()
{
    const LabelModel* label = project_.defaultLabel();
    currentLabelId_ = label ? label->id : -1;
}

