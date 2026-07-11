#pragma once

#include <QObject>
#include <QImage>
#include <memory>

#include "common/ICommandBase.h"
#include "model/ImageModel.h"
#include "model/ProjectModel.h"

class ImageViewModel : public QObject {
    Q_OBJECT

public:
    explicit ImageViewModel(ProjectModel& project);

    ICommandBase* loadImageCommand() const;
    ICommandBase* loadFolderCommand() const;
    ICommandBase* previousImageCommand() const;
    ICommandBase* nextImageCommand() const;

    // Navigation-oriented name reserved for views that display images as pages.
    ICommandBase* nextPageCommand() const;

    ImageModel currentImage() const;
    QImage currentQImage() const;

signals:
    void imageChanged(const QImage& image);
    void currentImageChanged();
    void statusChanged(const QString& message);
    void errorOccurred(const QString& message);

private:
    void loadImage(const QString& path);
    void loadFolder(const QString& folderPath);
    void nextImage();
    void previousImage();
    bool loadCurrentImage();
    QString imagePositionText() const;

    ProjectModel& project_;
    QImage currentImage_;
    std::unique_ptr<ICommandBase> loadImageCommand_;
    std::unique_ptr<ICommandBase> loadFolderCommand_;
    std::unique_ptr<ICommandBase> previousImageCommand_;
    std::unique_ptr<ICommandBase> nextImageCommand_;
};
