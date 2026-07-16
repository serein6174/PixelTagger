#pragma once

#include <QRectF>
#include <QVector>

#include "common/types/Result.h"
#include "model/ImageModel.h"
#include "model/LabelModel.h"

class ProjectModel final {
public:
    ProjectModel();

    bool hasCurrentImage() const noexcept;
    const ImageModel* currentImage() const noexcept;
    ImageModel currentImageValue() const;
    int currentImagePosition() const noexcept;
    int imageCount() const noexcept;
    int currentImageIndex() const noexcept;
    const QVector<ImageModel>& images() const noexcept;
    const QVector<LabelModel>& labels() const noexcept;

    void replaceWithSingleImage(ImageModel image);
    void replaceImages(QVector<ImageModel> images);
    Result<void> replaceProjectData(
        QVector<ImageModel> images,
        QVector<LabelModel> labels,
        int currentIndex
    );
    bool moveNextImage();
    bool movePreviousImage();

    bool addAnnotationToCurrentImage(const QRectF& imageRect, LabelId labelId);

    const LabelModel* defaultLabel() const noexcept;
    const LabelModel* findLabel(LabelId labelId) const noexcept;
    Result<LabelId> addLabel(const QString& name, const QColor& color);
    Result<void> removeLabel(LabelId labelId);
    Result<void> renameLabel(LabelId labelId, const QString& name);
    Result<void> setLabelColor(LabelId labelId, const QColor& color);
    bool isLabelInUse(LabelId labelId) const noexcept;
    bool renameDefaultLabel(const QString& name);

    bool isModified() const noexcept;
    void markSaved() noexcept;

private:
    void assignImageIds(QVector<ImageModel>& images);
    void refreshNextIds();

    QVector<ImageModel> images_;
    QVector<LabelModel> labels_;
    int currentIndex_ = -1;
    bool modified_ = false;
    ImageId nextImageId_ = 1;
    AnnotationId nextAnnotationId_ = 1;
    LabelId nextLabelId_ = 1;
};
