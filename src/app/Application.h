#pragma once

#include "exporter/YoloExporter.h"
#include "model/ProjectModel.h"
#include "processor/ImageProcessor.h"
#include "repository/JsonProjectRepository.h"
#include "view/MainWindow.h"
#include "viewmodel/AnnotationViewModel.h"
#include "viewmodel/ImageViewModel.h"
#include "viewmodel/LabelViewModel.h"
#include "viewmodel/ProcessViewModel.h"
#include "viewmodel/ProjectViewModel.h"

class Application final {
public:
    Application();

    void show();

private:
    void bindView();
    void bindImageFlow();
    void bindAnnotationFlow();
    void bindLabelFlow();
    void bindProcessFlow();
    void bindProjectFlow();
    void bindFeedback();
    void syncLabelsToView();

    ProjectModel projectModel_;
    JsonProjectRepository projectRepository_;
    YoloExporter yoloExporter_;
    ImageProcessor imageProcessor_;
    ImageViewModel imageViewModel_;
    AnnotationViewModel annotationViewModel_;
    LabelViewModel labelViewModel_;
    ProcessViewModel processViewModel_;
    ProjectViewModel projectViewModel_;
    MainWindow mainWindow_;
};
