#include "viewmodel/AnnotationViewModel.h"

namespace {
constexpr double kMinimumAnnotationSize = 3.0;
}

AnnotationViewModel::AnnotationViewModel(ProjectModel& project)
    : project_(project)
{
}

void AnnotationViewModel::createAnnotation(const QRectF& imageRect)
{
    ImageModel* image = project_.currentImage();
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

    const LabelModel label = defaultLabel();

    AnnotationModel annotation;
    annotation.id = project_.nextAnnotationId++;
    annotation.labelId = label.id;
    annotation.imageRect = boundedRect;

    image->annotations.push_back(annotation);
    image->modified = true;
    project_.modified = true;

    publishAnnotations();
}

void AnnotationViewModel::onCurrentImageChanged()
{
    selectedAnnotationId_.reset();
    publishAnnotations();
}

void AnnotationViewModel::onLabelsChanged()
{
    publishAnnotations();
}

void AnnotationViewModel::publishAnnotations()
{
    QVector<AnnotationViewData> items;

    const ImageModel* image = project_.currentImage();
    if (!image) {
        emit annotationsChanged(items);
        return;
    }

    const LabelModel label = defaultLabel();
    items.reserve(image->annotations.size());

    for (const AnnotationModel& annotation : image->annotations) {
        AnnotationViewData item;
        item.id = annotation.id;
        item.imageRect = annotation.imageRect;
        item.labelName = label.name;
        item.color = label.color;
        item.selected = selectedAnnotationId_.has_value() &&
                        selectedAnnotationId_.value() == annotation.id;
        items.push_back(item);
    }

    emit annotationsChanged(items);
}

LabelModel AnnotationViewModel::defaultLabel() const
{
    return project_.labels.isEmpty() ? LabelModel{} : project_.labels.front();
}

