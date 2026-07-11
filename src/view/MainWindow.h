#pragma once

#include <QMainWindow>

#include "model/ProjectModel.h"
#include "view/canvas/ImageCanvas.h"
#include "viewmodel/AnnotationViewModel.h"
#include "viewmodel/ImageViewModel.h"
#include "viewmodel/LabelViewModel.h"

class QComboBox;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);

private slots:
    void openImage();
    void openFolder();
    void showError(const QString& message);

private:
    void createMenus();
    void createToolBar();
    void connectViewModel();

    ProjectModel projectModel_;
    ImageCanvas* canvas_ = nullptr;
    QComboBox* labelComboBox_ = nullptr;
    ImageViewModel imageViewModel_;
    AnnotationViewModel annotationViewModel_;
    LabelViewModel labelViewModel_;
};
