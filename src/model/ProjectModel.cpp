#include "model/ProjectModel.h"

#include <algorithm>
#include <cmath>
#include <utility>

#include <QSet>

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

int ProjectModel::currentImageIndex() const noexcept
{
    return currentIndex_;
}

const QVector<ImageModel>& ProjectModel::images() const noexcept
{
    return images_;
}

const QVector<LabelModel>& ProjectModel::labels() const noexcept
{
    return labels_;
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

Result<void> ProjectModel::replaceProjectData(
    QVector<ImageModel> images,
    QVector<LabelModel> labels,
    int currentIndex
)
{
    if (labels.isEmpty()) {
        labels.push_back(LabelModel{});
    }

    QSet<LabelId> labelIds;
    QSet<QString> labelNames;
    for (LabelModel& label : labels) {
        const QString normalizedName = label.name.trimmed();
        const QString comparableName = normalizedName.toCaseFolded();
        if (label.id < 0 || labelIds.contains(label.id) || normalizedName.isEmpty() ||
            labelNames.contains(comparableName) || !label.color.isValid()) {
            return Result<void>::failure(QStringLiteral("项目中的类别 ID 或名称无效"));
        }
        label.name = normalizedName;
        labelIds.insert(label.id);
        labelNames.insert(comparableName);
    }

    QSet<ImageId> imageIds;
    QSet<AnnotationId> annotationIds;
    for (const ImageModel& image : images) {
        if (image.id <= 0 || imageIds.contains(image.id) ||
            image.filePath.isEmpty() || image.fileName.isEmpty() ||
            image.width <= 0 || image.height <= 0) {
            return Result<void>::failure(QStringLiteral("项目中的图片信息无效"));
        }
        imageIds.insert(image.id);

        const QRectF imageBounds(0.0, 0.0, image.width, image.height);
        for (const AnnotationModel& annotation : image.annotations) {
            const QRectF& rect = annotation.imageRect;
            const bool finite = std::isfinite(rect.x()) && std::isfinite(rect.y()) &&
                                std::isfinite(rect.width()) && std::isfinite(rect.height());
            if (annotation.id <= 0 || annotationIds.contains(annotation.id) ||
                !labelIds.contains(annotation.labelId) || !finite ||
                rect.width() <= 0.0 || rect.height() <= 0.0 ||
                rect != rect.normalized() || !imageBounds.contains(rect)) {
                return Result<void>::failure(
                    QStringLiteral("项目中的标注信息或类别引用无效")
                );
            }
            annotationIds.insert(annotation.id);
        }
    }

    if (!images.isEmpty() && (currentIndex < 0 || currentIndex >= images.size())) {
        currentIndex = 0;
    }

    images_ = std::move(images);
    labels_ = std::move(labels);
    currentIndex_ = images_.isEmpty() ? -1 : currentIndex;
    modified_ = false;
    refreshNextIds();
    return Result<void>::success();
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
    if (!findLabel(labelId) || !validateAnnotationRect(imageRect).isSuccess()) {
        return false;
    }

    AnnotationModel annotation;
    annotation.id = nextAnnotationId_++;
    annotation.labelId = labelId;
    annotation.imageRect = imageRect;

    images_[currentIndex_].annotations.push_back(annotation);
    markCurrentImageModified();
    return true;
}

const AnnotationModel* ProjectModel::findAnnotationInCurrentImage(
    AnnotationId annotationId
) const noexcept
{
    const ImageModel* image = currentImage();
    if (!image) {
        return nullptr;
    }

    for (const AnnotationModel& annotation : image->annotations) {
        if (annotation.id == annotationId) {
            return &annotation;
        }
    }
    return nullptr;
}

Result<void> ProjectModel::removeAnnotationFromCurrentImage(
    AnnotationId annotationId
)
{
    if (!hasCurrentImage()) {
        return Result<void>::failure(QStringLiteral("当前没有可编辑的图片"));
    }

    QVector<AnnotationModel>& annotations = images_[currentIndex_].annotations;
    const auto iterator = std::find_if(
        annotations.begin(), annotations.end(),
        [annotationId](const AnnotationModel& annotation) {
            return annotation.id == annotationId;
        }
    );
    if (iterator == annotations.end()) {
        return Result<void>::failure(QStringLiteral("标注不存在"));
    }

    annotations.erase(iterator);
    markCurrentImageModified();
    return Result<void>::success();
}

Result<void> ProjectModel::updateAnnotationRect(
    AnnotationId annotationId,
    const QRectF& imageRect
)
{
    const Result<void> validation = validateAnnotationRect(imageRect);
    if (!validation.isSuccess()) {
        return validation;
    }

    AnnotationModel* annotation = findMutableAnnotationInCurrentImage(annotationId);
    if (!annotation) {
        return Result<void>::failure(QStringLiteral("标注不存在"));
    }
    if (annotation->imageRect == imageRect) {
        return Result<void>::success();
    }

    annotation->imageRect = imageRect;
    markCurrentImageModified();
    return Result<void>::success();
}

Result<void> ProjectModel::changeAnnotationLabel(
    AnnotationId annotationId,
    LabelId labelId
)
{
    if (!findLabel(labelId)) {
        return Result<void>::failure(QStringLiteral("目标类别不存在"));
    }

    AnnotationModel* annotation = findMutableAnnotationInCurrentImage(annotationId);
    if (!annotation) {
        return Result<void>::failure(QStringLiteral("标注不存在"));
    }
    if (annotation->labelId == labelId) {
        return Result<void>::success();
    }

    annotation->labelId = labelId;
    markCurrentImageModified();
    return Result<void>::success();
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

Result<LabelId> ProjectModel::addLabel(const QString& name, const QColor& color)
{
    const QString normalizedName = name.trimmed();
    if (normalizedName.isEmpty()) {
        return Result<LabelId>::failure(QStringLiteral("类别名称不能为空"));
    }
    if (!color.isValid()) {
        return Result<LabelId>::failure(QStringLiteral("类别颜色无效"));
    }

    for (const LabelModel& label : labels_) {
        if (label.name.compare(normalizedName, Qt::CaseInsensitive) == 0) {
            return Result<LabelId>::failure(QStringLiteral("类别名称已存在"));
        }
    }

    LabelModel label;
    label.id = nextLabelId_++;
    label.name = normalizedName;
    label.color = color;
    labels_.push_back(label);
    modified_ = true;
    return Result<LabelId>::success(label.id);
}

Result<void> ProjectModel::removeLabel(LabelId labelId)
{
    if (labels_.size() <= 1) {
        return Result<void>::failure(QStringLiteral("项目必须至少保留一个类别"));
    }
    if (isLabelInUse(labelId)) {
        return Result<void>::failure(QStringLiteral("该类别仍被标注使用，无法删除"));
    }

    const auto iterator = std::find_if(
        labels_.begin(), labels_.end(),
        [labelId](const LabelModel& label) { return label.id == labelId; }
    );
    if (iterator == labels_.end()) {
        return Result<void>::failure(QStringLiteral("类别不存在"));
    }

    labels_.erase(iterator);
    modified_ = true;
    return Result<void>::success();
}

Result<void> ProjectModel::renameLabel(LabelId labelId, const QString& name)
{
    const QString normalizedName = name.trimmed();
    if (normalizedName.isEmpty()) {
        return Result<void>::failure(QStringLiteral("类别名称不能为空"));
    }

    LabelModel* target = nullptr;
    for (LabelModel& label : labels_) {
        if (label.id == labelId) {
            target = &label;
        } else if (label.name.compare(normalizedName, Qt::CaseInsensitive) == 0) {
            return Result<void>::failure(QStringLiteral("类别名称已存在"));
        }
    }
    if (!target) {
        return Result<void>::failure(QStringLiteral("类别不存在"));
    }
    if (target->name == normalizedName) {
        return Result<void>::success();
    }

    target->name = normalizedName;
    modified_ = true;
    return Result<void>::success();
}

Result<void> ProjectModel::setLabelColor(LabelId labelId, const QColor& color)
{
    if (!color.isValid()) {
        return Result<void>::failure(QStringLiteral("类别颜色无效"));
    }

    for (LabelModel& label : labels_) {
        if (label.id != labelId) {
            continue;
        }
        if (label.color != color) {
            label.color = color;
            modified_ = true;
        }
        return Result<void>::success();
    }
    return Result<void>::failure(QStringLiteral("类别不存在"));
}

bool ProjectModel::isLabelInUse(LabelId labelId) const noexcept
{
    for (const ImageModel& image : images_) {
        for (const AnnotationModel& annotation : image.annotations) {
            if (annotation.labelId == labelId) {
                return true;
            }
        }
    }
    return false;
}

bool ProjectModel::renameDefaultLabel(const QString& name)
{
    if (labels_.isEmpty() || labels_.front().name == name.trimmed()) {
        return false;
    }
    return renameLabel(labels_.front().id, name).isSuccess();
}

bool ProjectModel::isModified() const noexcept
{
    return modified_;
}

AnnotationModel* ProjectModel::findMutableAnnotationInCurrentImage(
    AnnotationId annotationId
) noexcept
{
    if (!hasCurrentImage()) {
        return nullptr;
    }

    for (AnnotationModel& annotation : images_[currentIndex_].annotations) {
        if (annotation.id == annotationId) {
            return &annotation;
        }
    }
    return nullptr;
}

Result<void> ProjectModel::validateAnnotationRect(const QRectF& imageRect) const
{
    const ImageModel* image = currentImage();
    if (!image) {
        return Result<void>::failure(QStringLiteral("当前没有可编辑的图片"));
    }

    const bool finite = std::isfinite(imageRect.x()) &&
                        std::isfinite(imageRect.y()) &&
                        std::isfinite(imageRect.width()) &&
                        std::isfinite(imageRect.height());
    const QRectF imageBounds(0.0, 0.0, image->width, image->height);
    if (!finite || imageRect != imageRect.normalized() ||
        imageRect.width() <= 0.0 || imageRect.height() <= 0.0 ||
        !imageBounds.contains(imageRect)) {
        return Result<void>::failure(QStringLiteral("标注矩形无效或超出图片范围"));
    }
    return Result<void>::success();
}

void ProjectModel::markCurrentImageModified()
{
    if (hasCurrentImage()) {
        images_[currentIndex_].modified = true;
        modified_ = true;
    }
}

void ProjectModel::markSaved() noexcept
{
    modified_ = false;
    for (ImageModel& image : images_) {
        image.modified = false;
    }
}

void ProjectModel::assignImageIds(QVector<ImageModel>& images)
{
    for (ImageModel& image : images) {
        image.id = nextImageId_++;
    }
}

void ProjectModel::refreshNextIds()
{
    ImageId maxImageId = 0;
    AnnotationId maxAnnotationId = 0;
    LabelId maxLabelId = -1;

    for (const LabelModel& label : labels_) {
        maxLabelId = std::max(maxLabelId, label.id);
    }

    for (const ImageModel& image : images_) {
        maxImageId = std::max(maxImageId, image.id);
        for (const AnnotationModel& annotation : image.annotations) {
            maxAnnotationId = std::max(maxAnnotationId, annotation.id);
        }
    }

    nextImageId_ = maxImageId + 1;
    nextAnnotationId_ = maxAnnotationId + 1;
    nextLabelId_ = maxLabelId + 1;
}
