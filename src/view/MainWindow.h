#pragma once

#include <QMainWindow>

#include "view/ImageCanvas.h"
#include "viewmodel/ImageViewModel.h"

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
    void connectViewModel();

    ImageCanvas* canvas_ = nullptr;
    ImageViewModel* imageViewModel_ = nullptr;
};
