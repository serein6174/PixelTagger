#include "app/Application.h"

#include <QObject>

Application::Application()
    : projectModel_{},
      projectRepository_{},
      yoloExporter_{},
      imageViewModel_(projectModel_),
      annotationViewModel_(projectModel_),
      labelViewModel_(projectModel_),
      projectViewModel_(projectModel_, projectRepository_, yoloExporter_),
      mainWindow_{}
{
    bindView();
    syncLabelsToView();
}

void Application::show()
{
    mainWindow_.resize(1100, 720);
    mainWindow_.show();
}

void Application::bindView()
{
    ImageCanvas& canvas = mainWindow_.imageCanvas();

    QObject::connect(&mainWindow_, &MainWindow::importImageRequested,
                     &imageViewModel_, &ImageViewModel::loadImage);
    QObject::connect(&mainWindow_, &MainWindow::importFolderRequested,
                     &imageViewModel_, &ImageViewModel::loadFolder);
    QObject::connect(&mainWindow_, &MainWindow::previousImageRequested,
                     &imageViewModel_, &ImageViewModel::previousImage);
    QObject::connect(&mainWindow_, &MainWindow::nextImageRequested,
                     &imageViewModel_, &ImageViewModel::nextImage);
    QObject::connect(&mainWindow_, &MainWindow::addLabelRequested,
                     &labelViewModel_, &LabelViewModel::addLabel);
    QObject::connect(&mainWindow_, &MainWindow::removeLabelRequested,
                     &labelViewModel_, &LabelViewModel::removeLabel);
    QObject::connect(&mainWindow_, &MainWindow::renameLabelRequested,
                     &labelViewModel_, &LabelViewModel::renameLabel);
    QObject::connect(&mainWindow_, &MainWindow::labelColorChangeRequested,
                     &labelViewModel_, &LabelViewModel::setLabelColor);
    QObject::connect(&mainWindow_, &MainWindow::currentLabelChangeRequested,
                     &labelViewModel_, &LabelViewModel::setCurrentLabel);
    QObject::connect(&mainWindow_, &MainWindow::openProjectRequested,
                     &projectViewModel_, &ProjectViewModel::openProject);
    QObject::connect(&mainWindow_, &MainWindow::saveProjectRequested,
                     &projectViewModel_, &ProjectViewModel::saveProject);

    QObject::connect(&canvas, &ImageCanvas::annotationCreateRequested,
                     &annotationViewModel_, &AnnotationViewModel::createAnnotation);

    QObject::connect(&imageViewModel_, &ImageViewModel::changed,
                     &mainWindow_, [this, &canvas](ViewModelChange change) {
                         if (change == ViewModelChange::CurrentImage) {
                             canvas.setImage(imageViewModel_.currentQImage());
                         }
                     });
    QObject::connect(&imageViewModel_, &ImageViewModel::changed,
                     &annotationViewModel_, &AnnotationViewModel::onImageViewModelChanged);
    QObject::connect(&imageViewModel_, &ImageViewModel::statusChanged,
                     &mainWindow_, &MainWindow::showStatus);
    QObject::connect(&imageViewModel_, &ImageViewModel::errorOccurred,
                     &mainWindow_, &MainWindow::showError);
    QObject::connect(&annotationViewModel_, &AnnotationViewModel::changed,
                     &mainWindow_, [this, &canvas](ViewModelChange change) {
                         if (change == ViewModelChange::Annotations) {
                             canvas.setAnnotations(annotationViewModel_.annotationItems());
                         }
                     });
    QObject::connect(&annotationViewModel_, &AnnotationViewModel::errorOccurred,
                     &mainWindow_, &MainWindow::showError);
    QObject::connect(&labelViewModel_, &LabelViewModel::labelsChanged,
                     &mainWindow_, [this]() { syncLabelsToView(); });
    QObject::connect(&labelViewModel_, &LabelViewModel::currentLabelChanged,
                     &annotationViewModel_, &AnnotationViewModel::setCurrentLabelId);
    QObject::connect(&labelViewModel_, &LabelViewModel::changed,
                     &annotationViewModel_, &AnnotationViewModel::onLabelViewModelChanged);
    QObject::connect(&labelViewModel_, &LabelViewModel::errorOccurred,
                     &mainWindow_, &MainWindow::showError);
    QObject::connect(&projectViewModel_, &ProjectViewModel::changed,
                     &imageViewModel_, &ImageViewModel::onProjectChanged);
    QObject::connect(&projectViewModel_, &ProjectViewModel::changed,
                     &labelViewModel_, &LabelViewModel::onProjectChanged);
    QObject::connect(&projectViewModel_, &ProjectViewModel::statusChanged,
                     &mainWindow_, &MainWindow::showStatus);
    QObject::connect(&projectViewModel_, &ProjectViewModel::errorOccurred,
                     &mainWindow_, &MainWindow::showError);
}

void Application::syncLabelsToView()
{
    mainWindow_.setLabels(labelViewModel_.labelItems());
}
