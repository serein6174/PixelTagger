#pragma once

#include <QMainWindow>
#include <QVector>

#include "common/presentation/LabelPresentationData.h"
#include "common/types/EntityIds.h"
#include "view/canvas/ImageCanvas.h"

class QAction;
class QComboBox;
class QDoubleSpinBox;
class QMenu;
class QSpinBox;
class QStackedWidget;

class MainWindow final : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);

    ImageCanvas& imageCanvas() noexcept;

signals:
    void importImageRequested(const QString& path);
    void importFolderRequested(const QString& path);
    void openProjectRequested(const QString& path);
    void saveProjectRequested(const QString& path);
    void previousImageRequested();
    void nextImageRequested();
    void labelNameChangeRequested(const QString& name);
    void addLabelRequested(const QString& name, const QColor& color);
    void removeLabelRequested(LabelId labelId);
    void renameLabelRequested(LabelId labelId, const QString& name);
    void labelColorChangeRequested(LabelId labelId, const QColor& color);
    void currentLabelChangeRequested(LabelId labelId);
    void selectedAnnotationLabelChangeRequested(LabelId labelId);
    void deleteSelectedAnnotationRequested();
    void exportYoloRequested(const QString& outputDirectory);
    void grayscaleRequested();
    void binaryRequested(int threshold);
    void meanBlurRequested(int kernelSize);
    void gaussianBlurRequested(int kernelSize);
    void medianBlurRequested(int kernelSize);
    void cannyRequested(int lowThreshold, int highThreshold);
    void brightnessRequested(int brightness);
    void contrastRequested(double contrast);
    void processPreviewResetRequested();
    void processPreviewSaveRequested(const QString& path);

public slots:
    void showStatus(const QString& message);
    void showError(const QString& message);
    void setCurrentLabelName(const QString& name);
    void setLabels(const QVector<LabelPresentationData>& labels);
    void setAnnotationEditEnabled(bool enabled);
    void setProcessPreviewActionsEnabled(bool enabled);

private slots:
    void openImage();
    void openFolder();
    void openProject();
    void saveProject();
    void exportYolo();
    void addLabel();
    void removeCurrentLabel();
    void changeCurrentLabelColor();
    void applyCurrentLabelToSelectedAnnotation();
    void deleteSelectedAnnotation();
    void applyProcessPreview();
    void saveProcessPreview();

private:
    void createMenus();
    void createToolBar();
    void createProcessDock();
    void connectView();
    LabelId currentComboLabelId() const noexcept;
    QColor currentComboLabelColor() const;
    int currentKernelSize(QComboBox* comboBox) const noexcept;

    ImageCanvas* canvas_ = nullptr;
    QMenu* viewMenu_ = nullptr;
    QComboBox* labelComboBox_ = nullptr;
    QAction* removeLabelAction_ = nullptr;
    QAction* changeLabelColorAction_ = nullptr;
    QAction* applyLabelToAnnotationAction_ = nullptr;
    QAction* deleteSelectedAnnotationAction_ = nullptr;
    QComboBox* processComboBox_ = nullptr;
    QStackedWidget* processParameterStack_ = nullptr;
    QSpinBox* binaryThresholdSpinBox_ = nullptr;
    QComboBox* meanKernelComboBox_ = nullptr;
    QComboBox* gaussianKernelComboBox_ = nullptr;
    QComboBox* medianKernelComboBox_ = nullptr;
    QSpinBox* cannyLowSpinBox_ = nullptr;
    QSpinBox* cannyHighSpinBox_ = nullptr;
    QSpinBox* brightnessSpinBox_ = nullptr;
    QDoubleSpinBox* contrastSpinBox_ = nullptr;
    QAction* resetProcessPreviewAction_ = nullptr;
    QAction* saveProcessPreviewAction_ = nullptr;
    bool annotationEditEnabled_ = false;
};
