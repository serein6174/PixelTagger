#include "app/Application.h"

#include <QObject>

Application::Application()
    : projectModel_{},
      imageViewModel_(projectModel_),
      annotationViewModel_(projectModel_),
      labelViewModel_(projectModel_),
      mainWindow_{}
{
    bindView();
    bindViewModels();
    mainWindow_.setCurrentLabelName(labelViewModel_.currentLabelName());
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
    QObject::connect(&mainWindow_, &MainWindow::labelNameChangeRequested,
                     &labelViewModel_, &LabelViewModel::setCurrentLabelName);

    QObject::connect(&canvas, &ImageCanvas::annotationCreateRequested,
                     &annotationViewModel_, &AnnotationViewModel::createAnnotation);

    QObject::connect(&imageViewModel_, &ImageViewModel::imageChanged,
                     &canvas, &ImageCanvas::setImage);
    QObject::connect(&imageViewModel_, &ImageViewModel::statusChanged,
                     &mainWindow_, &MainWindow::showStatus);
    QObject::connect(&imageViewModel_, &ImageViewModel::errorOccurred,
                     &mainWindow_, &MainWindow::showError);
    QObject::connect(&annotationViewModel_, &AnnotationViewModel::annotationsChanged,
                     &canvas, &ImageCanvas::setAnnotations);
    QObject::connect(&annotationViewModel_, &AnnotationViewModel::errorOccurred,
                     &mainWindow_, &MainWindow::showError);
    QObject::connect(&labelViewModel_, &LabelViewModel::currentLabelNameChanged,
                     &mainWindow_, &MainWindow::setCurrentLabelName);
    QObject::connect(&labelViewModel_, &LabelViewModel::errorOccurred,
                     &mainWindow_, &MainWindow::showError);
}

void Application::bindViewModels()
{
    QObject::connect(&imageViewModel_, &ImageViewModel::currentImageChanged,
                     &annotationViewModel_, &AnnotationViewModel::onCurrentImageChanged);
    QObject::connect(&labelViewModel_, &LabelViewModel::labelsChanged,
                     &annotationViewModel_, &AnnotationViewModel::onLabelsChanged);
}
