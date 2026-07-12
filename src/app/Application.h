#pragma once

#include "model/ProjectModel.h"
#include "service/ImageImportService.h"
#include "view/MainWindow.h"
#include "viewmodel/AnnotationViewModel.h"
#include "viewmodel/ImageViewModel.h"
#include "viewmodel/LabelViewModel.h"

class Application final {
public:
    Application();

    void show();

private:
    void bindView();
    void bindViewModels();

    ProjectModel projectModel_;
    ImageImportService imageImportService_;
    ImageViewModel imageViewModel_;
    AnnotationViewModel annotationViewModel_;
    LabelViewModel labelViewModel_;
    MainWindow mainWindow_;
};
