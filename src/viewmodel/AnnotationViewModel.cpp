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

void AnnotationViewModel::selectAnnotation(AnnotationId annotationId)
{
    if (!project_.findAnnotationInCurrentImage(annotationId)) {
        emit errorOccurred(QStringLiteral("选择的标注不存在"));
        return;
    }
    if (selectedAnnotationId_ == annotationId) {
        return;
    }

    selectedAnnotationId_ = annotationId;
    emit selectionChanged(true);
    publishAnnotations();
}

void AnnotationViewModel::clearSelection()
{
    if (!selectedAnnotationId_.has_value()) {
        return;
    }

    selectedAnnotationId_.reset();
    emit selectionChanged(false);
    publishAnnotations();
}

void AnnotationViewModel::deleteSelectedAnnotation()
{
    if (!selectedAnnotationId_.has_value()) {
        emit errorOccurred(QStringLiteral("请先选择一个标注"));
        return;
    }

    const Result<void> result = project_.removeAnnotationFromCurrentImage(
        selectedAnnotationId_.value()
    );
    if (!result.isSuccess()) {
        emit errorOccurred(result.error());
        return;
    }

    selectedAnnotationId_.reset();
    emit selectionChanged(false);
    publishAnnotations();
}

void AnnotationViewModel::updateSelectedAnnotationRect(const QRectF& imageRect)
{
    if (!selectedAnnotationId_.has_value()) {
        emit errorOccurred(QStringLiteral("请先选择一个标注"));
        return;
    }

    const QRectF normalizedRect = imageRect.normalized();
    if (normalizedRect.width() < kMinimumAnnotationSize ||
        normalizedRect.height() < kMinimumAnnotationSize) {
        emit errorOccurred(QStringLiteral("标注框不能小于 3 x 3 原图像素"));
        return;
    }

    const Result<void> result = project_.updateAnnotationRect(
        selectedAnnotationId_.value(),
        normalizedRect
    );
    if (!result.isSuccess()) {
        emit errorOccurred(result.error());
        return;
    }
    publishAnnotations();
}

void AnnotationViewModel::setSelectedAnnotationLabel(LabelId labelId)
{
    if (!selectedAnnotationId_.has_value()) {
        emit errorOccurred(QStringLiteral("请先选择一个标注"));
        return;
    }

    const Result<void> result = project_.changeAnnotationLabel(
        selectedAnnotationId_.value(),
        labelId
    );
    if (!result.isSuccess()) {
        emit errorOccurred(result.error());
        return;
    }
    publishAnnotations();
}

void AnnotationViewModel::onImageViewModelChanged(ViewModelChange change)
{
    if (change != ViewModelChange::CurrentImage) {
        return;
    }
    const bool hadSelection = selectedAnnotationId_.has_value();
    selectedAnnotationId_.reset();
    if (hadSelection) {
        emit selectionChanged(false);
    }
    publishAnnotations();
}

LabelId AnnotationViewModel::currentLabelId() const noexcept
{
    return currentLabelId_;
}

std::optional<AnnotationId> AnnotationViewModel::selectedAnnotationId() const noexcept
{
    return selectedAnnotationId_;
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

