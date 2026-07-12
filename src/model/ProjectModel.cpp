#include "model/ProjectModel.h"

#include <utility>

ProjectModel::ProjectModel()
{
    labels_.push_back(LabelModel{});
}

bool ProjectModel::hasCurrentImage() const noexcept
{
    return currentIndex_ >= 0 && currentIndex_ < images_.size();
}

const ImageModel* ProjectModel::currentImage() const noexcept
{
    return hasCurrentImage() ? &images_[currentIndex_] : nullptr;
}

ImageModel ProjectModel::currentImageValue() const
{
    return hasCurrentImage() ? images_[currentIndex_] : ImageModel{};
}

int ProjectModel::currentImagePosition() const noexcept
{
    return hasCurrentImage() ? currentIndex_ + 1 : 0;
}

int ProjectModel::imageCount() const noexcept
{
    return images_.size();
}

void ProjectModel::replaceWithSingleImage(ImageModel image)
{
    QVector<ImageModel> images;
    images.push_back(std::move(image));
    replaceImages(std::move(images));
}

void ProjectModel::replaceImages(QVector<ImageModel> images)
{
    assignImageIds(images);
    images_ = std::move(images);
    currentIndex_ = images_.isEmpty() ? -1 : 0;
    modified_ = false;
}

bool ProjectModel::moveNextImage()
{
    if (currentIndex_ + 1 >= images_.size()) {
        return false;
    }

    ++currentIndex_;
    return true;
}

bool ProjectModel::movePreviousImage()
{
    if (currentIndex_ <= 0) {
        return false;
    }

    --currentIndex_;
    return true;
}

bool ProjectModel::addAnnotationToCurrentImage(
    const QRectF& imageRect,
    LabelId labelId
)
{
    if (!hasCurrentImage() || !findLabel(labelId)) {
        return false;
    }

    AnnotationModel annotation;
    annotation.id = nextAnnotationId_++;
    annotation.labelId = labelId;
    annotation.imageRect = imageRect;

    ImageModel& image = images_[currentIndex_];
    image.annotations.push_back(annotation);
    image.modified = true;
    modified_ = true;
    return true;
}

const LabelModel* ProjectModel::defaultLabel() const noexcept
{
    return labels_.isEmpty() ? nullptr : &labels_.front();
}

const LabelModel* ProjectModel::findLabel(LabelId labelId) const noexcept
{
    for (const LabelModel& label : labels_) {
        if (label.id == labelId) {
            return &label;
        }
    }

    return nullptr;
}

bool ProjectModel::renameDefaultLabel(const QString& name)
{
    if (labels_.isEmpty() || labels_.front().name == name) {
        return false;
    }

    labels_.front().name = name;
    modified_ = true;
    return true;
}

bool ProjectModel::isModified() const noexcept
{
    return modified_;
}

void ProjectModel::assignImageIds(QVector<ImageModel>& images)
{
    for (ImageModel& image : images) {
        image.id = nextImageId_++;
    }
}
