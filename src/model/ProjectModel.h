#pragma once

#include <QRectF>
#include <QVector>

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

    void replaceWithSingleImage(ImageModel image);
    void replaceImages(QVector<ImageModel> images);
    bool moveNextImage();
    bool movePreviousImage();

    bool addAnnotationToCurrentImage(const QRectF& imageRect, LabelId labelId);

    const LabelModel* defaultLabel() const noexcept;
    const LabelModel* findLabel(LabelId labelId) const noexcept;
    bool renameDefaultLabel(const QString& name);

    bool isModified() const noexcept;

private:
    void assignImageIds(QVector<ImageModel>& images);

    QVector<ImageModel> images_;
    QVector<LabelModel> labels_;
    int currentIndex_ = -1;
    bool modified_ = false;
    ImageId nextImageId_ = 1;
    AnnotationId nextAnnotationId_ = 1;
};
