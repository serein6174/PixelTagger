#pragma once

#include "model/ProjectModel.h"
#include "repository/JsonProjectRepository.h"
#include "view/MainWindow.h"
#include "viewmodel/AnnotationViewModel.h"
#include "viewmodel/ImageViewModel.h"
#include "viewmodel/LabelViewModel.h"
#include "viewmodel/ProjectViewModel.h"

class Application final {
public:
    Application();

    void show();

private:
    void bindView();
    void syncLabelsToView();

    ProjectModel projectModel_;
    JsonProjectRepository projectRepository_;
    ImageViewModel imageViewModel_;
    AnnotationViewModel annotationViewModel_;
    LabelViewModel labelViewModel_;
    ProjectViewModel projectViewModel_;
    MainWindow mainWindow_;
};
