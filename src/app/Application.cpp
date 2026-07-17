#include "app/Application.h"

#include <QObject>

Application::Application()
    : projectModel_{},
      projectRepository_{},
      yoloExporter_{},
      imageProcessor_{},
      imageViewModel_(projectModel_),
      annotationViewModel_(projectModel_),
      labelViewModel_(projectModel_),
      processViewModel_(imageProcessor_),
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
    bindImageFlow();
    bindAnnotationFlow();
    bindLabelFlow();
    bindProcessFlow();
    bindProjectFlow();
    bindFeedback();
}

void Application::bindImageFlow()
{
    QObject::connect(&mainWindow_, &MainWindow::importImageRequested,
                     &imageViewModel_, &ImageViewModel::loadImage);
    QObject::connect(&mainWindow_, &MainWindow::importFolderRequested,
                     &imageViewModel_, &ImageViewModel::loadFolder);
    QObject::connect(&mainWindow_, &MainWindow::previousImageRequested,
                     &imageViewModel_, &ImageViewModel::previousImage);
    QObject::connect(&mainWindow_, &MainWindow::nextImageRequested,
                     &imageViewModel_, &ImageViewModel::nextImage);

    QObject::connect(&imageViewModel_, &ImageViewModel::changed,
                     &mainWindow_, [this](ViewModelChange change) {
                         if (change == ViewModelChange::CurrentImage) {
                             processViewModel_.setSourceImage(
                                 imageViewModel_.currentQImage());
                         }
                     });
    QObject::connect(&imageViewModel_, &ImageViewModel::changed,
                     &annotationViewModel_, &AnnotationViewModel::onImageViewModelChanged);
}

void Application::bindAnnotationFlow()
{
    ImageCanvas& canvas = mainWindow_.imageCanvas();

    QObject::connect(&canvas, &ImageCanvas::annotationCreateRequested,
                     &annotationViewModel_, &AnnotationViewModel::createAnnotation);
    QObject::connect(&canvas, &ImageCanvas::annotationSelectRequested,
                     &annotationViewModel_, &AnnotationViewModel::selectAnnotation);
    QObject::connect(&canvas, &ImageCanvas::annotationSelectionClearRequested,
                     &annotationViewModel_, &AnnotationViewModel::clearSelection);
    QObject::connect(&mainWindow_, &MainWindow::selectedAnnotationLabelChangeRequested,
                     &annotationViewModel_, &AnnotationViewModel::setSelectedAnnotationLabel);
    QObject::connect(&mainWindow_, &MainWindow::deleteSelectedAnnotationRequested,
                     &annotationViewModel_, &AnnotationViewModel::deleteSelectedAnnotation);
    QObject::connect(&canvas, &ImageCanvas::selectedAnnotationRectChangeRequested,
                     &annotationViewModel_, &AnnotationViewModel::updateSelectedAnnotationRect);

    QObject::connect(&annotationViewModel_, &AnnotationViewModel::changed,
                     &mainWindow_, [this, &canvas](ViewModelChange change) {
                         if (change == ViewModelChange::Annotations) {
                             canvas.setAnnotations(annotationViewModel_.annotationItems());
                         }
                     });
    QObject::connect(&annotationViewModel_, &AnnotationViewModel::selectionChanged,
                     &mainWindow_, &MainWindow::setAnnotationEditEnabled);
}

void Application::bindLabelFlow()
{
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
    QObject::connect(&labelViewModel_, &LabelViewModel::labelsChanged,
                     &mainWindow_, [this]() { syncLabelsToView(); });
    QObject::connect(&labelViewModel_, &LabelViewModel::currentLabelChanged,
                     &annotationViewModel_, &AnnotationViewModel::setCurrentLabelId);
    QObject::connect(&labelViewModel_, &LabelViewModel::changed,
                     &annotationViewModel_, &AnnotationViewModel::onLabelViewModelChanged);
}

void Application::bindProcessFlow()
{
    ImageCanvas& canvas = mainWindow_.imageCanvas();

    QObject::connect(&mainWindow_, &MainWindow::grayscaleRequested,
                     &processViewModel_, &ProcessViewModel::previewGrayscale);
    QObject::connect(&mainWindow_, &MainWindow::binaryRequested,
                     &processViewModel_, &ProcessViewModel::previewBinary);
    QObject::connect(&mainWindow_, &MainWindow::meanBlurRequested,
                     &processViewModel_, &ProcessViewModel::previewMeanBlur);
    QObject::connect(&mainWindow_, &MainWindow::gaussianBlurRequested,
                     &processViewModel_, &ProcessViewModel::previewGaussianBlur);
    QObject::connect(&mainWindow_, &MainWindow::medianBlurRequested,
                     &processViewModel_, &ProcessViewModel::previewMedianBlur);
    QObject::connect(&mainWindow_, &MainWindow::cannyRequested,
                     &processViewModel_, &ProcessViewModel::previewCanny);
    QObject::connect(&mainWindow_, &MainWindow::brightnessRequested,
                     &processViewModel_, &ProcessViewModel::previewBrightness);
    QObject::connect(&mainWindow_, &MainWindow::contrastRequested,
                     &processViewModel_, &ProcessViewModel::previewContrast);
    QObject::connect(&mainWindow_, &MainWindow::processPreviewResetRequested,
                     &processViewModel_, &ProcessViewModel::resetPreview);
    QObject::connect(&mainWindow_, &MainWindow::processPreviewSaveRequested,
                     &processViewModel_, &ProcessViewModel::savePreview);

    QObject::connect(&processViewModel_, &ProcessViewModel::previewChanged,
                     &mainWindow_, [this, &canvas]() {
                         canvas.setImage(processViewModel_.displayImage());
                     });
    QObject::connect(&processViewModel_, &ProcessViewModel::previewAvailabilityChanged,
                     &mainWindow_, &MainWindow::setProcessPreviewActionsEnabled);
}

void Application::bindProjectFlow()
{
    QObject::connect(&mainWindow_, &MainWindow::openProjectRequested,
                     &projectViewModel_, &ProjectViewModel::openProject);
    QObject::connect(&mainWindow_, &MainWindow::saveProjectRequested,
                     &projectViewModel_, &ProjectViewModel::saveProject);
    QObject::connect(&mainWindow_, &MainWindow::exportYoloRequested,
                     &projectViewModel_, &ProjectViewModel::exportYolo);

    QObject::connect(&projectViewModel_, &ProjectViewModel::changed,
                     &imageViewModel_, &ImageViewModel::onProjectChanged);
    QObject::connect(&projectViewModel_, &ProjectViewModel::changed,
                     &labelViewModel_, &LabelViewModel::onProjectChanged);
}

void Application::bindFeedback()
{
    QObject::connect(&imageViewModel_, &ImageViewModel::statusChanged,
                     &mainWindow_, &MainWindow::showStatus);
    QObject::connect(&imageViewModel_, &ImageViewModel::errorOccurred,
                     &mainWindow_, &MainWindow::showError);
    QObject::connect(&annotationViewModel_, &AnnotationViewModel::errorOccurred,
                     &mainWindow_, &MainWindow::showError);
    QObject::connect(&labelViewModel_, &LabelViewModel::errorOccurred,
                     &mainWindow_, &MainWindow::showError);
    QObject::connect(&processViewModel_, &ProcessViewModel::statusChanged,
                     &mainWindow_, &MainWindow::showStatus);
    QObject::connect(&processViewModel_, &ProcessViewModel::errorOccurred,
                     &mainWindow_, &MainWindow::showError);
    QObject::connect(&projectViewModel_, &ProjectViewModel::statusChanged,
                     &mainWindow_, &MainWindow::showStatus);
    QObject::connect(&projectViewModel_, &ProjectViewModel::errorOccurred,
                     &mainWindow_, &MainWindow::showError);
}

void Application::syncLabelsToView()
{
    mainWindow_.setLabels(labelViewModel_.labelItems());
}
