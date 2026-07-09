#pragma once

#include <QVector>

#include "model/ImageModel.h"

class ProjectModel {
public:
    QVector<ImageModel> images;
    int currentIndex = -1;

    bool hasCurrentImage() const
    {
        return currentIndex >= 0 && currentIndex < images.size();
    }

    ImageModel currentImage() const
    {
        return hasCurrentImage() ? images[currentIndex] : ImageModel{};
    }

    void setSingleImage(const ImageModel& image)
    {
        images.clear();
        images.push_back(image);
        currentIndex = 0;
    }

    void setImages(const QVector<ImageModel>& imageList)
    {
        images = imageList;
        currentIndex = images.isEmpty() ? -1 : 0;
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
