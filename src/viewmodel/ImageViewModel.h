#pragma once

#include <QObject>
#include <QImage>
#include <QVector>

#include "common/types/Result.h"
#include "model/ImageModel.h"
#include "model/ProjectModel.h"

class ImageViewModel : public QObject {
    Q_OBJECT

public:
    explicit ImageViewModel(ProjectModel& project);

    void loadImage(const QString& path);
    void loadFolder(const QString& folderPath);
    void nextImage();
    void previousImage();
    void onProjectChanged();

    ImageModel currentImage() const;
    const QImage& currentQImage() const noexcept;

signals:
    void currentImageChanged();
    void statusChanged(const QString& message);
    void errorOccurred(const QString& message);

private:
    Result<QVector<ImageModel>> importImage(const QString& path) const;
    Result<QVector<ImageModel>> importFolder(const QString& folderPath) const;
    Result<ImageModel> readImage(const QString& path, const QString& rootPath) const;
    bool loadCurrentImage();
    QString imagePositionText() const;

    ProjectModel& project_;
    QImage currentImage_;
};
