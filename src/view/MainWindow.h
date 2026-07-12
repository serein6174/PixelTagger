#pragma once

#include <QMainWindow>

#include "view/canvas/ImageCanvas.h"

class QComboBox;

class MainWindow final : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);

    ImageCanvas& imageCanvas() noexcept;

signals:
    void importImageRequested(const QString& path);
    void importFolderRequested(const QString& path);
    void previousImageRequested();
    void nextImageRequested();
    void labelNameChangeRequested(const QString& name);

public slots:
    void showStatus(const QString& message);
    void showError(const QString& message);
    void setCurrentLabelName(const QString& name);

private slots:
    void openImage();
    void openFolder();

private:
    void createMenus();
    void createToolBar();
    void connectView();

    ImageCanvas* canvas_ = nullptr;
    QComboBox* labelComboBox_ = nullptr;
};
