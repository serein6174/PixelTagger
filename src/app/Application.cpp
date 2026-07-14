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

    QObject::connect(&imageViewModel_, &ImageViewModel::changed,
                     &mainWindow_, [this, &canvas](ViewModelChange change) {
                         if (change == ViewModelChange::CurrentImage) {
                             canvas.setImage(imageViewModel_.currentQImage());
                             annotationViewModel_.onCurrentImageChanged();
                         }
                     });
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
    QObject::connect(&labelViewModel_, &LabelViewModel::changed,
                     &mainWindow_, [this](ViewModelChange change) {
                         if (change == ViewModelChange::CurrentLabel) {
                             mainWindow_.setCurrentLabelName(labelViewModel_.currentLabelName());
                             annotationViewModel_.onLabelsChanged();
                         }
                     });
    QObject::connect(&labelViewModel_, &LabelViewModel::errorOccurred,
                     &mainWindow_, &MainWindow::showError);
}
