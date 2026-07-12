#pragma once

#include <QObject>
#include <QImage>

#include "model/ImageModel.h"
#include "model/ProjectModel.h"
#include "service/ImageImportService.h"

class ImageViewModel : public QObject {
    Q_OBJECT

public:
    ImageViewModel(ProjectModel& project, const ImageImportService& importService);

    void loadImage(const QString& path);
    void loadFolder(const QString& folderPath);
    void nextImage();
    void previousImage();

    ImageModel currentImage() const;
    QImage currentQImage() const;

signals:
    void imageChanged(const QImage& image);
    void currentImageChanged();
    void statusChanged(const QString& message);
    void errorOccurred(const QString& message);

private:
    bool loadCurrentImage();
    QString imagePositionText() const;

    ProjectModel& project_;
    const ImageImportService& importService_;
    QImage currentImage_;
};
