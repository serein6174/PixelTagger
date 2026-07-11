#pragma once

#include <QVector>

#include "model/ImageModel.h"
#include "model/LabelModel.h"

class ProjectModel {
public:
    ProjectModel()
    {
        labels.push_back(LabelModel{});
    }

    QVector<ImageModel> images;
    QVector<LabelModel> labels;
    int currentIndex = -1;
    bool modified = false;
    ImageId nextImageId = 1;
    AnnotationId nextAnnotationId = 1;

    bool hasCurrentImage() const
    {
        return currentIndex >= 0 && currentIndex < images.size();
    }

    ImageModel* currentImage()
    {
        return hasCurrentImage() ? &images[currentIndex] : nullptr;
    }

    const ImageModel* currentImage() const
    {
        return hasCurrentImage() ? &images[currentIndex] : nullptr;
    }

    ImageModel currentImageValue() const
    {
        return hasCurrentImage() ? images[currentIndex] : ImageModel{};
    }

    void setSingleImage(const ImageModel& image)
    {
        images.clear();
        images.push_back(image);
        currentIndex = 0;
        modified = false;
    }

    void setImages(const QVector<ImageModel>& imageList)
    {
        images = imageList;
        currentIndex = images.isEmpty() ? -1 : 0;
        modified = false;
    }

    bool moveNext()
    {
        if (currentIndex + 1 >= images.size()) {
            return false;
        }

        ++currentIndex;
        return true;
    }

    bool movePrevious()
    {
        if (currentIndex <= 0) {
            return false;
        }

        --currentIndex;
        return true;
    }
};
